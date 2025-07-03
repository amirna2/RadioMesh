/**
 * @file StandardDevice.ino
 * @brief Standard Device Automatic Inclusion Example
 *
 * This example demonstrates the automatic inclusion protocol for a standard device.
 * The device automatically joins the network when a hub broadcasts INCLUDE_OPEN messages.
 */

#include "device_info.h"
#include <Arduino.h>
#include <RadioMesh.h>

// Application states (simplified)
enum class AppState
{
    INITIALIZING,
    NOT_INCLUDED,      // Device is not part of the network
    INCLUSION_PENDING, // Inclusion process is ongoing
    INCLUDED,          // Device successfully joined the network
    ERROR
};

// Constants
const unsigned long STATUS_UPDATE_INTERVAL = 3000; // Update status every 3 seconds
const unsigned long SENSOR_DATA_INTERVAL = 10000;  // Send sensor data every 10 seconds
const unsigned long FACTORY_RESET_TIMEOUT = 300000; // 5 minutes - auto factory reset if not included

// Global variables
IDevice* device = nullptr;
IRadio* radio = nullptr;
IDisplay* display = nullptr;

AppState appState = AppState::INITIALIZING;
unsigned long lastStatusUpdate = 0;
unsigned long lastSensorData = 0;
unsigned long bootTime = 0;
int sensorValue = 0; // Mock sensor value

// Display helper functions
#ifdef USE_DISPLAY
void updateDisplay()
{
    if (!display)
        return;

    display->clear();
    display->drawString(0, 0, "RadioMesh Sensor");

    switch (appState) {
    case AppState::INITIALIZING:
        display->drawString(0, 20, "Initializing...");
        break;

    case AppState::NOT_INCLUDED:
        display->drawString(0, 20, "Listening for hub");
        display->drawString(0, 30, "Not connected");
        break;

    case AppState::INCLUSION_PENDING:
        display->drawString(0, 20, "Joining network...");
        display->drawString(0, 30, "Please wait");
        break;

    case AppState::INCLUDED:
        display->drawString(0, 20, "Connected!");
        display->drawString(0, 30, "Temp: " + std::to_string(sensorValue) + "C");
        break;

    case AppState::ERROR:
        display->drawString(0, 20, "ERROR");
        display->drawString(0, 30, "Check config");
        break;
    }
}
#endif

/**
 * Callback for monitoring messages and inclusion progress
 */
void onPacketReceived(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Failed [%d]", err);
        return;
    }

    // Monitor inclusion messages for UI feedback
    switch (packet->topic) {
    case MessageTopic::INCLUDE_OPEN:
        loginfo_ln("Received INCLUDE_OPEN from hub");
        break;

    case MessageTopic::INCLUDE_RESPONSE:
        loginfo_ln("Received INCLUDE_RESPONSE from hub");
        break;

    case MessageTopic::INCLUDE_SUCCESS:
        loginfo_ln("Inclusion completed successfully! Device is now part of the network");
        break;

    default:
        // Handle normal application messages
        loginfo_ln("Received application message, topic: 0x%02X", packet->topic);
        break;
    }
}

bool initializeHardware()
{
    loginfo_ln("Initializing standard device...");

    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(radioParams)
#ifdef USE_DISPLAY
                 .withOledDisplay(displayParams)
#endif
                 .withRxPacketCallback(onPacketReceived)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::STANDARD);

    if (!device) {
        logerr_ln("Failed to create device");
        return false;
    }

    radio = device->getRadio();
    if (!radio) {
        logerr_ln("Failed to get radio instance");
        return false;
    }

#ifdef USE_DISPLAY
    display = device->getDisplay();
    if (!display) {
        logerr_ln("Failed to get display instance");
        return false;
    }

    // Setup display
    if (display->setup() != RM_E_NONE) {
        logerr_ln("Display setup failed");
        return false;
    }
    display->clear();
    display->drawString(0, 0, "RadioMesh Sensor");
    display->drawString(0, 10, "Initializing...");
#endif

    // Setup radio
    int rc = radio->setup();
    if (rc != RM_E_NONE) {
        logerr_ln("Radio setup failed: %d", rc);
        return false;
    }

    loginfo_ln("Standard device initialized successfully");
    return true;
}

void updateAppState()
{
    AppState newState;

    // Check current inclusion state from the InclusionController
    if (device->isIncluded()) {
        newState = AppState::INCLUDED;
    } else {
        newState = AppState::NOT_INCLUDED;
    }

    if (newState != appState) {
        appState = newState;

        switch (appState) {
        case AppState::NOT_INCLUDED:
            loginfo_ln("Device state: NOT_INCLUDED - waiting for hub inclusion broadcast");
            break;

        case AppState::INCLUSION_PENDING:
            loginfo_ln("Device state: INCLUSION_PENDING - joining network...");
            break;

        case AppState::INCLUDED:
            loginfo_ln("Device state: INCLUDED - successfully joined network!");
            break;

        default:
            break;
        }

#ifdef USE_DISPLAY
        updateDisplay();
#endif
    }
}

void sendSensorData()
{
    if (appState != AppState::INCLUDED) {
        // Can only send data when included in the network
        return;
    }

    if (millis() - lastSensorData < SENSOR_DATA_INTERVAL) {
        return;
    }

    // Simulate sensor reading
    sensorValue = random(20, 30); // Mock temperature sensor (20-30°C)

    // Prepare data payload
    std::vector<byte> payload;
    payload.push_back(static_cast<byte>(sensorValue));

    // Send to hub (broadcast to all hub devices)
    std::array<byte, RM_ID_LENGTH> hubTarget = {0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast

    int rc = device->sendData(0x10, payload, hubTarget); // Use topic 0x10 for sensor data
    if (rc == RM_E_NONE) {
        loginfo_ln("Sent sensor data: %d°C", sensorValue);
    } else {
        logerr_ln("Failed to send sensor data: %d", rc);
    }

    lastSensorData = millis();

#ifdef USE_DISPLAY
    updateDisplay(); // Update display with new sensor value
#endif
}

void printStatus()
{
    if (millis() - lastStatusUpdate < STATUS_UPDATE_INTERVAL) {
        return;
    }

    switch (appState) {
    case AppState::INITIALIZING:
        loginfo_ln("Status: Initializing device...");
        break;

    case AppState::NOT_INCLUDED:
        loginfo_ln("Status: Not included - listening for hub inclusion broadcasts");
        break;

    case AppState::INCLUSION_PENDING:
        loginfo_ln("Status: Inclusion in progress - automatic protocol handling");
        break;

    case AppState::INCLUDED:
        loginfo_ln("Status: Included in network - sending sensor data (last: %d°C)", sensorValue);
        break;

    case AppState::ERROR:
        loginfo_ln("Status: ERROR - check device configuration");
        break;
    }

    lastStatusUpdate = millis();
}

void setup()
{
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        appState = AppState::ERROR;
        return;
    }
    
    // Factory reset on every boot for testing
    loginfo_ln("Performing factory reset...");
    int rc = device->factoryReset();
    if (rc == RM_E_NONE) {
        loginfo_ln("Factory reset successful");
    } else {
        logerr_ln("Factory reset failed: %d", rc);
    }

    appState = AppState::NOT_INCLUDED;
    loginfo_ln("Device ready - waiting for hub inclusion broadcast");
    loginfo_ln("Device will automatically join when hub enables inclusion mode");

#ifdef USE_DISPLAY
    updateDisplay();
#endif
}

void loop()
{
    if (!device)
        return;

    // Run the device
    device->run();

    // Update application state based on inclusion status
    updateAppState();

    // Send sensor data if included in network
    sendSensorData();

    // Print periodic status updates
    printStatus();

    delay(10);
}
