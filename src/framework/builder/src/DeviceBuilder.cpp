#include <framework/builder/inc/DeviceBuilder.h>
#include <common/inc/Logger.h>
#include <common/inc/Errors.h>

DeviceBuilder &DeviceBuilder::start()
{
   blueprint = {
      .hasRadio = false,
      .canRelay = false,
      .hasDisplay = false,
      .usesCrypto = false,
      .hasRxCallback = false,
      .hasTxCallback = false,
      .hasWifi = false,
      .hasWifiAccessPoint = false,
      .hasStorage = false
   };
   relayEnabled = false;
   isBuilderStarted = true;

   // Initialize the serial port here since we need it for builder lo
   // This gives us 10 seconds to do a hard reset if the board is in a bad state after power cycle
   while (!Serial && millis() < 10000)
      ;
   Serial.begin(115200);
   loginfo_ln("Running RadioMesh %s",RadioMeshUtils::getVersion().c_str());
   loginfo_ln("DeviceBuilder started...");

   return *this;
}

DeviceBuilder &DeviceBuilder::withRxPacketCallback(PacketReceivedCallback callback)
{
   loginfo_ln("Setting Rx callback %p", callback);
   blueprint.hasRxCallback = true;
   rxCallback = callback;
   return *this;
}

DeviceBuilder &DeviceBuilder::withTxPacketCallback(PacketSentCallback callback)
{
   loginfo_ln("Setting Tx callback %p", callback);
   blueprint.hasTxCallback = true;
   txCallback = callback;
   return *this;
}

DeviceBuilder &DeviceBuilder::withLoraRadio(const LoraRadioParams &params)
{
   loginfo_ln("Setting LoRa radio params: %s", params.toString().c_str());
   blueprint.hasRadio = true;
   this->radioParams = params;
   return *this;
}

DeviceBuilder &DeviceBuilder::withRelayEnabled(bool enabled)
{
   loginfo_ln("Setting relay enabled: %d", enabled);
   blueprint.canRelay = true;
   relayEnabled = enabled;
   return *this;
}

DeviceBuilder &DeviceBuilder::withSecureMessaging(const SecurityParams &params)
{
   loginfo_ln("Setting secure messaging params");
   blueprint.usesCrypto = true;
   securityParams.method = params.method;
   securityParams.iv.assign(params.iv.begin(), params.iv.end());
   securityParams.key.assign(params.key.begin(), params.key.end());
   return *this;
}

DeviceBuilder &DeviceBuilder::withOledDisplay(const OledDisplayParams &params)
{
   loginfo_ln("Setting OLED display params");
   blueprint.hasDisplay = true;
   oledDisplayParams = params;
   return *this;
}

DeviceBuilder &DeviceBuilder::withWifi(const WifiParams &params)
{
   loginfo_ln("Setting WiFi params");
   blueprint.hasWifi = true;
   wifiParams.password = params.password;
   wifiParams.ssid = params.ssid;
   return *this;
}

DeviceBuilder &DeviceBuilder::withWifiAccessPoint(const WifiAccessPointParams &params)
{
   loginfo_ln("Setting WiFi access point params");
   blueprint.hasWifiAccessPoint = true;
   wifiAPParams.password = params.password;
   wifiAPParams.ssid = params.ssid;
   wifiAPParams.ipAddress = params.ipAddress;
   return *this;
}

DeviceBuilder &DeviceBuilder::withStorage(const ByteStorageParams& params)
{
   loginfo_ln("Setting storage params");
   blueprint.hasStorage = true;
   storageParams = params;
   return *this;
}

IDevice *DeviceBuilder::build(const std::string name, std::array<byte, RM_ID_LENGTH> id, MeshDeviceType deviceType)
{
   int build_error = RM_E_NONE;
   loginfo_ln("Building device...");
   if (!isBuilderStarted) {
      logerr_ln("ERROR: DeviceBuilder not started. Call start() first.");
      return nullptr;
   }

   // Create the Device object using setters as needed as per the blueprint
   RadioMeshDevice *device = new RadioMeshDevice(name, id);

   build_error = device->initialize();
   if (build_error != RM_E_NONE) {
      logerr_ln("ERROR: Device initialization failed. [%d]", build_error);
      destroyDevice(device);
      return nullptr;
   }
   logdbg_ln("Device initialized.");

   device->setDeviceType(deviceType);
   logdbg_ln("Device type set.");

   if (blueprint.hasRxCallback) {
      if (rxCallback == nullptr) {
         logerr_ln("ERROR: Rx callback is cannot be null.");
         destroyDevice(device);
         return nullptr;
      }
      device->registerCallback(rxCallback);
      logdbg_ln("Rx callback set.");
   }

   if (blueprint.hasTxCallback) {
      if (txCallback == nullptr) {
         logerr_ln("ERROR: Rx callback is cannot be null.");
         destroyDevice(device);
         return nullptr;
      }
      device->registerTxCallback(txCallback);
      logdbg_ln("Tx callback set.");
   }

   if (blueprint.hasRadio) {
      build_error = device->initializeRadio(radioParams);
      if (build_error != RM_E_NONE) {
         logerr_ln("ERROR: LoRa radio initialization failed. [%d]", build_error);
         destroyDevice(device);
         return nullptr;
      }
      logdbg_ln("Radio initialized.");
   }

   if (blueprint.canRelay) {
      device->enableRelay(relayEnabled);
      logdbg_ln("Relay enabled.");
   }

   if (blueprint.usesCrypto) {
      build_error = device->initializeAesCrypto(securityParams);
      if (build_error != RM_E_NONE) {
         logerr_ln("ERROR: Aes crypto initialization failed. [%d]", build_error);
         destroyDevice(device);
         return nullptr;
      }
      logdbg_ln("Crypto initialized.");
   }

   if (blueprint.hasDisplay) {
      build_error = device->initializeOledDisplay(oledDisplayParams);
      if (build_error != RM_E_NONE) {
         logerr_ln("ERROR: Failed to create display [%d]", build_error);
         destroyDevice(device);
         return nullptr;
      }
      logdbg_ln("Display initialized.");
   }

   if (blueprint.hasWifi) {
      build_error = device->initializeWifi(wifiParams);
      if (build_error != RM_E_NONE) {
         logerr_ln("ERROR: Failed to create wifi [%d]", build_error);
         destroyDevice(device);
         return nullptr;
      }
      logdbg_ln("Wifi initialized.");
   }

   if (blueprint.hasWifiAccessPoint) {
      build_error = device->initializeWifiAccessPoint(wifiAPParams);
      if (build_error != RM_E_NONE) {
         logerr_ln("ERROR: Failed to create wifi access point [%d]", build_error);
         destroyDevice(device);
         return nullptr;
      }
      logdbg_ln("Wifi access point initialized.");
   }

   if (blueprint.hasStorage) {
      build_error = device->initializeStorage(storageParams);
      if (build_error != RM_E_NONE) {
         logerr_ln("ERROR: Failed to create storage [%d]", build_error);
         destroyDevice(device);
         return nullptr;
      }
      logdbg_ln("Storage initialized.");
   }
   loginfo_ln("Device [%s] built successfully.", device->getDeviceName().c_str());
   isBuilderStarted = false;
   return device;
}
