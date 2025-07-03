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
enum class AppState {
    INITIALIZING,
    NOT_INCLUDED,      // Device is not part of the network
    INCLUSION_PENDING, // Inclusion process is ongoing
    INCLUDED,          // Device successfully joined the network
    ERROR
};

// Constants
const unsigned long STATUS_UPDATE_INTERVAL = 3000;  // Update status every 3 seconds
const unsigned long SENSOR_DATA_INTERVAL = 10000;   // Send sensor data every 10 seconds

// Global variables
IDevice* device = nullptr;
IRadio* radio = nullptr;

AppState appState = AppState::INITIALIZING;
unsigned long lastStatusUpdate = 0;
unsigned long lastSensorData = 0;
int sensorValue = 0;  // Mock sensor value

/**
 * Callback for monitoring messages and inclusion progress
 */
void onPacketReceived(const RadioMeshPacket* packet, int err) {
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

bool initializeHardware() {
    loginfo_ln("Initializing standard device...");
    
    device = DeviceBuilder()
                .start()
                .withLoraRadio(LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262)
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

    // Setup individual components
    int rc = radio->setup();
    if (rc != RM_E_NONE) {
        logerr_ln("Radio setup failed: %d", rc);
        return false;
    }

    loginfo_ln("Standard device initialized successfully");
    return true;
}

void updateAppState() {
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
    }
}

void sendSensorData() {
    if (appState != AppState::INCLUDED) {
        // Can only send data when included in the network
        return;
    }
    
    if (millis() - lastSensorData < SENSOR_DATA_INTERVAL) {
        return;
    }
    
    // Simulate sensor reading
    sensorValue = random(20, 30);  // Mock temperature sensor (20-30°C)
    
    // Prepare data payload
    std::vector<byte> payload;
    payload.push_back(static_cast<byte>(sensorValue));
    
    // Send to hub (broadcast to all hub devices)
    std::array<byte, RM_ID_LENGTH> hubTarget = {0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast
    
    int rc = device->sendData(MessageTopic::DATA, payload, hubTarget);
    if (rc == RM_E_NONE) {
        loginfo_ln("Sent sensor data: %d°C", sensorValue);
    } else {
        logerr_ln("Failed to send sensor data: %d", rc);
    }
    
    lastSensorData = millis();
}

void printStatus() {
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

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(100);
    
    loginfo_ln("=== RadioMesh Standard Device - Automatic Inclusion Example ===");
    
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        appState = AppState::ERROR;
        return;
    }
    
    appState = AppState::NOT_INCLUDED;
    loginfo_ln("Device ready - waiting for hub inclusion broadcast");
    loginfo_ln("Device will automatically join when hub enables inclusion mode");
}

void loop() {
    if (!device) return;
    
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