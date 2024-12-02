#pragma once

#include <vector>
#include <string>
#include <memory>

#include <common/inc/Definitions.h>
#include <framework/interfaces/IDevice.h>
#include <core/protocol/inc/routing/PacketRouter.h>
#include <hardware/inc/radio/LoraRadio.h>
#include <hardware/inc/storage/eeprom/EEPROMStorage.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <core/protocol/inc/packet/Callbacks.h>
#include <common/inc/Errors.h>

#ifndef RM_NO_WIFI
#include <hardware/inc/wifi/WifiConnector.h>
#include <hardware/inc/wifi/WifiAccessPoint.h>
#endif

#ifndef RM_NO_DISPLAY
#include <hardware/inc/display/oled/OledDisplay.h>
#endif

class RadioMeshDevice : public IDevice
{
public:

   RadioMeshDevice(const std::string& name, const std::array<byte, RM_ID_LENGTH>& id)
      : name(name), id(id) {}

   virtual ~RadioMeshDevice() {
      radio = nullptr;
      crypto = nullptr;
#ifndef RM_NO_WIFI
      wifiConnector = nullptr;
      wifiAccessPoint = nullptr;
#endif
#ifndef RM_NO_DISPLAY
      display = nullptr;
#endif
      }

   // IDevice interface

   // Components
   IRadio *getRadio() override;
   IAesCrypto *getCrypto() override;
   IDisplay *getDisplay() override;
   IWifiConnector *getWifiConnector() override;
   IWifiAccessPoint *getWifiAccessPoint() override;
   IByteStorage *getByteStorage() override;

   int sendData(const uint8_t topic, const std::vector<byte> data, std::array<byte, RM_ID_LENGTH> target = BROADCAST_ADDR) override;
   void enableRelay(bool enabled) override;
   bool isRelayEnabled() override;
   int run() override;
   std::string getDeviceName() override;
   std::array<byte, RM_ID_LENGTH> getDeviceId() override;
   void setDeviceType(MeshDeviceType type) override;

   int enableInclusionMode(bool enable) override;
   int sendInclusionOpen() override;
   int sendInclusionRequest(const std::vector<byte>& publicKey, uint32_t initialCounter) override;

   // Device specific methods

   /**
    * @brief Get the LoRa Radio Params object
    *
    * @return LoraRadioParams
    */
   LoraRadioParams getLoRaRadioParams() { return radioParams; };

   /**
    * @brief Get the device type
    *
    * @return DeviceType
    */
   MeshDeviceType getDeviceType() { return deviceType; }

   /**
    * @brief Initialize the device
    *
    * @return RM_E_NONE if the device was successfully initialized, an error code otherwise.
    */
   int initialize();

   /**
    * @brief Register a callback function for when a packet is received
    *
    * @param callback  Callback function to register
    */
   void registerCallback(PacketReceivedCallback callback) { this->onPacketReceived = callback; }

   /**
    * @brief Register a Tx callback function for when a packet is received
    *
    * @param callback  Callback function to register
    */
   void registerTxCallback(PacketSentCallback callback) { this->onPacketSent = callback; }

   /**
    * @brief Initialize the radio with the given parameters
    *
    * @param radioParams LoraRadioParams object containing the radio parameters
    * @return RM_E_NONE if the radio was successfully initialized, an error code otherwise.
    */
   int initializeRadio(LoraRadioParams radioParams);

   /**
    * @brief Initialize the OLED display with the given parameters
    *
    * @param displayParams OledDisplayParams object containing the display parameters
    * @return RM_E_NONE if the display was successfully initialized, an error code otherwise.
    */
   int initializeOledDisplay(OledDisplayParams displayParams);

   /**
    * @brief Initialize the crypto component
    *
    * @return int RM_E_NONE if the crypto component was successfully initialized, an error code otherwise.
    */
   int initializeAesCrypto(std::vector<byte> key, std::vector<byte> iv);

   /**
    * @brief Initialize the WiFi module with the given parameters
    *
    * @param wifiParams WifiParams object containing the WiFi parameters
    * @return int RM_E_NONE if the WiFi module was successfully initialized, an error code otherwise.
    */
   int initializeWifi(WifiParams wifiParams);

   /**
    * @brief Initialize the WiFi access point with the given parameters
    *
    * @param wifiAPParams WifiAccessPointParams object containing the WiFi access point parameters
    * @return int RM_E_NONE if the WiFi access point was successfully initialized, an error code otherwise.
    */
   int initializeWifiAccessPoint(WifiAccessPointParams wifiAPParams);

   /**
    * @brief Initialize the storage component with the given parameters
    *
    * @param storageParams ByteStorageParams object containing the storage parameters
    * @return int RM_E_NONE if the storage component was successfully initialized, an error code otherwise.
    */
   int initializeStorage(ByteStorageParams storageParams);

   /**
    * @brief Handle received data
    *
    * @return int RM_E_NONE if the data was successfully handled, an error code otherwise.
    */
   int handleReceivedData();


private:

   std::string name;
   std::array<byte, RM_ID_LENGTH> id;
   DeviceBlueprint blueprint;
   uint32_t packetCounter = 0;

   LoraRadio* radio = nullptr;
   AesCrypto* crypto = nullptr;
   EEPROMStorage* eepromStorage = nullptr;

#ifndef RM_NO_DISPLAY
   OledDisplay* display = nullptr;
#endif

#ifndef RM_NO_WIFI
   WifiConnector* wifiConnector = nullptr;
   WifiAccessPoint* wifiAccessPoint = nullptr;
#endif

   LoraRadioParams radioParams = LoraRadioParams();
   WifiParams wifiParams = WifiParams();
   WifiAccessPointParams wifiAPParams = WifiAccessPointParams();

   bool relayEnabled = false;

   MeshDeviceType deviceType = MeshDeviceType::UNKNOWN;
   PacketReceivedCallback onPacketReceived = nullptr;
   PacketSentCallback onPacketSent = nullptr;
   PacketRouter *router = PacketRouter::getInstance();

   bool isReceivedDataCrcValid(RadioMeshPacket &receivedPacket);

   // Inclusion mode
   enum class HubMode {
    NORMAL,
    INCLUSION    // Hub is accepting new devices
   };

   HubMode hubMode = HubMode::NORMAL;

   bool deviceIncluded = false;

   RadioMeshPacket txPacket = RadioMeshPacket();

   bool isIncluded() { return deviceIncluded; }

};
