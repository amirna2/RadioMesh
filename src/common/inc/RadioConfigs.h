#pragma once

#include <string>
#include <common/inc/Options.h>

/**
 * @class PinConfig
 *
 * @brief This class is used to store the pin configuration for the LoRa radio.
 *
 */
class PinConfig
{
public:
    PinConfig(int ss = 10, int rst = 9, int di0 = 2, int di1 = 3)
        : ss(ss), rst(rst), di0(di0), di1(di1) {}

    /// @brief slave select pin (cs pin)
    int ss;
    /// @brief reset pin
    int rst;
    /// @brief DIO0 pin (BUSY pin for SX1262 radios)
    int di0;
    /// @brief DIO1 pin
    int di1;
};

/**
 * @class LoraRadioParams
 *
 * @brief This class is used to store the parameters of the LoRa radio.
 *
 */
class LoraRadioParams
{
public:
    // Constructor with default parameters
    LoraRadioParams(PinConfig pinConfig = PinConfig(), float band = 915.0, int8_t txPower = 14, float bw = 125.0, uint8_t sf = 7, uint8_t gain = 0, bool privateNetwork = true)
        : pinConfig(pinConfig), band(band), txPower(txPower), bw(bw), sf(sf), gain(gain), privateNetwork(privateNetwork) {}

    // Fluent interface for setting parameters
    LoraRadioParams& setPinConfig(const PinConfig& pinConfig) { this->pinConfig = pinConfig; return *this; }
    LoraRadioParams& setBand(float band) { this->band = band; return *this; }
    LoraRadioParams& setTxPower(int8_t txPower) { this->txPower = txPower; return *this; }
    LoraRadioParams& setBW(float bw) { this->bw = bw; return *this; }
    LoraRadioParams& setSF(uint8_t sf) { this->sf = sf; return *this; }
    LoraRadioParams& setGain(uint8_t gain) { this->gain = gain; return *this; }
    LoraRadioParams& setPrivateNetwork(bool privateNetwork) { this->privateNetwork = privateNetwork; return *this; }

    // Method to validate parameters
    bool validate() const {
        // Add validation logic here
        if (txPower < 2 || txPower > 20) {
            return false;
        }
        // Add more validation as needed
        return true;
    }

    // Assignment operator
    LoraRadioParams& operator=(const LoraRadioParams& other) {
        if (this != &other) {
            pinConfig = other.pinConfig;
            band = other.band;
            txPower = other.txPower;
            bw = other.bw;
            sf = other.sf;
            gain = other.gain;
            privateNetwork = other.privateNetwork;
        }
        return *this;
    }

    // Method to reset parameters to zero
    void reset() {
        pinConfig = PinConfig(0, 0, 0, 0);
        band = 0.0;
        txPower = 0;
        bw = 0.0;
        sf = 0;
        gain = 0;
        privateNetwork = false;
    }

    // Method to convert parameters to string
    std::string toString() const {
        // Convert to Hz. Some compiler versions do not support std::to_string with float
        uint32_t band_hz = band * 1000 * 1000;
        uint32_t bw_hz = bw * 1000 * 1000;

        return std::string("LoraRadioParams(ss=") + std::to_string(pinConfig.ss) +
               std::string(", rst=") + std::to_string(pinConfig.rst) +
               std::string(", di0=") + std::to_string(pinConfig.di0) +
               std::string(", di1=") + std::to_string(pinConfig.di1) +
               std::string(", band=") + std::to_string(band_hz) +
               std::string(", txPower=") + std::to_string(txPower) +
               std::string(", bw=") + std::to_string(bw_hz) +
               std::string(", sf=") + std::to_string(sf) +
               std::string(", gain=") + std::to_string(gain) +
               std::string(", privateNetwork=") + std::to_string(privateNetwork) +
               std::string(")");
    }

public:
    /// @brief Pin configuration
    PinConfig pinConfig;
    /// @brief frequency band
    float band;
    /// @brief transmission power
    int8_t txPower;
    /// @brief bandwidth
    float bw;
    /// @brief spreading factor
    uint8_t sf;
    /// @brief gain
    uint8_t gain;
    /// @brief private network flag
    bool privateNetwork;
};

// Preset configurations for different boards
namespace LoraRadioPresets {
    const LoraRadioParams HELTEC_WIFI_LORA_32_V3(PinConfig(8, 12, 13, 14), 915.0, 20, 125.0, 8, 0, true);
    const LoraRadioParams HELTEC_CUBECELL(PinConfig(35, 47, 39, 38), 915.0, 20, 125.0, 7, 0, true);
}
