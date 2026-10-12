/*
 * RadioMesh Zephyr example — HUB device.
 *
 * Mirrors the Arduino examples/DeviceInclusion/MiniHub/MiniHub.ino: the
 * InclusionController handles all inclusion logic; the application enables
 * inclusion mode and monitors the process. Not mirrored: the WiFi access point
 * and the web device portal (WiFi is not part of the Zephyr port).
 *
 * Substitution: MiniHub enables inclusion mode when the user clicks the portal's
 * "Enable Inclusion Mode" button. Here a timer does it: the first inclusion
 * window opens INCLUSION_FIRST_OPEN ms after boot, and a new one opens
 * INCLUSION_REOPEN_INTERVAL ms after each window closes (on a successful
 * inclusion, or on the protocol's 60 s session timeout). Devices that are
 * already included ignore the extra INCLUDE_OPEN broadcasts.
 */
#include <zephyr/kernel.h>

#include <RadioMesh.h>

#include <set>

// Application states (much simpler than before)
enum class AppState
{
    INITIALIZING,
    READY,
    INCLUSION_MODE_ACTIVE,
    ERROR
};

// Constants
static const uint32_t STATUS_UPDATE_INTERVAL = 2000;     // Update status every 2 seconds
static const uint32_t INCLUSION_FIRST_OPEN = 5000;       // First window, after boot
static const uint32_t INCLUSION_REOPEN_INTERVAL = 30000; // Next window, after one closes

// Fixed test identity
static const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x5A, 0x00, 0x00, 0x00};
static const std::string DEVICE_NAME = "ZephyrHub";

// Global variables
static IDevice* device = nullptr;
static IRadio* radio = nullptr;

static AppState appState = AppState::INITIALIZING;
static uint32_t lastStatusUpdate = 0;
static bool inclusionModeActive = false;

// Inclusion trigger timing (boot counts as the first close)
static uint32_t inclusionClosedAt = 0;
static uint32_t inclusionOpenDelay = INCLUSION_FIRST_OPEN;

// Counters for monitoring
static int totalInclusionRequests = 0;
static int successfulInclusions = 0;

// Included devices (MiniHub tracks them for the admin panel)
static std::set<std::string> connectedDevices;

static void displayStatus()
{
    // Status shown via serial log on Xiao board (no display)
    switch (appState) {
    case AppState::INITIALIZING:
        loginfo_ln("Status: Initializing...");
        break;

    case AppState::READY: {
        auto deviceId = device->getDeviceId();
        char deviceIdStr[16];
        snprintf(deviceIdStr, sizeof(deviceIdStr), "%02X%02X%02X%02X", deviceId[0], deviceId[1],
                 deviceId[2], deviceId[3]);
        loginfo_ln("Status: Hub Ready - Device ID: %s, Connected Devices: %d", deviceIdStr,
                   static_cast<int>(connectedDevices.size()));
        break;
    }

    case AppState::INCLUSION_MODE_ACTIVE:
        loginfo_ln("Status: Inclusion Active - Requests: %d, Success: %d", totalInclusionRequests,
                   successfulInclusions);
        break;

    case AppState::ERROR:
        loginfo_ln("Status: ERROR - Check logs");
        break;
    }
}

static void markInclusionClosed()
{
    inclusionClosedAt = k_uptime_get_32();
    inclusionOpenDelay = INCLUSION_REOPEN_INTERVAL;
}

static void startInclusionMode()
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
}

static void stopInclusionMode()
{
    if (!inclusionModeActive)
        return;

    loginfo_ln("Stopping inclusion mode");
    device->enableInclusionMode(false);
    inclusionModeActive = false;
    appState = AppState::READY;
    markInclusionClosed();

    loginfo_ln("Inclusion session summary: %d requests, %d successful", totalInclusionRequests,
               successfulInclusions);
}

/**
 * Callback for monitoring inclusion messages and tracking devices
 */
static void onPacketReceived(const RadioMeshPacket* packet, int err)
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

    // Monitor inclusion messages for statistics
    switch (packet->topic) {
    case MessageTopic::INCLUDE_REQUEST:
        totalInclusionRequests++;
        loginfo_ln("Received inclusion request from device %s (total: %d)", deviceId.c_str(),
                   totalInclusionRequests);
        break;

    case MessageTopic::INCLUDE_CONFIRM:
        // When hub receives INCLUDE_CONFIRM, it sends INCLUDE_SUCCESS to complete the protocol
        successfulInclusions++;
        loginfo_ln("Received inclusion confirmation from device %s - inclusion completed "
                   "successfully (total: %d)",
                   deviceId.c_str(), successfulInclusions);

        // Add device to tracking
        connectedDevices.insert(deviceId);

        // Stop inclusion mode after successful inclusion
        stopInclusionMode();
        break;

    default:
        // Handle other application messages. Zephyr addition: also log the first
        // payload byte (the standard example's mock sensor value) so it can be
        // matched against what the sender logged.
        loginfo_ln("Received application message from %s, topic: 0x%02X, value: %d",
                   deviceId.c_str(), packet->topic,
                   packet->packetData.empty() ? -1 : packet->packetData[0]);
        break;
    }
}

static bool initializeHardware()
{
    loginfo_ln("Initializing hub device...");

    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262)
                 .withRxPacketCallback(onPacketReceived)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::HUB);

    if (!device) {
        logerr_ln("Failed to create device");
        return false;
    }

    radio = device->getRadio();
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

    loginfo_ln("Hub device initialized successfully");
    return true;
}

int main()
{
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        appState = AppState::ERROR;
        return 0;
    }

    appState = AppState::READY;
    displayStatus();

    while (true) {
        // Run the device
        device->run();

        // Check if inclusion mode was disabled by timeout
        if (appState == AppState::INCLUSION_MODE_ACTIVE && !device->isInclusionModeEnabled()) {
            loginfo_ln("Inclusion mode disabled, returning to ready state");
            appState = AppState::READY;
            inclusionModeActive = false;
            markInclusionClosed();
        }

        // Timed trigger in place of the portal's "Enable Inclusion Mode" button
        if (appState == AppState::READY &&
            k_uptime_get_32() - inclusionClosedAt >= inclusionOpenDelay) {
            startInclusionMode();
        }

        // Update status periodically
        if (k_uptime_get_32() - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
            // Only display status if inclusion mode is active (to avoid spam)
            if (inclusionModeActive) {
                displayStatus();
            }
            lastStatusUpdate = k_uptime_get_32();
        }

        k_msleep(10);
    }
    return 0;
}
