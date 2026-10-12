#pragma once

#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <framework/interfaces/IRadio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

/**
 * @class ZephyrLoraRadio
 *
 * @brief This class implements the IRadio interface over the Zephyr LoRa API.
 *
 * The radio wiring (SPI, NSS, RESET, BUSY, DIO1) lives in the devicetree under
 * the `lora0` alias, so LoraRadioParams.pinConfig is ignored — the devicetree
 * owns the pins. Everything else in LoraRadioParams maps onto
 * lora_modem_config, with CR 4/7 and preamble length 8 pinned to the RadioMesh
 * wire contract (D5).
 *
 * The Zephyr driver is interrupt-driven end to end (DIO1 -> system workqueue
 * -> LoRaMac-node dispatch). This implementation preserves the Arduino port's
 * non-blocking flag model: sendPacket() starts the transmission and returns;
 * the TxDone interrupt raises a k_poll_signal that checkAndClearTxFlag()
 * polls, and received packets are buffered by the driver's receive callback
 * for checkAndClearRxFlag()/readReceivedData().
 */
class ZephyrLoraRadio : public IRadio
{
public:
    /**
     * @brief Get the instance of the ZephyrLoraRadio.
     * @returns A pointer to the instance of the ZephyrLoraRadio.
     */
    static ZephyrLoraRadio* getInstance();

    // IRadio interface
    virtual ~ZephyrLoraRadio() = default;

    virtual int setup(const LoraRadioParams& params) override;
    virtual int setup() override;
    virtual int standBy() override;
    virtual int sleep() override;
    virtual int getRSSI() override;
    virtual float getSNR() override;
    virtual int setParams(LoraRadioParams params) override;
    virtual int sendPacket(std::vector<byte>& data) override;
    virtual int startReceive() override;
    virtual int readReceivedData(std::vector<byte>* packetData) override;
    virtual bool checkAndClearRxFlag() override;
    virtual bool checkAndClearTxFlag() override;
    virtual int getRadioStateError() override;

private:
    ZephyrLoraRadio();
    ZephyrLoraRadio(const ZephyrLoraRadio&) = delete; // Prevent copy
    void operator=(const ZephyrLoraRadio&) = delete;  // Prevent assignment

    /**
     * @brief Driver receive callback (runs in the system workqueue).
     */
    static void onReceive(const struct device* dev, uint8_t* data, uint16_t size, int16_t rssi,
                          int8_t snr, void* userData);

    int checkLoraParameters(LoraRadioParams params);
    int configureModem(bool tx);
    int cancelReceive();

    /// @brief Maximum LoRa payload size accepted by the Zephyr driver.
    static constexpr size_t MAX_PAYLOAD_SIZE = 255;

    const struct device* radioDev = nullptr;
    LoraRadioParams radioParams;
    bool isSetup = false;
    bool rxArmed = false;

    // RX handoff: written by onReceive() (system workqueue context), consumed
    // from the application loop — guarded by rxLock.
    struct k_spinlock rxLock = {};
    bool rxDone = false;
    uint8_t rxBuffer[MAX_PAYLOAD_SIZE];
    uint16_t rxLength = 0;
    int16_t lastRssi = 0;
    int8_t lastSnr = 0;

    // TX completion: the TxDone interrupt raises txDoneSignal (the tx flag).
    // The driver raises no signal on TX timeout, so txDeadlineMs bounds the
    // wait (2x expected airtime, same margin the blocking lora_send uses).
    struct k_poll_signal txDoneSignal;
    bool txInFlight = false;
    uint32_t txDeadlineMs = 0;

    int16_t radioStateError = RM_E_NONE;
};
