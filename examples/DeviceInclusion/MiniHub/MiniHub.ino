/**
 * @file MiniHub.ino
 * @brief Hub Device Automatic Inclusion Example
 * 
 * This example demonstrates the automatic inclusion protocol where the
 * InclusionController handles all inclusion logic automatically. The application
 * simply enables inclusion mode and monitors the process.
 */

#include "device_info.h"
#include <Arduino.h>
#include <RadioMesh.h>

// Application states (much simpler than before)
enum class AppState {
    INITIALIZING,
    READY,
    INCLUSION_MODE_ACTIVE,
    ERROR
};

// Constants
const unsigned long INCLUSION_MODE_DURATION = 60000; // 60 seconds inclusion window
const unsigned long STATUS_UPDATE_INTERVAL = 2000;   // Update display every 2 seconds

// Global variables
IDevice* device = nullptr;
IDisplay* display = nullptr;
IRadio* radio = nullptr;

AppState appState = AppState::INITIALIZING;
unsigned long inclusionModeStartTime = 0;
unsigned long lastStatusUpdate = 0;
bool inclusionModeActive = false;

// Counters for monitoring
int totalInclusionRequests = 0;
int successfulInclusions = 0;

void displayStatus() {
    // Status shown via serial log on Xiao board (no display)
    switch (appState) {
        case AppState::INITIALIZING:
            loginfo_ln("Status: Initializing...");
            break;
            
        case AppState::READY:
            loginfo_ln("Status: Hub Ready - waiting for inclusion request");
            break;
            
        case AppState::INCLUSION_MODE_ACTIVE: {
            unsigned long remaining = (INCLUSION_MODE_DURATION - (millis() - inclusionModeStartTime)) / 1000;
            loginfo_ln("Status: Inclusion Active - Time: %lus, Requests: %d, Success: %d", 
                      remaining, totalInclusionRequests, successfulInclusions);
            break;
        }
            
        case AppState::ERROR:
            loginfo_ln("Status: ERROR - Check logs");
            break;
    }
}

/**
 * Callback for monitoring inclusion messages (optional)
 * The inclusion protocol works automatically, but we can monitor it for UI updates
 */
void onPacketReceived(const RadioMeshPacket* packet, int err) {
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Failed [%d]", err);
        return;
    }
    
    // Monitor inclusion messages for statistics
    switch (packet->topic) {
        case MessageTopic::INCLUDE_REQUEST:
            totalInclusionRequests++;
            loginfo_ln("Received inclusion request from device (total: %d)", totalInclusionRequests);
            break;
            
        case MessageTopic::INCLUDE_CONFIRM:
            // When hub receives INCLUDE_CONFIRM, it sends INCLUDE_SUCCESS to complete the protocol
            successfulInclusions++;
            loginfo_ln("Received inclusion confirmation - inclusion completed successfully (total: %d)", successfulInclusions);
            // Exit inclusion mode after successful inclusion
            stopInclusionMode();
            break;
            
        default:
            // Handle other application messages here
            loginfo_ln("Received application message, topic: 0x%02X", packet->topic);
            break;
    }
}

bool initializeHardware() {
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
    
    // No display on Xiao board

    loginfo_ln("Hub device initialized successfully");
    return true;
}

void startInclusionMode() {
    if (appState != AppState::READY) {
        logwarn_ln("Cannot start inclusion mode in current state");
        return;
    }
    
    loginfo_ln("Starting inclusion mode for %lu seconds", INCLUSION_MODE_DURATION / 1000);
    
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
    
    inclusionModeStartTime = millis();
    inclusionModeActive = true;
    appState = AppState::INCLUSION_MODE_ACTIVE;
    totalInclusionRequests = 0;
    successfulInclusions = 0;
    
    loginfo_ln("Inclusion mode active - devices can now join the network");
}

void stopInclusionMode() {
    if (!inclusionModeActive) return;
    
    loginfo_ln("Stopping inclusion mode");
    device->enableInclusionMode(false);
    inclusionModeActive = false;
    appState = AppState::READY;
    
    loginfo_ln("Inclusion session summary: %d requests, %d successful", 
              totalInclusionRequests, successfulInclusions);
}

void handleInclusionModeTimeout() {
    if (!inclusionModeActive) return;
    
    if (millis() - inclusionModeStartTime >= INCLUSION_MODE_DURATION) {
        loginfo_ln("Inclusion mode timeout reached");
        stopInclusionMode();
    }
}

void setup() {
    
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        appState = AppState::ERROR;
        return;
    }
    
    appState = AppState::READY;
    displayStatus();
    
    // Automatically start inclusion mode for demonstration
    // In real applications, this would be triggered by user input (button press, web UI, etc.)
    delay(2000);
    startInclusionMode();
}

void loop() {
    if (!device) return;
    
    // Run the device
    device->run();
    
    // Handle inclusion mode timeout
    handleInclusionModeTimeout();
    
    // Update display periodically
    if (millis() - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
        displayStatus();
        lastStatusUpdate = millis();
    }
    
    // In a real application, you would handle user input here
    // For example: button press to start/stop inclusion mode
    
    delay(10);
}