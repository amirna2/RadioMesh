#include <cstring>
#include <errno.h>

#include <zephyr/drivers/lora.h>

#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <radio/ZephyrLoraRadio.h>

namespace
{
// The radio device is resolved from the devicetree `lora0` alias; the
// devicetree (ports/zephyr/common/*.overlay) owns all pin wiring.
const struct device* const loraDevice = DEVICE_DT_GET(DT_ALIAS(lora0));

// D5 wire contract: CR 4/7 and preamble 8 are RadioLib begin() defaults, never
// set explicitly by the Arduino port, so they are pinned here to match.
// Changing either silently breaks Arduino/Zephyr interop. (Absorbed from M1's
// ports/zephyr/common/lora_phy.h.)
constexpr enum lora_coding_rate RM_CODING_RATE = CR_4_7;
constexpr uint16_t RM_PREAMBLE_LENGTH = 8;
} // namespace

ZephyrLoraRadio* ZephyrLoraRadio::getInstance()
{
    // Static-storage singleton: same contract as the Arduino port's lazy new,
    // without requiring heap allocation in the port layer.
    static ZephyrLoraRadio instance;
    return &instance;
}

ZephyrLoraRadio::ZephyrLoraRadio()
{
    k_poll_signal_init(&txDoneSignal);
}

int ZephyrLoraRadio::checkLoraParameters(LoraRadioParams params)
{
    // Same acceptance ranges as the Arduino port (LoraRadio.cpp).
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

    return RM_E_NONE;
}

int ZephyrLoraRadio::setParams(LoraRadioParams params)
{
    if (checkLoraParameters(params) != RM_E_NONE) {
        logerr_ln("ERROR:  invalid radio parameters");
        return RM_E_INVALID_RADIO_PARAMS;
    }

    radioParams = params;

    return RM_E_NONE;
}

int ZephyrLoraRadio::configureModem(bool tx)
{
    struct lora_modem_config cfg = {};

    cfg.frequency = static_cast<uint32_t>(radioParams.band * 1000000.0f);

    // The Zephyr LoRa API supports only the three discrete LoRa bandwidths;
    // the enum values are the kHz figures, so the mapping is by value.
    switch (static_cast<int>(radioParams.bw)) {
    case 125:
        cfg.bandwidth = BW_125_KHZ;
        break;
    case 250:
        cfg.bandwidth = BW_250_KHZ;
        break;
    case 500:
        cfg.bandwidth = BW_500_KHZ;
        break;
    default:
        logerr_ln("ERROR  bandwidth %d kHz not supported by the Zephyr LoRa API",
                  static_cast<int>(radioParams.bw));
        return RM_E_INVALID_RADIO_PARAMS;
    }

    // lora_datarate values equal the numeric spreading factor (SF_6 == 6 ...).
    cfg.datarate = static_cast<enum lora_datarate>(radioParams.sf);
    cfg.coding_rate = RM_CODING_RATE;
    cfg.preamble_len = RM_PREAMBLE_LENGTH;
    cfg.tx_power = radioParams.txPower;
    cfg.tx = tx;
    cfg.iq_inverted = false;
    // Arduino sync words: private 0x12, public 0x34 — the driver applies the
    // same pair through SetPublicNetwork().
    cfg.public_network = !radioParams.privateNetwork;
    // params.gain (RadioLib RX boosted-gain mode) has no Zephyr API equivalent
    // and is not set by any RadioMesh preset; it is validated but unused here.

    int rc = lora_config(radioDev, &cfg);
    if (rc < 0) {
        logerr_ln("ERROR  lora_config(%s) failed, code %d", tx ? "tx" : "rx", rc);
        return RM_E_RADIO_SETUP;
    }
    return RM_E_NONE;
}

int ZephyrLoraRadio::setup(const LoraRadioParams& params)
{
    loginfo_ln("Setting up LoRa radio...");

    if (!device_is_ready(loraDevice)) {
        logerr_ln("ERROR  LoRa device not ready");
        return RM_E_RADIO_SETUP;
    }

    int rc = checkLoraParameters(params);
    if (rc != RM_E_NONE) {
        logerr_ln("ERROR  invalid radio parameters");
        return rc;
    }
    if (isSetup) {
        logwarn_ln("WARNING LoRa overwriting existing lora parameters.");
    }

    radioDev = loraDevice;
    radioParams = params;

    // Configure both directions up front: the underlying Semtech driver keeps
    // TX and RX configuration independently, and the TX config also feeds
    // lora_airtime() used for the transmit deadline.
    rc = configureModem(true);
    if (rc != RM_E_NONE) {
        return rc;
    }
    rc = configureModem(false);
    if (rc != RM_E_NONE) {
        return rc;
    }

    isSetup = true;

    rc = startReceive();
    if (rc != RM_E_NONE) {
        logerr_ln("ERROR Failed to start receive");
        return rc;
    }

    return RM_E_NONE;
}

int ZephyrLoraRadio::setup()
{
    // setup the radio with previously stored parameters
    return setup(radioParams);
}

int ZephyrLoraRadio::sendPacket(std::vector<byte>& data)
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return RM_E_RADIO_NOT_INITIALIZED;
    }
    if (data.size() > MAX_PAYLOAD_SIZE) {
        logerr_ln("ERROR startTransmitData too long!");
        return RM_E_PACKET_TOO_LONG;
    }

    logdbg_ln("TX Data - len: %d, %s", static_cast<int>(data.size()),
              RadioMeshUtils::convertToHex(data.data(), data.size()).c_str());

    // The driver enforces modem exclusivity: continuous reception holds the
    // modem and lora_send_async would return -EBUSY, so reception is cancelled
    // first. Device::run() re-arms it via startReceive() after the tx flag.
    cancelReceive();

    k_poll_signal_reset(&txDoneSignal);

    int rc = lora_send_async(radioDev, data.data(), data.size(), &txDoneSignal);
    if (rc < 0) {
        logerr_ln("ERROR startTransmitData failed, err: %d", rc);
        return RM_E_RADIO_TX;
    }

    // The driver raises no signal on TX timeout (sx12xx_ev_tx_timed_out only
    // releases the modem), so completion is bounded by twice the expected
    // airtime — the same margin the blocking lora_send applies.
    txDeadlineMs = k_uptime_get_32() + 2 * lora_airtime(radioDev, data.size());
    txInFlight = true;

    return RM_E_NONE;
}

int ZephyrLoraRadio::startReceive()
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return RM_E_RADIO_NOT_INITIALIZED;
    }

    if (rxArmed) {
        // Continuous reception: the driver re-arms itself after each packet.
        return RM_E_NONE;
    }

    loginfo_ln("Start receiving data...");
    int state = lora_recv_async(radioDev, &ZephyrLoraRadio::onReceive, this);

    if (state < 0) {
        logerr_ln("ERROR startReceive failed, code %d", state);
        return RM_E_RADIO_NOT_INITIALIZED;
    }

    rxArmed = true;
    return RM_E_NONE;
}

int ZephyrLoraRadio::cancelReceive()
{
    if (!rxArmed) {
        return RM_E_NONE;
    }

    // A NULL callback cancels reception and releases the modem; -EINVAL means
    // it was not receiving (already released), which is fine here.
    int rc = lora_recv_async(radioDev, nullptr, nullptr);
    if (rc < 0 && rc != -EINVAL) {
        logwarn_ln("WARNING cancel receive failed, code %d", rc);
    }

    rxArmed = false;
    return RM_E_NONE;
}

void ZephyrLoraRadio::onReceive(const struct device* dev, uint8_t* data, uint16_t size,
                                int16_t rssi, int8_t snr, void* userData)
{
    // Runs in the system workqueue (DIO1 interrupt -> work item -> LoRaMac
    // dispatch). The driver has already re-armed reception and has already
    // dropped any PHY-CRC-corrupt frame (such frames never reach this
    // callback; the RadioMesh packet CRC32 and MIC still validate end to end).
    ARG_UNUSED(dev);
    ZephyrLoraRadio* self = static_cast<ZephyrLoraRadio*>(userData);

    K_SPINLOCK(&self->rxLock)
    {
        // A packet not yet consumed by the run loop is overwritten, matching
        // the Arduino port where RadioLib holds only the latest packet.
        uint16_t length = size > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : size;
        memcpy(self->rxBuffer, data, length);
        self->rxLength = length;
        self->lastRssi = rssi;
        self->lastSnr = snr;
        self->rxDone = true;
    }
}

int ZephyrLoraRadio::readReceivedData(std::vector<byte>* packetData)
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return RM_E_RADIO_NOT_INITIALIZED;
    }

    // Copy out under the lock, grow the vector outside it: std::vector may
    // allocate, and allocation is not legal inside a spinlock section.
    uint8_t localBuffer[MAX_PAYLOAD_SIZE];
    uint16_t localLength = 0;
    K_SPINLOCK(&rxLock)
    {
        localLength = rxLength;
        memcpy(localBuffer, rxBuffer, localLength);
        // Mirror the Arduino port's resetRadioState(RX_TX_STATE) on read.
        rxDone = false;
    }
    packetData->assign(localBuffer, localBuffer + localLength);

    logdbg_ln("Rx packet: %s",
              RadioMeshUtils::convertToHex(packetData->data(), packetData->size()).c_str());
    logdbg_ln("RX: rssi: %d snr: %d size: %d", lastRssi, lastSnr,
              static_cast<int>(packetData->size()));

    radioStateError = RM_E_NONE;

    return RM_E_NONE;
}

bool ZephyrLoraRadio::checkAndClearRxFlag()
{
    bool wasDone = false;
    K_SPINLOCK(&rxLock)
    {
        if (rxDone) {
            rxDone = false;
            wasDone = true;
        }
    }
    return wasDone;
}

bool ZephyrLoraRadio::checkAndClearTxFlag()
{
    if (!txInFlight) {
        return false;
    }

    unsigned int signaled = 0;
    int result = 0;
    k_poll_signal_check(&txDoneSignal, &signaled, &result);

    if (signaled) {
        k_poll_signal_reset(&txDoneSignal);
        if (result < 0) {
            radioStateError = RM_E_RADIO_TX;
        }
        txInFlight = false;
        return true;
    }

    if (static_cast<int32_t>(k_uptime_get_32() - txDeadlineMs) >= 0) {
        // TxDone never arrived. Record the same error the Arduino interrupt
        // handler does and force the modem free (the cancel path releases it
        // from any state) so the RX re-arm that follows can succeed.
        logerr_ln("ERROR startTransmitData timeout!");
        lora_recv_async(radioDev, nullptr, nullptr);
        radioStateError = RM_E_RADIO_TX_TIMEOUT;
        txInFlight = false;
        return true;
    }

    return false;
}

int ZephyrLoraRadio::getRadioStateError()
{
    // Mirrors the Arduino port: when an error is pending, reset the radio
    // state and return to receive mode; the recorded error is cleared by the
    // recovery, so callers observe RM_E_NONE afterwards (same observable
    // behavior as LoraRadio::getRadioStateError()).
    if (radioStateError != RM_E_NONE) {
        radioStateError = RM_E_NONE;
        K_SPINLOCK(&rxLock)
        {
            rxDone = false;
        }
        txInFlight = false;
        startReceive();
    }
    return radioStateError;
}

int ZephyrLoraRadio::getRSSI()
{
    return lastRssi;
}

float ZephyrLoraRadio::getSNR()
{
    return lastSnr;
}

int ZephyrLoraRadio::standBy()
{
    // The Zephyr LoRa API has no standby state: the driver sleeps the radio
    // whenever the modem is released, so cancelling reception is the closest
    // equivalent idle state.
    return cancelReceive();
}

int ZephyrLoraRadio::sleep()
{
    return cancelReceive();
}
