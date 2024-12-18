/**
 * @file Definitions.h
 * @brief This file contains type definitions used in the RadioMesh project.
 */

#pragma once

#include <array>
#include <string>
#include <vector>

#include "Options.h"

/// @brief The length of a RadioMesh device ID.
#define RM_ID_LENGTH 4

/// @brief Tiny font
#define RM_FONT_TINY 1
/// @brief Small font
#define RM_FONT_SMALL 2
/// @brief Medium font
#define RM_FONT_MEDIUM 3
/// @brief Large font
#define RM_FONT_LARGE 4
/// @brief Battery font
#define RM_FONT_BATTERY 5

const std::array<byte, RM_ID_LENGTH> BROADCAST_ADDR = {0xFF, 0xFF, 0xFF, 0xFF};
/**
 * @struct DeviceBlueprint
 * @brief A map of device capabilities
 */
typedef struct
{
   /// @brief The device has a LoRa radio
   bool hasRadio;
   /// @brief The device can relay messages
   bool canRelay;
   /// @brief The device has a display
   bool hasDisplay;
   /// @brief The device uses crypto
   bool usesCrypto;
   /// @brief The device has a callback for handling received packets
   bool hasRxCallback;
   /// @brief The device has a callback for handling transmitted packets
   bool hasTxCallback;
   /// @brief The device is connected to WiFi
   bool hasWifi;
   /// @brief The device has a WiFi access point
   bool hasWifiAccessPoint;
   /// @brief The device has storage
   bool hasStorage;
} DeviceBlueprint;

/**
 * @enum MeshDeviceType
 * @brief Enumerates the possible device types in the mesh.
 */
enum MeshDeviceType
{
   /// @brief The device type is unknown.
   UNKNOWN = 0x01,
   /**
    * @brief The device is a standard node in the network.
    *
    * A standard node can send and receive messages. It can also relay messages to other nodes if
    * its relay capability is enabled.
    */
   STANDARD,
   /**
    * @brief The device is a hub node in the network.
    *
    * It can route packets to other nodes and to external networks.
    * It can also manage the network and include new devices.
    */
   HUB
};

/**
 * @enum MessageTopic
 * @brief Message Topics used internally by the protocol.
 */

enum MessageTopic : uint8_t
{
   UNUSED = 0x00,
   PING = 0x01,
   PONG = 0x02,
   ACK = 0x03,
   CMD = 0x04,
   BYE = 0x05,
   INCLUDE_REQUEST = 0x06,
   INCLUDE_RESPONSE = 0x07,
   INCLUDE_OPEN = 0x08,
   INCLUDE_CONFIRM = 0x09,
   MAX_RESERVED = 0x0F
};

/**
 * @class OledDisplayParams
 * @brief This class is used to store the parameters of the OLED display.
 */
class OledDisplayParams
{
public:
   uint8_t clockPin;
   uint8_t dataPin;
   uint8_t resetPin;
   uint8_t fontId;
   OledDisplayParams() : clockPin(0), dataPin(0), resetPin(0), fontId(RM_FONT_MEDIUM)
   {
   }

   /**
    * @brief Construct a new Oled Display Params object from the given parameters.
    * @param rotationPin The rotation pin of the display.
    * @param clockPin The clock pin of the display.
    * @param dataPin The data pin of the display.
    * @param resetPin The reset pin of the display.
    * @return A new OledDisplayParams object.
    */
   OledDisplayParams(uint8_t clockPin, uint8_t dataPin, uint8_t resetPin,
                     uint8_t font = RM_FONT_MEDIUM)
       : clockPin(clockPin), dataPin(dataPin), resetPin(resetPin), fontId(font)
   {
   }

   OledDisplayParams& operator=(const OledDisplayParams& other)
   {
      if (this != &other) {
         clockPin = other.clockPin;
         dataPin = other.dataPin;
         resetPin = other.resetPin;
         fontId = other.fontId;
      }
      return *this;
   }
};

/**
 * @struct WifiParams
 * @brief This class is used to store the parameters of the WiFi connection.
 */
struct WifiParams
{
   std::string ssid;
   std::string password;
};

struct WifiAccessPointParams
{
   std::string ssid;
   std::string password;
   std::string ipAddress;
};

/**
 * @enum SignalIndicator
 * @brief Enumerates the possible signal strength indicators.
 * - NO_SIGNAL: No signal is detected.
 * - WEAK: A weak signal is detected.
 * - FAIR: A fair signal is detected.
 * - GOOD: A good signal is detected.
 * - EXCELLENT: An excellent signal is detected.
 */
typedef enum SignalIndicator
{
   NO_SIGNAL = 0,
   WEAK,
   FAIR,
   GOOD,
   EXCELLENT
} SignalStrength;

namespace MessageTopicUtils
{

/**
 * @brief Check if a topic is reserved
 * @param topic Topic value
 * @return true if the topic is reserved, false otherwise
 */
inline bool isTopicReverved(uint8_t topic)
{
   return topic > MessageTopic::MAX_RESERVED;
}

/**
 * @brief Check if a topic is unused
 * @param topic Topic value
 * @return true if the topic is unused, false otherwise
 */
inline bool isPing(uint8_t topic)
{
   return topic == MessageTopic::PING;
}

/**
 * @brief Check if a topic is a PONG
 * @param topic Topic value
 * @return true if the topic is a PONG, false otherwise
 */
inline bool isPong(uint8_t topic)
{
   return topic == MessageTopic::PONG;
}

/**
 * @brief Check if a topic is an ACK
 * @param topic Topic value
 * @return true if the topic is an ACK, false otherwise
 */
inline bool isAck(uint8_t topic)
{
   return topic == MessageTopic::ACK;
}

/**
 * @brief Check if a topic is a CMD
 * @param topic Topic value
 * @return true if the topic is a CMD, false otherwise
 */
inline bool isCmd(uint8_t topic)
{
   return topic == MessageTopic::CMD;
}

/**
 * @brief Check if a topic is a BYE
 * @param topic Topic value
 * @return true if the topic is a BYE, false otherwise
 */
inline bool isBye(uint8_t topic)
{
   return topic == MessageTopic::BYE;
}

/**
 * @brief Check if a topic is an INCLUDE_REQUEST
 * @param topic Topic value
 * @return true if the topic is an INCLUDE_REQUEST, false otherwise
 */
inline bool isIncludeRequest(uint8_t topic)
{
   return topic == MessageTopic::INCLUDE_REQUEST;
}

/**
 * @brief Check if a topic is an INCLUDE_RESPONSE
 * @param topic Topic value
 * @return true if the topic is an INCLUDE_RESPONSE, false otherwise
 */
inline bool isIncludeResponse(uint8_t topic)
{
   return topic == MessageTopic::INCLUDE_RESPONSE;
}

/**
 * @brief Check if a topic is an INCLUDE_OPEN
 * @param topic Topic value
 * @return true if the topic is an INCLUDE_OPEN, false otherwise
 */
inline bool isIncludeOpen(uint8_t topic)
{
   return topic == MessageTopic::INCLUDE_OPEN;
}

/**
 * @brief Check if a topic is an INCLUDE_CONFIRM
 * @param topic Topic value
 * @return true if the topic is an INCLUDE_CONFIRM, false otherwise
 */
inline bool isIncludeConfirm(uint8_t topic)
{
   return topic == MessageTopic::INCLUDE_CONFIRM;
}

/**
 * @brief Convert topic value to string representation
 * @param topic Topic value
 * @return String describing the topic. If the topic is unknown, "UNKNOWN" is returned.
 */
inline std::string topicToString(uint8_t topic)
{
   switch (topic) {
   case MessageTopic::PING:
      return "PING";
   case MessageTopic::PONG:
      return "PONG";
   case MessageTopic::ACK:
      return "ACK";
   case MessageTopic::CMD:
      return "CMD";
   case MessageTopic::BYE:
      return "BYE";
   case MessageTopic::INCLUDE_REQUEST:
      return "INCLUDE_REQUEST";
   case MessageTopic::INCLUDE_RESPONSE:
      return "INCLUDE_RESPONSE";
   case MessageTopic::INCLUDE_OPEN:
      return "INCLUDE_OPEN";
   default:
      return "0x" + std::to_string(topic);
   }
}
} // namespace MessageTopicUtils

/**
 * @struct ByteStorageParams
 * @brief This structure holds parameters for byte storage configuration.
 */
struct ByteStorageParams
{
   size_t size;            // Total storage size in bytes
   bool persist;           // Whether to persist across reboots
   std::string mountPoint; // For filesystem implementations

   ByteStorageParams() : size(0), persist(true), mountPoint("")
   {
   }

   /**
    * @brief Construct a new Storage Params object
    *
    * @param size Total storage size in bytes
    * @param persist Whether to persist across reboots
    * @param mountPoint Mount point for filesystem implementations
    * @return A new ByteStorageParams object
    */
   ByteStorageParams(size_t size, bool persist = true, const std::string& mountPoint = "")
       : size(size), persist(persist), mountPoint(mountPoint)
   {
   }
};

enum class SecurityMethod
{
   NONE,
   AES,
   CUSTOM
};

/**
 * @struct SecurityParams
 * @brief This structure holds parameters for security configuration.
 */
struct SecurityParams
{
   std::vector<byte> key; // Key for encryption
   std::vector<byte> iv;  // Initialization vector for encryption
   SecurityMethod method; // Encryption method

   SecurityParams() : key(), iv(), method()
   {
   }

   /**
    * @brief Construct a new Security Params object
    *
    * @param key Key for encryption
    * @param iv Initialization vector for encryption
    * @param method Encryption method. Default is AES.
    * @return A new SecurityParams object
    */
   SecurityParams(const std::vector<byte>& key, const std::vector<byte>& iv,
                  SecurityMethod method = SecurityMethod::AES)
       : key(key), iv(iv), method(method)
   {
   }
};

/**
 * @struct CaptivePortalParams
 * @brief Configuration parameters for the captive portal
 */
struct CaptivePortalParams
{
   std::string title;     // Portal page title
   std::string indexHtml; // Main portal content
   uint16_t webPort;      // Web server port (usually 80)
   uint16_t dnsPort;      // DNS server port (usually 53)

   CaptivePortalParams() : title("RadioMesh Portal"), indexHtml(""), webPort(80), dnsPort(53)
   {
   }

   CaptivePortalParams(const std::string& title, const std::string& html, uint16_t webPort = 80,
                       uint16_t dnsPort = 53)
       : title(title), indexHtml(html), webPort(webPort), dnsPort(dnsPort)
   {
   }
};
