#include <vector>
#include <string>

#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <common/utils/RadioMeshCrc32.h>
#include <common/inc/Definitions.h>
#include <common/inc/RadioConfigs.h>
#include <hardware/inc/radio/LoraRadio.h>

LoraRadio* LoraRadio::instance = nullptr;

int LoraRadio::setParams(LoraRadioParams params)
{
   if (checkLoraParameters(params) != RM_E_NONE) {
      logerr_ln("ERROR:  invalid radio parameters");
      return RM_E_INVALID_RADIO_PARAMS;
   }

   radioParams = params;
   radio = new Module(radioParams.pinConfig.ss, radioParams.pinConfig.di1, radioParams.pinConfig.rst, radioParams.pinConfig.di0 /*busy pin*/);

   return RM_E_NONE;
}

int LoraRadio::checkLoraParameters(LoraRadioParams params) {
    int rc = RM_E_NONE;
    if (params.sf < 6 || params.sf > 12) {
        logerr_ln("ERROR  spreading factor is invalid");
        return RM_E_INVALID_PARAM;
    }
    if (params.band < 150.0 || params.band > 960.0) {
        logerr_ln("ERROR  frequency is invalid");
        return RM_E_INVALID_PARAM;
    }
    if (params.txPower < -9 || params.txPower > 22) {
        logerr_ln("ERROR  tx power is invalid");
        return RM_E_INVALID_PARAM;
    }
    if (params.bw < 7.8 || params.bw > 500.0) {
        logerr_ln("ERROR  bandwidth is invalid");
        return RM_E_INVALID_PARAM;
    }
    if (params.gain < 0 || params.gain > 3) {
        logerr_ln("ERROR  gain is invalid");
        return RM_E_INVALID_PARAM;
    }

    return rc;
}

int LoraRadio::setup(const LoraRadioParams& params)
{
   int rc = checkLoraParameters(params);
   if (rc != RM_E_NONE) {
      return rc;
   }
   if (isSetup) {
      logwarn_ln("WARNING LoRa overwriting existing lora parameters.");
   }

   rc = radio.begin();

   if (rc != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR  initializing LoRa driver. state = %d", rc);
      return RM_E_RADIO_SETUP;
   }

   // the radio is started, we need to set all the radio parameters, before it can
   // start receiving packets
   loginfo_ln("Setting up LoRa radio parameters...");
   rc = radio.setFrequency(params.band);
   if (rc == RADIOLIB_ERR_INVALID_FREQUENCY) {
      logerr_ln("ERROR  frequency is invalid");
      return RM_E_RADIO_SETUP;
   }

   rc = radio.setBandwidth(params.bw);
   if (rc == RADIOLIB_ERR_INVALID_BANDWIDTH) {
      logerr_ln("ERROR  bandwidth is invalid");
      return RM_E_RADIO_SETUP;
   }

   rc = radio.setSpreadingFactor(params.sf);
   if (rc == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
      logerr_ln("ERROR  spreading factor is invalid");
      return RM_E_RADIO_SETUP;
   }

   rc = radio.setOutputPower(params.txPower);
   if (rc == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
      logerr_ln("ERROR  output power is invalid");
      return RM_E_RADIO_SETUP;
   }

   if (params.privateNetwork) {
      rc = radio.setSyncWord(0x12);
   }
   else {
      rc = radio.setSyncWord(0x34);
   }

   if (rc != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR  sync word is invalid");
      return RM_E_RADIO_SETUP;
   }

   // set the interrupt handler to execute when packet tx or rx is done.
   radio.setDio1Action(LoraRadio::onInterrupt);

   rc = radio.startReceive();
   if (rc != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR Failed to start receive");
      return RM_E_RADIO_SETUP;
   }
   loginfo_ln("LoRa radio setup complete");
   isSetup = true;
   radioParams = params;

   return RM_E_NONE;
}

int LoraRadio::setup()
{
   return setup(radioParams);
}

int LoraRadio::sendPacket(std::vector<byte>& data)
{
   return startTransmitPacket(data.data(), data.size());
}

int LoraRadio::startReceive()
{
   if (!isSetup) {
      logerr_ln("ERROR  LoRa radio not setup");
      return RM_E_RADIO_NOT_INITIALIZED;
   }

   loginfo_ln("Start receiving data...");
   int state = radio.startReceive();

   if (state != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR startReceive failed, code %d", state);
      return RM_E_RADIO_NOT_INITIALIZED;
   }

   return RM_E_NONE;
}

int LoraRadio::startTransmitPacket(byte *data, int length)
{
   int err = RM_E_NONE;
   int tx_err = RADIOLIB_ERR_NONE;
   logdbg_ln("TX Data - len: %d, %s", length, RadioMeshUtils::convertToHex(data, length).c_str());

   resetRadioState(TX_STATE);

   long t1 = millis();
   // TODO: Request RadioLib to use a const byte* instead of byte*
   tx_err = radio.startTransmit(data, length);
   switch (tx_err) {
      case RADIOLIB_ERR_NONE:
         logdbg_ln("TX data done in : %d ms",(millis() - t1));
         err = RM_E_NONE;
         break;

      case RADIOLIB_ERR_PACKET_TOO_LONG:
         logerr_ln("ERROR startTransmitData too long!");
         err = RM_E_PACKET_TOO_LONG;
         break;

      case RADIOLIB_ERR_TX_TIMEOUT:
         logerr_ln("ERROR startTransmitData timeout!");
         err = RM_E_RADIO_TX_TIMEOUT;
         break;

      default:
         logerr_ln("ERROR startTransmitData failed, err: %d", tx_err);
         err = RM_E_RADIO_TX;
         break;
   }

   return err;
}

int LoraRadio::standBy()
{
   int rc = radio.standby();
   if (rc != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR  standby failed, code %d", rc);
      return RM_E_RADIO_FAILURE;
   }
   return RM_E_NONE;
}

float LoraRadio::getSNR()
{
   return radio.getSNR();
}

int LoraRadio::getRSSI()
{
   return radio.getRSSI();
}

int LoraRadio::sleep()
{
   int rc = radio.sleep();
   if (rc != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR  sleep failed, code %d", rc);
      return RM_E_RADIO_FAILURE;
   }
   return RM_E_NONE;
}

int LoraRadio::readReceivedData(std::vector<byte>* packetBytes)
{
   int packet_length = 0;
   int err = RM_E_NONE;
   int rxState = RM_E_NONE;

   if (!isSetup) {
      logerr_ln("ERROR  LoRa radio not setup");
      return RM_E_RADIO_NOT_INITIALIZED;
   }

   packet_length = radio.getPacketLength();
   if (packet_length < 0) {
      logerr_ln("ERROR  getPacketLength failed. err = %d", packet_length);
      return RM_E_RADIO_FAILURE;
   }
   logtrace_ln("readReceivedData() - packet length returns: %d", packet_length);

   packetBytes->resize(packet_length);
   err = radio.readData(packetBytes->data(), packet_length);
   if (err != RADIOLIB_ERR_NONE) {
      logerr_ln("ERROR  readReceivedData failed. err = %d", err);
      return RM_E_RADIO_FAILURE;
   }

   logdbg_ln("Rx packet: %s", RadioMeshUtils::convertToHex(packetBytes->data(), packetBytes->size()).c_str());
   logdbg_ln("RX: rssi: %f snr: %f size: %d", radio.getRSSI(), radio.getSNR(), packet_length);

   resetRadioState(RX_TX_STATE);

   logtrace_ln("readReceivedData() - DONE");
   return err;
}

// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
#if defined (ESP32) || defined (ESP8266)
void ICACHE_RAM_ATTR LoraRadio::onInterrupt()
#else
void LoraRadio::onInterrupt()
#endif
{
   // This function will be called immediately when an interrupt occurs
   uint16_t irqStatus = instance->radio.getIrqFlags();

   // check for rx/tx done first and then check for errors
   if (irqStatus & RADIOLIB_SX126X_IRQ_RX_DONE) {
      instance->rxDone = true;
   }
   if (irqStatus & RADIOLIB_SX126X_IRQ_TX_DONE) {
      instance->txDone = true;
   }

   if (irqStatus & RADIOLIB_SX126X_IRQ_TIMEOUT ) {
      if (instance->rxDone) {
         instance->radioStateError = RM_E_RADIO_RX_TIMEOUT;
      }
      if (instance->txDone) {
         instance->radioStateError = RM_E_RADIO_TX_TIMEOUT;
      }
   }

   if (irqStatus & RADIOLIB_SX126X_IRQ_CRC_ERR ) {
      instance->radioStateError = RM_E_RADIO_CRC_MISMATCH;
   }
   if (irqStatus & RADIOLIB_SX126X_IRQ_HEADER_ERR ) {
      instance->radioStateError = RM_E_RADIO_HEADER_CRC_MISMATCH;
   }
}

bool LoraRadio::checkAndClearRxFlag() {

   if (rxDone) {
      rxDone = false;
      return true;
   }
   return false;
}

bool LoraRadio::checkAndClearTxFlag() {
   if (txDone) {
      txDone = false;
      return true;
   }
   return false;
}

int LoraRadio::getRadioStateError()
{
   // TODO: review this function. There's probably a better place to switch to receive mode when an error occurs.
   int error = radioStateError;
   if (radioStateError != RM_E_NONE) {
      resetRadioState();
      switchToReceiveMode();
   }
   return radioStateError;
}

int LoraRadio::switchToReceiveMode()
{
   if (!isSetup) {
      logerr_ln("ERROR  LoRa radio not setup");
      return RM_E_RADIO_NOT_INITIALIZED;
   }
   resetRadioState(RX_TX_STATE);
   return startReceive();
}
