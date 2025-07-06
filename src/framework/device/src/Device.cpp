#include <string>
#include <vector>

#include <common/utils/RadioMeshCrc32.h>
#include <core/protocol/inc/routing/RoutingTable.h>
#include <framework/device/inc/Device.h>

RadioMeshDevice::RadioMeshDevice(const std::string& name, const std::array<byte, RM_ID_LENGTH>& id,
                                 MeshDeviceType type)
    : name(name), id(id), deviceType(type)
{
    // InclusionController will be created in initialize() after storage is set up
}

bool RadioMeshDevice::isIncluded() const
{
    return inclusionController->getState() == DeviceInclusionState::INCLUDED;
}

bool RadioMeshDevice::canSendMessage(uint8_t topic) const
{
    return inclusionController->canSendMessage(topic);
}

bool RadioMeshDevice::isInclusionMessage(uint8_t topic) const
{
    return (topic >= INCLUDE_REQUEST && topic <= INCLUDE_SUCCESS);
}

std::array<byte, RM_ID_LENGTH> RadioMeshDevice::getDeviceId()
{
    return id;
}

std::string RadioMeshDevice::getDeviceName()
{
    return name;
}

void RadioMeshDevice::setDeviceType(MeshDeviceType type)
{
    deviceType = type;
}

int RadioMeshDevice::initializeRadio(LoraRadioParams radioParams)
{
    int rc = RM_E_NONE;
    radio = LoraRadio::getInstance();

    if (radio == nullptr) {
        logerr_ln("Failed to get Lora radio instance");
        return RM_E_UNKNOWN;
    }

    rc = radio->setParams(radioParams);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to set radio params");
        radio = nullptr;
    }
    return rc;
}

int RadioMeshDevice::initializeAesCrypto(const SecurityParams& securityParams)
{
    crypto = AesCrypto::getInstance();
    if (crypto == nullptr) {
        logerr_ln("Failed to create crypto");
        return RM_E_UNKNOWN;
    }

    crypto->setParams(securityParams);

    router->setCrypto(crypto);
    return RM_E_NONE;
}

#ifdef RM_NO_DISPLAY
int RadioMeshDevice::initializeOledDisplay(OledDisplayParams displayParams)
{
    logwarn_ln("The device does not support OLED display.");
    return RM_E_NONE;
}

IDisplay* RadioMeshDevice::getDisplay()
{
    logwarn_ln("The device does not support OLED display.");
    return nullptr;
}
int RadioMeshDevice::setCustomDisplay(IDisplay* display)
{
    logwarn_ln("The device does not support OLED display.");
    return RM_E_NONE;
}
#else

int RadioMeshDevice::setCustomDisplay(IDisplay* display)
{
    if (display == nullptr) {
        logerr_ln("Custom display cannot be null");
        return RM_E_INVALID_PARAM;
    }
    this->customDisplay = display; // Use the provided custom display
    return RM_E_NONE;
}

int RadioMeshDevice::initializeOledDisplay(OledDisplayParams displayParams)
{
    int rc = RM_E_NONE;

    oledDisplay = OledDisplay::getInstance();

    if (oledDisplay == nullptr) {
        logerr_ln("Failed to create display");
        return RM_E_UNKNOWN;
    }

    rc = oledDisplay->setParams(displayParams);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to set display params");
        oledDisplay = nullptr;
    }
    return rc;
}

IDisplay* RadioMeshDevice::getDisplay()
{
    if (customDisplay != nullptr) {
        return customDisplay;
    }
    return oledDisplay;
}
#endif // RM_NO_DISPLAY

#ifdef RM_NO_WIFI
int RadioMeshDevice::initializeWifi(WifiParams wifiParams)
{
    logwarn_ln("The device does not support Wifi.");
    return RM_E_NONE;
}

IWifiConnector* RadioMeshDevice::getWifiConnector()
{
    logwarn_ln("The device does not support Wifi.");
    return nullptr;
}
#else
int RadioMeshDevice::initializeWifi(WifiParams wifiParams)
{
    int rc = RM_E_NONE;
    wifiConnector = WifiConnector::getInstance();

    if (wifiConnector == nullptr) {
        logerr_ln("Failed to create wifi");
        return RM_E_UNKNOWN;
    }

    rc = wifiConnector->setParams(wifiParams);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to set wifi params");
        wifiConnector = nullptr;
    }
    return rc;
}

IWifiConnector* RadioMeshDevice::getWifiConnector()
{
    return wifiConnector;
}
#endif // RM_NO_WIFI

#ifdef RM_NO_WIFI
int RadioMeshDevice::initializeWifiAccessPoint(WifiAccessPointParams wifiAPParams)
{
    logwarn_ln("The device does not support Wifi Access Point.");
    return RM_E_NONE;
}
IWifiAccessPoint* RadioMeshDevice::getWifiAccessPoint()
{
    logwarn_ln("The device does not support Wifi Access Point.");
    return nullptr;
}
int RadioMeshDevice::initializeDevicePortal(DevicePortalParams devicePortalParams)
{
    logwarn_ln("The device does not support Device Portal.");
    return RM_E_NONE;
}
IDevicePortal* RadioMeshDevice::getDevicePortal()
{
    logwarn_ln("The device does not support Device Portal.");
    return nullptr;
}
#else
int RadioMeshDevice::initializeWifiAccessPoint(WifiAccessPointParams wifiAPParams)
{
    int rc = RM_E_NONE;
    wifiAccessPoint = WifiAccessPoint::getInstance();

    if (wifiAccessPoint == nullptr) {
        logerr_ln("Failed to create wifi access point");
        return RM_E_UNKNOWN;
    }

    rc = wifiAccessPoint->setParams(wifiAPParams);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to set wifi access point params");
        wifiAccessPoint = nullptr;
    }
    return rc;
}

IWifiAccessPoint* RadioMeshDevice::getWifiAccessPoint()
{
    return wifiAccessPoint;
}

int RadioMeshDevice::initializeDevicePortal(DevicePortalParams devicePortalParams)
{
    int rc = RM_E_NONE;
    devicePortal = AsyncDevicePortal::getInstance();

    if (devicePortal == nullptr) {
        logerr_ln("Failed to create device portal");
        return RM_E_UNKNOWN;
    }

    rc = devicePortal->setParams(devicePortalParams);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to set device portal params");
        devicePortal = nullptr;
    }
    return rc;
}

IDevicePortal* RadioMeshDevice::getDevicePortal()
{
    return devicePortal;
}

#endif // RM_NO_WIFI

IRadio* RadioMeshDevice::getRadio()
{
    return radio;
}

IAesCrypto* RadioMeshDevice::getCrypto()
{
    return crypto;
}

IByteStorage* RadioMeshDevice::getByteStorage()
{
    return eepromStorage;
}

int RadioMeshDevice::sendData(const uint8_t topic, const std::vector<byte> data,
                              std::array<byte, RM_ID_LENGTH> target)
{
    if (!canSendMessage(topic)) {
        logerr_ln("Device not authorized to send messages");
        return RM_E_DEVICE_NOT_INCLUDED;
    }

    if (target.size() != DEV_ID_LENGTH) {
        logerr_ln("Invalid target device ID length: %d, expected: %d", target.size(),
                  DEV_ID_LENGTH);
        return RM_E_INVALID_LENGTH;
    }
    if (this->id.size() != DEV_ID_LENGTH) {
        logerr_ln("Device ID not properly initialized");
        return RM_E_INVALID_LENGTH;
    }

    if (data.size() > MAX_DATA_LENGTH) {
        logerr_ln("Data too large: %d bytes, maximum: %d", data.size(), MAX_DATA_LENGTH);
        return RM_E_PACKET_TOO_LONG;
    }

    txPacket.reset();
    txPacket.topic = topic;
    txPacket.sourceDevId = this->id;
    txPacket.destDevId = target;
    txPacket.deviceType = this->deviceType;
    txPacket.packetId = RadioMeshUtils::getRandomBytesArray<MSG_ID_LENGTH>();
    txPacket.hopCount = 0;
    txPacket.fcounter = ++packetCounter;
    txPacket.lastHopId = this->id;
    txPacket.nextHopId = BROADCAST_ADDR;
    txPacket.packetData = data;

    return router->routePacket(txPacket, this->id.data());
}

bool RadioMeshDevice::isReceivedDataCrcValid(RadioMeshPacket& receivedPacket)
{
    RadioMeshUtils::CRC32 crc32;
    crc32.update(receivedPacket.fcounter);
    crc32.update(receivedPacket.packetData.data(), receivedPacket.packetData.size());
    uint32_t computed_data_crc = crc32.finalize();
    crc32.reset();

    if (computed_data_crc != receivedPacket.packetCrc) {
        logerr_ln("ERROR data crc mismatch: received: 0x%X, calculated: 0x%X",
                  receivedPacket.packetCrc, computed_data_crc);
        return false;
    }
    return true;
}

int RadioMeshDevice::handleReceivedData()
{
    std::vector<byte> dataBytes;

    logtrace_ln("handleReceivedPacket() START...");

    // Read the received data from the radio and create a RadioMeshPacket object
    int rc = radio->readReceivedData(&dataBytes);

    if (rc != RM_E_NONE) {
        logerr_ln("ERROR handleReceivedPacket. Failed to get data. rc = %d", rc);
        return rc;
    }

    RadioMeshPacket receivedPacket = RadioMeshPacket(dataBytes);
    receivedPacket.log();

    // skip already seen packets
    if (router->isPacketFoundInTracker(receivedPacket)) {
        logwarn_ln("Packet already seen. Ignoring...");
        return RM_E_NONE;
    }

    if (!isReceivedDataCrcValid(receivedPacket)) {
        logerr_ln("ERROR handleReceivedPacket. Data CRC mismatch");
        return RM_E_PACKET_CORRUPTED;
    }

    // Update routing table with information from received packet
    // We do this for all valid packets, even if they're for us
    int lastRssi = radio->getRSSI();
    RoutingTable::getInstance()->updateRoute(receivedPacket, lastRssi);
    logdbg_ln(
        "Updated route table for source: %s, last hop: %s, RSSI: %d",
        RadioMeshUtils::convertToHex(receivedPacket.sourceDevId.data(), DEV_ID_LENGTH).c_str(),
        RadioMeshUtils::convertToHex(receivedPacket.lastHopId.data(), DEV_ID_LENGTH).c_str(),
        lastRssi);

    // Check if this is an inclusion message and handle it automatically
    if (isInclusionMessage(receivedPacket.topic)) {
        logdbg_ln("Received inclusion message with topic: 0x%02X", receivedPacket.topic);

        // Let the InclusionController handle it automatically
        int result = inclusionController->handleInclusionMessage(receivedPacket);
        if (result != RM_E_NONE) {
            logerr_ln("Failed to handle inclusion message: %d", result);
        }

        // Still notify application for monitoring if callback is set
        if (onPacketReceived != nullptr) {
            logdbg_ln("Notifying application about inclusion message");
            onPacketReceived(&receivedPacket, RM_E_NONE);
        }

        // Don't forward inclusion messages
        return result;
    }

    // Packet has reached its destination or the device is a HUB, let the application handle it
    if (onPacketReceived != nullptr) {
        logdbg_ln("Calling onPacketReceived callback");
        onPacketReceived(&receivedPacket, RM_E_NONE);
    }

    // The hub is a final destination for all packets and also if the packet is for this device
    if (this->deviceType == MeshDeviceType::HUB || receivedPacket.destDevId == this->id) {
        logtrace_ln("handleReceivedPacket() DONE!");
        return RM_E_NONE;
    }
    // Only a standard device with relay enabled should route the packet
    if (this->deviceType == MeshDeviceType::STANDARD && relayEnabled) {
        loginfo_ln("Router device. Routing received packet...");
        rc = router->routePacket(receivedPacket, this->id.data());
        if (rc != RM_E_NONE) {
            logerr_ln("ERROR handleReceivedPacket. Failed to route packet. rc = %d", rc);
            return rc;
        }
    }
    logtrace_ln("handleReceivedPacket() DONE!");

    return RM_E_NONE;
}

void RadioMeshDevice::enableRelay(bool enabled)
{
    relayEnabled = enabled;
}

bool RadioMeshDevice::isRelayEnabled()
{
    return relayEnabled;
}

int RadioMeshDevice::run()
{
    inclusionController->checkProtocolTimeouts();

    // handle radio Rx/Tx events
    if (radio->checkAndClearRxFlag()) {
        logtrace_ln("Packet RX done");
        int radioErr = radio->getRadioStateError();
        if (radioErr != RM_E_NONE) {
            logerr_ln("ERROR radio RX failed with rc = %d", radioErr);
            if (onPacketReceived != nullptr) {
                onPacketReceived(nullptr, radioErr);
            }
            return radioErr;
        }

        // Handle received data and invoke the callback with the received packet
        int rc = handleReceivedData();
        // Report error if any
        if (rc != RM_E_NONE) {
            logerr_ln("ERROR handleReceivedData failed with rc = %d", rc);
            if (onPacketReceived != nullptr) {
                onPacketReceived(nullptr, rc);
            }
            return rc;
        }
    }
    if (radio->checkAndClearTxFlag()) {
        logtrace_ln("Packet TX done");
        int radioErr = radio->getRadioStateError();
        if (onPacketSent != nullptr) {
            logdbg_ln("Calling onPacketSent callback");
            onPacketSent(&txPacket, radioErr);
        }
        radio->startReceive();
    }

    return RM_E_NONE;
}

int RadioMeshDevice::sendInclusionOpen()
{
    return inclusionController->sendInclusionOpen();
}

int RadioMeshDevice::sendInclusionRequest()
{
    return inclusionController->sendInclusionRequest();
}

int RadioMeshDevice::sendInclusionResponse(const RadioMeshPacket& packet)
{
    return inclusionController->sendInclusionResponse(packet);
}

int RadioMeshDevice::sendInclusionConfirm()
{
    return inclusionController->sendInclusionConfirm();
}

int RadioMeshDevice::sendInclusionSuccess()
{
    return inclusionController->sendInclusionSuccess();
}

int RadioMeshDevice::enableInclusionMode(bool enable)
{
    if (enable) {
        return inclusionController->enterInclusionMode();
    } else {
        return inclusionController->exitInclusionMode();
    }
}

bool RadioMeshDevice::isInclusionModeEnabled() const
{
    return inclusionController->isInclusionModeEnabled();
}

int RadioMeshDevice::initialize()
{
    int rc = RM_E_NONE;
    eepromStorage = EEPROMStorage::getInstance();

    if (eepromStorage == nullptr) {
        logerr_ln("Failed to get storage instance");
        return RM_E_DEVICE_INITIALIZATION_FAILED;
    }

    ByteStorageParams defaultParams(EEPROM_STORAGE_MAX_SIZE);
    eepromStorage->setParams(defaultParams);
    rc = eepromStorage->begin();

    if (rc != RM_E_NONE) {
        logerr_ln("Failed to initialize storage");
        eepromStorage = nullptr;
        return rc;
    }

    inclusionController = std::make_unique<InclusionController>(*this);
    
    if (inclusionController->getState() == DeviceInclusionState::INCLUDED) {
        rc = inclusionController->loadAndApplySessionKey();
        if (rc != RM_E_NONE) {
            logwarn_ln("Failed to load session key, device may need re-inclusion: %d", rc);
        }
    }
    
    return RM_E_NONE;
}

int RadioMeshDevice::factoryReset()
{
    if (eepromStorage == nullptr) {
        logerr_ln("No storage available for factory reset");
        return RM_E_UNKNOWN;
    }

    loginfo_ln("Performing factory reset - clearing all stored state");

    // Clear all storage
    int rc = eepromStorage->clear();
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to clear storage: %d", rc);
        return rc;
    }

    // Reset frame counter
    packetCounter = 0;

    // Recreate inclusion controller to reset its state
    if (inclusionController != nullptr) {
        inclusionController = std::make_unique<InclusionController>(*this);
    }

    loginfo_ln("Factory reset complete");
    return RM_E_NONE;
}

int RadioMeshDevice::updateSecurityParams(const SecurityParams& params)
{
    if (crypto == nullptr) {
        loginfo_ln("Crypto not initialized, creating AesCrypto instance for dynamic security");
        crypto = AesCrypto::getInstance();
        if (crypto == nullptr) {
            logerr_ln("Failed to create AesCrypto instance");
            return RM_E_UNKNOWN;
        }
    }

    loginfo_ln("Updating device security parameters");
    
    crypto->setParams(params);
    router->setCrypto(crypto);
    
    loginfo_ln("Security parameters updated successfully");
    return RM_E_NONE;
}
