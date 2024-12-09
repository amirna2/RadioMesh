#pragma once

#include <vector>
#include <string>

#include <common/inc/Options.h>
#include <common/inc/RadioConfigs.h>
#include <framework/interfaces/IRadio.h>
#include <framework/interfaces/IDevice.h>
#include <framework/interfaces/IDisplay.h>
#include <framework/interfaces/ICrypto.h>
#include <framework/device/inc/Device.h>


/**
 * @class DeviceBuilder
 * @brief A builder class for creating devices
 *
*/
class DeviceBuilder
{
public:
   DeviceBuilder() {};

   /**
    * @brief Start the device builder
    * @return A reference to the builder
   */
   DeviceBuilder &start();

   /**
    * @brief Add a LoRa radio to the device with the given parameters
    * @param params The parameters for the LoRa radio
    * @return A reference to the updated builder
   */
   DeviceBuilder &withLoraRadio(const LoraRadioParams &params);

   /**
    * @brief Enable or disable the relay feature
    * @param enabled True to enable the relay, false to disable
    * @return A reference to the updated builder
   */
   DeviceBuilder &withRelayEnabled(bool enabled);

   /**
    * @brief Add a callback for received packets
    * @param callback The callback function to call when a packet is received
    * @return A reference to the updated builder
   */
   DeviceBuilder &withRxPacketCallback(PacketReceivedCallback callback);

   /**
    * @brief Add a callback for transmitted packets
    * @param callback The callback function to call when a packet is transmitted
    * @return A reference to the updated builder
    */
   DeviceBuilder &withTxPacketCallback(PacketSentCallback callback);

   /**
    * @brief Add an OLED display to the device with the given parameters
    * @param params The parameters for the OLED display
    * @return A reference to the updated builder
   */
   DeviceBuilder &withOledDisplay(const OledDisplayParams &params);

   /**
    * @brief Add a WiFi module to the device with the given parameters
    * @param params The parameters for the WiFi module
    * @return A reference to the updated builder
   */
   DeviceBuilder &withWifi(const WifiParams &params);


   /**
    * @brief Add a WiFi access point to the device with the given parameters
    * @param params The parameters for the WiFi access point
    * @return A reference to the updated builder
   */
   DeviceBuilder &withWifiAccessPoint(const WifiAccessPointParams &params);


   /**
    * @brief Add storage to the device with the given parameters
    * @param params The parameters for the storage
    * @return A reference to the updated builder
   */
   DeviceBuilder& withStorage(const ByteStorageParams& params);


   /**
    * @brief Add secure messaging to the device with the given parameters
    * @param params The parameters for the secure messaging
    * @return A reference to the updated builder
   */
   DeviceBuilder& withSecureMessaging(const SecurityParams &params);

   /**
    * @brief Add a custom display to the device
    * @param display The custom display to add
    * @return A reference to the updated builder
   */
   DeviceBuilder& withCustomDisplay(IDisplay *display);

   /**
    * @brief Build the device
    * @param name The name of the device
    * @param id The ID of the device
    * @return A pointer to the built device
   */
   IDevice *build(const std::string name, std::array<byte, RM_ID_LENGTH> id, MeshDeviceType deviceType = MeshDeviceType::UNKNOWN);

   /**
    * @brief Get the blueprint of the device
    * @return The blueprint of the device
   */
   DeviceBlueprint getBlueprint() {
      return blueprint;
   }

private:


   bool isBuilderStarted = false;

   DeviceBlueprint blueprint;

   // Device parameters
   LoraRadioParams radioParams;
   SecurityParams securityParams;
   OledDisplayParams oledDisplayParams;
   WifiParams wifiParams = WifiParams();
   WifiAccessPointParams wifiAPParams = WifiAccessPointParams();
   ByteStorageParams storageParams;

   bool relayEnabled = false;
   PacketReceivedCallback rxCallback = nullptr;
   PacketSentCallback txCallback = nullptr;
   IDisplay* customDisplay = nullptr;
   bool useCustomDisplay = false;

   void destroyDevice(IDevice *device) {
      if (device != nullptr) {
         delete device;
      }
   }

};
