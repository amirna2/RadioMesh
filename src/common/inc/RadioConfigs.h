#pragma once

#include <common/inc/Options.h>
#include <string>

/**
 * @class PinConfig
 *
 * @brief This class is used to store the pin configuration for the LoRa radio.
 *
 */
class PinConfig
{
public:
   /// @brief Undefined pin value
   constexpr static int PIN_UNDEFINED = 0;

   /// @brief Default slave select pin
   constexpr static int PIN_DEFAULT_SS = 10;

   /// @brief Default reset pin
   constexpr static int PIN_DEFAULT_RST = 9;

   /// @brief Default DIO0 pin
   constexpr static int PIN_DEFAULT_DI0 = 2;

   /// @brief Default DIO1 pin
   constexpr static int PIN_DEFAULT_DI1 = 3;

   // Constructor with default parameters
   PinConfig(int ss = PIN_DEFAULT_SS, int rst = PIN_DEFAULT_RST, int di0 = PIN_DEFAULT_DI0,
             int di1 = PIN_DEFAULT_DI1)
       : ss(ss), rst(rst), di0(di0), di1(di1)
   {
   }

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
   constexpr static float DEFAULT_BAND = 915.0;
   constexpr static int8_t DEFAULT_TX_POWER = 14;
   constexpr static float DEFAULT_BW = 125.0;
   constexpr static uint8_t DEFAULT_SF = 7;
   constexpr static uint8_t DEFAULT_GAIN = 0;
   constexpr static bool DEFAULT_PRIVATE_NETWORK = true;

   LoraRadioParams(PinConfig pinConfig = PinConfig(), float band = DEFAULT_BAND,
                   int8_t txPower = DEFAULT_TX_POWER, float bw = DEFAULT_BW,
                   uint8_t sf = DEFAULT_SF, uint8_t gain = DEFAULT_GAIN,
                   bool privateNetwork = DEFAULT_PRIVATE_NETWORK)
       : pinConfig(pinConfig), band(band), txPower(txPower), bw(bw), sf(sf), gain(gain),
         privateNetwork(privateNetwork)
   {
   }

   LoraRadioParams& setPinConfig(const PinConfig& pinConfig)
   {
      this->pinConfig = pinConfig;
      return *this;
   }
   LoraRadioParams& setBand(float band)
   {
      this->band = band;
      return *this;
   }
   LoraRadioParams& setTxPower(int8_t txPower)
   {
      this->txPower = txPower;
      return *this;
   }
   LoraRadioParams& setBW(float bw)
   {
      this->bw = bw;
      return *this;
   }
   LoraRadioParams& setSF(uint8_t sf)
   {
      this->sf = sf;
      return *this;
   }
   LoraRadioParams& setGain(uint8_t gain)
   {
      this->gain = gain;
      return *this;
   }
   LoraRadioParams& setPrivateNetwork(bool privateNetwork)
   {
      this->privateNetwork = privateNetwork;
      return *this;
   }

   inline bool validate() const
   {
      if (txPower < 2 || txPower > 20) {
         return false;
      }
      // Add more validation as needed
      return true;
   }

   // Assignment operator
   LoraRadioParams& operator=(const LoraRadioParams& other)
   {
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

   // Method to convert parameters to string
   inline std::string toString() const
   {
      // Convert to Hz. Some compiler versions do not support std::to_string with float
      uint32_t band_hz = band * 1000 * 1000;
      uint32_t bw_hz = bw * 1000 * 1000;

      return std::string("LoraRadioParams(ss=") + std::to_string(pinConfig.ss) +
             std::string(", rst=") + std::to_string(pinConfig.rst) + std::string(", di0=") +
             std::to_string(pinConfig.di0) + std::string(", di1=") + std::to_string(pinConfig.di1) +
             std::string(", band=") + std::to_string(band_hz) + std::string(", txPower=") +
             std::to_string(txPower) + std::string(", bw=") + std::to_string(bw_hz) +
             std::string(", sf=") + std::to_string(sf) + std::string(", gain=") +
             std::to_string(gain) + std::string(", privateNetwork=") +
             std::to_string(privateNetwork) + std::string(")");
   }

   inline bool isInitialized() const
   {
      return (pinConfig.ss != PinConfig::PIN_DEFAULT_SS &&
              pinConfig.rst != PinConfig::PIN_DEFAULT_RST &&
              pinConfig.di0 != PinConfig::PIN_DEFAULT_DI0 &&
              pinConfig.di1 != PinConfig::PIN_DEFAULT_DI1);
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
namespace LoraRadioPresets
{
const LoraRadioParams HELTEC_WIFI_LORA_32_V3(PinConfig(8, 12, 13, 14), 915.0, 20, 125.0, 8, 0,
                                             true);
const LoraRadioParams HELTEC_CUBECELL(PinConfig(35, 47, 39, 38), 915.0, 20, 125.0, 7, 0, true);
const LoraRadioParams XIAO_ESP32S3_WIO_SX1262(PinConfig(41, 42, 40, 39), 915.0, 20, 125.0, 7, 0,
                                              true);
} // namespace LoraRadioPresets
