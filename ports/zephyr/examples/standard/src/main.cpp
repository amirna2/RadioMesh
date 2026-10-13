/*
 * RadioMesh Zephyr example — STANDARD device (leaf, or relay with relay.conf).
 *
 * Mirrors the Arduino examples/DeviceInclusion/Standard/StandardDevice.ino: the
 * device joins the network automatically when a hub opens inclusion, then sends
 * a mock sensor reading on topic 0x10 every 10 seconds. Not mirrored: the OLED
 * display (the XIAO has none).
 *
 * Built with relay.conf (CONFIG_RM_EXAMPLE_RELAY, `make build ROLE=relay`) the
 * same device also relays packets for other nodes, under its own ID and name so
 * a leaf and a relay can run in the same cluster.
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

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
static const uint32_t STATUS_UPDATE_INTERVAL = 3000; // Update status every 3 seconds
static const uint32_t SENSOR_DATA_INTERVAL = 10000;  // Send sensor data every 10 seconds

// Fixed test identities, one per variant.
static constexpr bool RELAY = IS_ENABLED(CONFIG_RM_EXAMPLE_RELAY);
static const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x5A, 0x00, 0x00, RELAY ? 0x02 : 0x01};
static const std::string DEVICE_NAME = RELAY ? "ZephyrRelay" : "ZephyrStandard";

// Global variables
static IDevice* device = nullptr;
static IRadio* radio = nullptr;

static AppState appState = AppState::INITIALIZING;
static uint32_t lastStatusUpdate = 0;
static uint32_t lastSensorData = 0;
static int sensorValue = 0; // Mock sensor value

/**
 * Callback for monitoring messages and inclusion progress
 */
static void onPacketReceived(const RadioMeshPacket* packet, int err)
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

static bool initializeHardware()
{
    loginfo_ln("Initializing standard device...");

    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262)
                 .withRelayEnabled(RELAY)
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

    // Setup radio
    int rc = radio->setup();
    if (rc != RM_E_NONE) {
        logerr_ln("Radio setup failed: %d", rc);
        return false;
    }

    loginfo_ln("Standard device initialized successfully");
    return true;
}

static void updateAppState()
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
    }
}

static void sendSensorData()
{
    if (appState != AppState::INCLUDED) {
        // Can only send data when included in the network
        return;
    }

    if (k_uptime_get_32() - lastSensorData < SENSOR_DATA_INTERVAL) {
        return;
    }

    // Simulate sensor reading
    sensorValue = 20 + static_cast<int>(sys_rand32_get() % 10); // Mock temperature sensor (20-30°C)

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

    lastSensorData = k_uptime_get_32();
}

static void printStatus()
{
    if (k_uptime_get_32() - lastStatusUpdate < STATUS_UPDATE_INTERVAL) {
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

    lastStatusUpdate = k_uptime_get_32();
}

int main()
{
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        appState = AppState::ERROR;
        return 0;
    }

    while (true) {
        // Run the device
        device->run();

        // Update application state based on inclusion status
        updateAppState();

        // Send sensor data if included in network
        sendSensorData();

        // Print periodic status updates
        printStatus();

        k_msleep(10);
    }
    return 0;
}
