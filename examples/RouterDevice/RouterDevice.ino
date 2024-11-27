#include <Arduino.h>
#include <Warp.h>
#include <string>
#include <arduino-timer.h>
#include "device_info.h"

std::string toHex(const byte *data, int size);
bool sendSensorData();
bool runSensor(void *);
void RxCallback(const WarpPacket &packet, int err);
void TxCallback(int err);
void displaySetupMsg(std::string message, int x, int y, bool clear, int delayMs);
bool setupDisplay();
bool setupRadio();


IDevice *device = nullptr;
IRadio *radio = nullptr;
IDisplay *display = nullptr;
IWifiConnector *wifiConnector = nullptr;
IWifiAccessPoint *wifiAP = nullptr;

DeviceBuilder builder;
bool setupOk = false;

auto timer = timer_create_default();
const int INTERVAL_MS = 15000;
int counter = 1;

// Default display configuration
OledDisplayParams displayParams = OledDisplayParams(SCL_OLED, SDA_OLED, RST_OLED, WARP_FONT_SMALL);

// convert a vector of bytes to a hex string
std::string toHex(const byte *data, int size)
{
    std::string buf;
    buf.reserve(size * 2); // Each byte will become two characters
    static const char hex[] = "0123456789ABCDEF";

    for (int i = 0; i < size; i++) {
        buf.push_back(hex[(data[i] >> 4) & 0xF]);
        buf.push_back(hex[data[i] & 0xF]);
    }

    return buf;
}

void TxCallback(int err)
{
      if (err != WARP_ERR_NONE) {
         logerr_ln("TX Failed [%d]", err);
         display->setFont(WARP_FONT_TINY);
         display->clear();
         display->drawString(0, 30, "TX failed"); display->drawString(45, 30, "Error: " + std::to_string(err));
      } else {
         loginfo_ln("Packet sent successfully");
      }
}

// Callback function for received packets
void RxCallback(const WarpPacket &packet, int err)
{
   std::string source = toHex(packet.sourceDevId.data(), packet.sourceDevId.size());
   std::string destination = toHex(packet.destinationDevId.data(), packet.destinationDevId.size());
   if (err != WARP_ERR_NONE) {

      logerr_ln("RX Failed [%d], from:%s, to:%s", err, source.c_str(), destination.c_str());
      display->setFont(WARP_FONT_TINY);
      display->clear();
      display->drawString(0, 30, "TX failed"); display->drawString(45, 30, "Error: " + std::to_string(err));
      return;
   }

   loginfo_ln("---------------------------------------------");
   loginfo_ln("Received packet ID: 0x%s [ RSSI: %ddBm, SNR: %.2fdB ]",
              toHex(packet.packetId.data(), packet.packetId.size()).c_str(), radio->getRSSI(), radio->getSNR());
   loginfo_ln("  Topic: 0x%02X", packet.topic);
   loginfo_ln("  Source: %s ", source.c_str());
   loginfo_ln("  Destination: %s", destination.c_str());
   loginfo_ln("  Device Type: 0x%02X", packet.deviceType);
   loginfo_ln("  Hop Count: %d", packet.hopCount);
   loginfo_ln("  Data CRC: 0x%04X", packet.dcrc);
   loginfo_ln("  Data: %s", WarpUtils::toString(packet.data).c_str());
   loginfo_ln("---------------------------------------------");

   display->setFont(WARP_FONT_TINY);
   display->clear();
   display->drawString(90,10, getWifiSignal());
   display->drawString(0, 30, "pkt id: "); display->drawString(45, 30, toHex(packet.packetId.data(), packet.packetId.size()));
   display->drawString(0, 40,"from:"); display->drawString(30, 40, source);
   display->drawString(0, 50,"to:"); display->drawString(30, 50, destination);
   display->drawString(0, 60,WarpUtils::toString(packet.data));
}

// Display a message on the OLED display
void displaySetupMsg(std::string message, int x=10, int y=40, bool clear=true, int delayMs=1000)
{
   if (clear) {
      display->clear();
   }
   display->drawString(x, y, message);
   delay(delayMs);
}

std::string getWifiSignal()
{
   SignalIndicator si = wifiConnector->getSignalIndicator();
   return WarpUtils::wifiSignalToString(si);
}


bool setupDisplay()
{
   display = device->getDisplay();
   if (display == nullptr) {
      logerr_ln("ERROR  display is null");

      return false;
   } else {
      // Setup the display with the built-in configuration
      if (display->setup() != WARP_ERR_NONE) {
         logerr_ln("ERROR  display setup failed");
         return false;
      }
      display->clear();
   }
   return true;
}

bool setupRadio()
{
   radio = device->getRadio();
   if (radio == nullptr) {
      logerr_ln("ERROR  radio is null");
      displaySetupMsg("E: radio null");
      return false;
   } else {
      // Setup the radio with the built-in configuration
      if (radio->setup() != WARP_ERR_NONE) {
         logerr_ln("ERROR  radio setup failed");
         displaySetupMsg("E: radio setup");
         return false;
      }
   }

   return true;
}

bool setupWifiConnector()
{
   wifiConnector = device->getWifiConnector();
   if (wifiConnector == nullptr) {
      logerr_ln("ERROR  wifiConnector is null");
      displaySetupMsg("E: wifi null");
      return false;
   } else {
      // Setup the wifi connector with the built-in configuration
      if (wifiConnector->connect() != WARP_ERR_NONE) {
         logerr_ln("ERROR: WifiConnector setup failed.");
         displaySetupMsg("E: wifi failed");
         return false;
      }
   }
   return true;
}

bool setupAccessPoint()
{
   wifiAP = device->getWifiAccessPoint();
   if (wifiAP == nullptr) {
      logerr_ln("ERROR  wifiAP is null");
      displaySetupMsg("E: wifiAP null");
      return false;
   } else {
      // Setup the wifi access point with the built-in configuration
      if (wifiAP->setup() != WARP_ERR_NONE) {
         logerr_ln("ERROR: WifiAP setup failed.");
         displaySetupMsg("E: wifiAP setup");
         return false;
      }
      if (wifiAP->start() != WARP_ERR_NONE) {
         logerr_ln("ERROR: WifiAP start failed.");
         displaySetupMsg("E: wifiAP failed");
         return false;
      }
   }
   return true;
}


bool sendSensorData()
{
   bool result = false;

   std::string message = "Counter-1:" + std::to_string(counter);
   std::vector<byte> buffer(message.begin(), message.end());

   loginfo_ln("[DEVICE]  Sending sensor data %s", message.c_str());
   int err = device->sendData(topics::text_message, buffer);
   if (err == WARP_ERR_NONE) {
   loginfo_ln("[DEVICE]  Sensor data sent");
      result = true;
      counter++;
   } else {
      logerr_ln("[DEVICE]  ERROR - Failed to send sensor data");
      return false;
   }
   return result;
}

bool runSensor(void *)
{
   loginfo_ln("[DEVICE]  Running sensor");
   return sendSensorData();
}

void setup()
{

   // Create the device first using the Device Builder
   // This device does not have wifi capabilities
   device = builder.start()
                   .withLoraRadio(LoraRadioPresets::HELTEC_WIFI_LORA_32_V3)
                   .withRelayEnabled(true)
                   .withRxPacketCallback(RxCallback)
                   .withTxPacketCallback(TxCallback)
                   .withAesCrypto(key, iv)
                   .withOledDisplay(displayParams)
                   .build(DEVICE_NAME, DEVICE_ID, WarpDeviceType::ROUTER);

   if (device == nullptr) {
      logerr_ln("ERROR  device is null");
      setupOk = false;
      return;
   }

   // Setup the device components with the built-in configuration

   setupOk = setupDisplay();
   if (!setupOk) {
      logerr_ln("ERROR  display setup failed");
      return;
   }

   setupOk = setupRadio();
   if (!setupOk) {
      logerr_ln("ERROR  radio setup failed");
      return;
   }
   displaySetupMsg("I: radio ok", 10, 20, false, 1000);

   setupOk = true;
   loginfo_ln("INFO  setup complete");

   displaySetupMsg("Setup OK!");

   // Send the first sensor data and start the timer
   if (sendSensorData()) {
      timer.every(INTERVAL_MS, runSensor);
   } else {
      loginfo_ln("[DEVICE] ERROR - Failed to send sensor data");
   }
}

void loop() {
   if (!setupOk) {
      return;
   }
   device->run();
   timer.tick();
}

