/**
 * @file MiniHub.ino
 * @brief Hub Device Automatic Inclusion Example
 *
 * This example demonstrates the automatic inclusion protocol where the
 * InclusionController handles all inclusion logic automatically. The application
 * simply enables inclusion mode and monitors the process.
 */
#include "admin_panel.h"
#include "device_info.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <RadioMesh.h>
#include <map>

// Application states (much simpler than before)
enum class AppState
{
    INITIALIZING,
    READY,
    INCLUSION_MODE_ACTIVE,
    ERROR
};

// Constants
const unsigned long STATUS_UPDATE_INTERVAL = 2000;   // Update display every 2 seconds

// Global variables
IDevice* device = nullptr;
IDisplay* display = nullptr;
IRadio* radio = nullptr;
IWifiConnector* wifiConnector = nullptr;
IWifiAccessPoint* wifiAP = nullptr;

AppState appState = AppState::INITIALIZING;
unsigned long lastStatusUpdate = 0;
bool inclusionModeActive = false;

// Counters for monitoring
int totalInclusionRequests = 0;
int successfulInclusions = 0;

// Device tracking for admin panel
std::map<std::string, DeviceInfo> connectedDevicesMap;

bool setupWifiConnector()
{
    wifiConnector = device->getWifiConnector();
    if (wifiConnector == nullptr) {
        logerr_ln("ERROR  wifiConnector is null");
        return false;
    } else {
        // Setup the wifi connector with the built-in configuration
        int retries = 0;
        while (wifiConnector->connect() != RM_E_NONE) {
            logerr_ln("ERROR: WifiConnector setup failed.");
            retries++;
            if (retries > 3) {
                return false;
            }
        }
    }
    std::string ip = wifiConnector->getIpAddress();
    loginfo_ln("setupWifiConnector: IP Address: %s", ip.c_str());
    return true;
}

bool setupAccessPoint()
{
    wifiAP = device->getWifiAccessPoint();
    if (wifiAP == nullptr) {
        logerr_ln("ERROR  wifiAP is null");
        return false;
    } else {
        // Setup the wifi access point with the built-in configuration
        if (wifiAP->setup() != RM_E_NONE) {
            logerr_ln("ERROR: WifiAP setup failed.");
            return false;
        }
        if (wifiAP->start() != RM_E_NONE) {
            logerr_ln("ERROR: WifiAP start failed.");
            return false;
        }
    }
    loginfo_ln("setupAccessPoint: AP started");
    return true;
}

bool setupCaptivePortal()
{
    ICaptivePortal* captivePortal = device->getCaptivePortal();
    if (captivePortal == nullptr) {
        logerr_ln("ERROR captivePortal is null");
        return false;
    } else {
        // Start the captive portal web server
        if (captivePortal->start() != RM_E_NONE) {
            logerr_ln("ERROR: CaptivePortal start failed.");
            return false;
        }
        loginfo_ln("setupCaptivePortal: Portal started on AP IP: %s",
                   WiFi.softAPIP().toString().c_str());
    }
    return true;
}

void displayStatus()
{
    // Status shown via serial log on Xiao board (no display)
    switch (appState) {
    case AppState::INITIALIZING:
        loginfo_ln("Status: Initializing...");
        break;

    case AppState::READY: {
        auto deviceId = device->getDeviceId();
        char deviceIdStr[16];
        snprintf(deviceIdStr, sizeof(deviceIdStr), "%02X%02X%02X%02X", 
                 deviceId[0], deviceId[1], deviceId[2], deviceId[3]);
        loginfo_ln("Status: Hub Ready - Device ID: %s, Connected Devices: %d",
                   deviceIdStr, connectedDevicesMap.size());
        break;
    }

    case AppState::INCLUSION_MODE_ACTIVE:
        loginfo_ln("Status: Inclusion Active - Requests: %d, Success: %d",
                   totalInclusionRequests, successfulInclusions);
        break;

    case AppState::ERROR:
        loginfo_ln("Status: ERROR - Check logs");
        break;
    }
}

/**
 * Callback for monitoring inclusion messages and tracking devices
 */
void onPacketReceived(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Failed [%d]", err);
        return;
    }

    // Convert device ID to string for tracking
    char deviceIdStr[16];
    snprintf(deviceIdStr, sizeof(deviceIdStr), "%02X%02X%02X%02X", packet->sourceDevId[0],
             packet->sourceDevId[1], packet->sourceDevId[2], packet->sourceDevId[3]);
    std::string deviceId(deviceIdStr);

    // Monitor inclusion messages for statistics and web UI
    switch (packet->topic) {
    case MessageTopic::INCLUDE_REQUEST:
        totalInclusionRequests++;
        loginfo_ln("Received inclusion request from device %s (total: %d)", deviceId.c_str(),
                   totalInclusionRequests);
        sendInclusionEvent("request_received", deviceId);
        break;

    case MessageTopic::INCLUDE_CONFIRM: {
        // When hub receives INCLUDE_CONFIRM, it sends INCLUDE_SUCCESS to complete the protocol
        successfulInclusions++;
        loginfo_ln("Received inclusion confirmation from device %s - inclusion completed "
                   "successfully (total: %d)",
                   deviceId.c_str(), successfulInclusions);

        // Add device to tracking
        DeviceInfo newDevice;
        newDevice.id = deviceId;
        newDevice.name = "Device_" + deviceId.substr(4, 4); // Use last 4 chars for short name
        newDevice.lastSeen = millis();
        newDevice.rssi = radio ? radio->getRSSI() : -100;
        connectedDevicesMap[deviceId] = newDevice;

        sendInclusionEvent("success", deviceId);

        // Send updated device list to web clients
        std::vector<byte> emptyData;
        handleGetDevices(nullptr, emptyData);

        // Stop inclusion mode after successful inclusion
        stopInclusionMode();
        break;
    }

    default:
        // Handle other application messages and update device last seen
        if (connectedDevicesMap.find(deviceId) != connectedDevicesMap.end()) {
            connectedDevicesMap[deviceId].lastSeen = millis();
            connectedDevicesMap[deviceId].rssi = radio ? radio->getRSSI() : -100;
        }
        loginfo_ln("Received application message from %s, topic: 0x%02X", deviceId.c_str(),
                   packet->topic);
        break;
    }
}

bool initializeHardware()
{
    loginfo_ln("Initializing hub device...");

    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262)
                 .withWifiAccessPoint(apParams)
                 .withCaptivePortal(portalParams)
                 .withRxPacketCallback(onPacketReceived)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::HUB);

    if (!device) {
        logerr_ln("Failed to create device");
        return false;
    }

    radio = device->getRadio();
    display = device->getDisplay();

    if (!radio) {
        logerr_ln("Failed to get radio instance");
        return false;
    }

    // Setup individual components
    int rc = radio->setup();
    if (rc != RM_E_NONE) {
        logerr_ln("Radio setup failed: %d", rc);
        return false;
    }

    //    if (!setupWifiConnector()) {
    //        logerr_ln("ERROR  wifi setup failed");
    //        return false;
    //    }

    if (!setupAccessPoint()) {
        logerr_ln("ERROR  wifiAP setup failed");
        return false;
    }

    if (!setupCaptivePortal()) {
        logerr_ln("ERROR  captivePortal setup failed");
        return false;
    }

    loginfo_ln("Hub device initialized successfully");
    return true;
}

void startInclusionMode()
{
    if (appState != AppState::READY) {
        logwarn_ln("Cannot start inclusion mode in current state");
        return;
    }

    loginfo_ln("Starting inclusion mode");

    // Enable inclusion mode
    int rc = device->enableInclusionMode(true);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to enable inclusion mode: %d", rc);
        appState = AppState::ERROR;
        return;
    }

    // Start broadcasting inclusion open messages
    rc = device->sendInclusionOpen();
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to send inclusion open: %d", rc);
        device->enableInclusionMode(false);
        appState = AppState::ERROR;
        return;
    }

    inclusionModeActive = true;
    appState = AppState::INCLUSION_MODE_ACTIVE;
    totalInclusionRequests = 0;
    successfulInclusions = 0;

    loginfo_ln("Inclusion mode active - devices can now join the network");

    // Send status update to web clients
    if (device && device->getCaptivePortal() && device->getCaptivePortal()->isRunning()) {
        std::vector<byte> emptyData;
        handleGetStatus(nullptr, emptyData);
    }
}

void stopInclusionMode()
{
    if (!inclusionModeActive)
        return;

    loginfo_ln("Stopping inclusion mode");
    device->enableInclusionMode(false);
    inclusionModeActive = false;
    appState = AppState::READY;

    loginfo_ln("Inclusion session summary: %d requests, %d successful", totalInclusionRequests,
               successfulInclusions);

    // Send status update to web clients
    if (device && device->getCaptivePortal() && device->getCaptivePortal()->isRunning()) {
        std::vector<byte> emptyData;
        handleGetStatus(nullptr, emptyData);
    }
}

// Functions for web admin panel to call
void webStartInclusionMode()
{
    startInclusionMode();
}

void webStopInclusionMode()
{
    stopInclusionMode();
}

void setup()
{
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        appState = AppState::ERROR;
        return;
    }

    appState = AppState::READY;
    displayStatus();
}

void loop()
{
    if (!device)
        return;

    // Run the device
    device->run();

    // Check if inclusion mode was disabled by timeout
    if (appState == AppState::INCLUSION_MODE_ACTIVE && !device->isInclusionModeEnabled()) {
        loginfo_ln("Inclusion mode disabled, returning to ready state");
        appState = AppState::READY;
        inclusionModeActive = false;
    }

    // Update display and web clients periodically
    if (millis() - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
        // Only display status if inclusion mode is active (to avoid spam)
        if (inclusionModeActive) {
            displayStatus();
        }

        // Send status update to web clients if captive portal is active
        if (device && device->getCaptivePortal() && device->getCaptivePortal()->isRunning()) {
            std::vector<byte> emptyData;
            handleGetStatus(nullptr, emptyData);
        }

        lastStatusUpdate = millis();
    }
    delay(10);
}
