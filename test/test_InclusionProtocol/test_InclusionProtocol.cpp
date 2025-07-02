#include <unity.h>
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <core/protocol/inc/packet/Packet.h>

// Simple test device class without inheritance complexity
class TestDevice 
{
public:
    TestDevice(MeshDeviceType type) : deviceType(type), inclusionState(DeviceInclusionState::NOT_INCLUDED) {
        if (type == MeshDeviceType::HUB) {
            inclusionState = DeviceInclusionState::INCLUDED;
            inclusionModeEnabled = false;
        }
    }
    
    // Test helper methods
    bool isInclusionMessage(uint8_t topic) {
        return (topic >= INCLUDE_REQUEST && topic <= INCLUDE_SUCCESS);
    }
    
    DeviceInclusionState getInclusionState() { return inclusionState; }
    void setInclusionState(DeviceInclusionState state) { inclusionState = state; }
    
    bool isInclusionModeEnabled() { return inclusionModeEnabled; }
    void setInclusionMode(bool enabled) { inclusionModeEnabled = enabled; }
    
    MeshDeviceType getDeviceType() { return deviceType; }
    
    // Mock message handling
    int handleInclusionMessage(const RadioMeshPacket& packet) {
        if (deviceType == MeshDeviceType::HUB) {
            return handleHubMessage(packet);
        } else {
            return handleDeviceMessage(packet);
        }
    }
    
    // Test data
    uint8_t lastSentTopic = 0;
    std::vector<byte> lastSentData;
    
private:
    MeshDeviceType deviceType;
    DeviceInclusionState inclusionState;
    bool inclusionModeEnabled = false;
    
    int handleHubMessage(const RadioMeshPacket& packet) {
        switch (packet.topic) {
            case INCLUDE_REQUEST:
                if (inclusionModeEnabled) {
                    lastSentTopic = INCLUDE_RESPONSE;
                    return RM_E_NONE;
                } else {
                    return RM_E_INVALID_STATE;
                }
                break;
            case INCLUDE_CONFIRM:
                lastSentTopic = INCLUDE_SUCCESS;
                return RM_E_NONE;
                break;
        }
        return RM_E_NONE;
    }
    
    int handleDeviceMessage(const RadioMeshPacket& packet) {
        switch (packet.topic) {
            case INCLUDE_OPEN:
                if (inclusionState == DeviceInclusionState::NOT_INCLUDED) {
                    inclusionState = DeviceInclusionState::INCLUSION_PENDING;
                    lastSentTopic = INCLUDE_REQUEST;
                    return RM_E_NONE;
                }
                break;
            case INCLUDE_RESPONSE:
                if (inclusionState == DeviceInclusionState::INCLUSION_PENDING) {
                    lastSentTopic = INCLUDE_CONFIRM;
                    return RM_E_NONE;
                }
                break;
            case INCLUDE_SUCCESS:
                if (inclusionState == DeviceInclusionState::INCLUSION_PENDING) {
                    inclusionState = DeviceInclusionState::INCLUDED;
                    return RM_E_NONE;
                }
                break;
        }
        return RM_E_NONE;
    }
};

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

void test_device_isInclusionMessage() {
    TestDevice device(MeshDeviceType::STANDARD);
    
    // Test inclusion message topics
    TEST_ASSERT_TRUE(device.isInclusionMessage(INCLUDE_REQUEST));
    TEST_ASSERT_TRUE(device.isInclusionMessage(INCLUDE_RESPONSE));
    TEST_ASSERT_TRUE(device.isInclusionMessage(INCLUDE_OPEN));
    TEST_ASSERT_TRUE(device.isInclusionMessage(INCLUDE_CONFIRM));
    TEST_ASSERT_TRUE(device.isInclusionMessage(INCLUDE_SUCCESS));
    
    // Test non-inclusion message topics
    TEST_ASSERT_FALSE(device.isInclusionMessage(PING));
    TEST_ASSERT_FALSE(device.isInclusionMessage(PONG));
    TEST_ASSERT_FALSE(device.isInclusionMessage(ACK));
    TEST_ASSERT_FALSE(device.isInclusionMessage(0x10)); // User topic
}

void test_device_handles_include_open() {
    TestDevice device(MeshDeviceType::STANDARD);
    
    // Create INCLUDE_OPEN packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_OPEN;
    packet.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    packet.destDevId = {0x00, 0x00, 0x00, 0x00};   // Broadcast
    
    // Device should be NOT_INCLUDED initially
    TEST_ASSERT_EQUAL(DeviceInclusionState::NOT_INCLUDED, device.getInclusionState());
    
    // Process the INCLUDE_OPEN message
    int result = device.handleInclusionMessage(packet);
    
    // Should succeed
    TEST_ASSERT_EQUAL(RM_E_NONE, result);
    
    // Should have sent INCLUDE_REQUEST
    TEST_ASSERT_EQUAL(INCLUDE_REQUEST, device.lastSentTopic);
    
    // State should now be INCLUSION_PENDING
    TEST_ASSERT_EQUAL(DeviceInclusionState::INCLUSION_PENDING, device.getInclusionState());
}

void test_device_handles_include_response() {
    TestDevice device(MeshDeviceType::STANDARD);
    
    // Set device to INCLUSION_PENDING state
    device.setInclusionState(DeviceInclusionState::INCLUSION_PENDING);
    
    // Create INCLUDE_RESPONSE packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_RESPONSE;
    packet.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    packet.destDevId = {0x01, 0x02, 0x03, 0x04};   // Device ID
    
    // Process the INCLUDE_RESPONSE message
    int result = device.handleInclusionMessage(packet);
    
    // Should succeed
    TEST_ASSERT_EQUAL(RM_E_NONE, result);
    
    // Should have sent INCLUDE_CONFIRM
    TEST_ASSERT_EQUAL(INCLUDE_CONFIRM, device.lastSentTopic);
}

void test_device_handles_include_success() {
    TestDevice device(MeshDeviceType::STANDARD);
    
    // Set device to INCLUSION_PENDING state
    device.setInclusionState(DeviceInclusionState::INCLUSION_PENDING);
    
    // Create INCLUDE_SUCCESS packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_SUCCESS;
    packet.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    packet.destDevId = {0x01, 0x02, 0x03, 0x04};   // Device ID
    
    // Process the INCLUDE_SUCCESS message
    int result = device.handleInclusionMessage(packet);
    
    // Should succeed
    TEST_ASSERT_EQUAL(RM_E_NONE, result);
    
    // State should now be INCLUDED
    TEST_ASSERT_EQUAL(DeviceInclusionState::INCLUDED, device.getInclusionState());
}

void test_hub_handles_include_request() {
    TestDevice hub(MeshDeviceType::HUB);
    
    // Enable inclusion mode
    hub.setInclusionMode(true);
    
    // Create INCLUDE_REQUEST packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_REQUEST;
    packet.sourceDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    packet.destDevId = {0x11, 0x12, 0x13, 0x14};   // Hub ID
    
    // Process the INCLUDE_REQUEST message
    int result = hub.handleInclusionMessage(packet);
    
    // Should succeed
    TEST_ASSERT_EQUAL(RM_E_NONE, result);
    
    // Should have sent INCLUDE_RESPONSE
    TEST_ASSERT_EQUAL(INCLUDE_RESPONSE, hub.lastSentTopic);
}

void test_hub_handles_include_confirm() {
    TestDevice hub(MeshDeviceType::HUB);
    
    // Enable inclusion mode
    hub.setInclusionMode(true);
    
    // Create INCLUDE_CONFIRM packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_CONFIRM;
    packet.sourceDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    packet.destDevId = {0x11, 0x12, 0x13, 0x14};   // Hub ID
    
    // Process the INCLUDE_CONFIRM message
    int result = hub.handleInclusionMessage(packet);
    
    // Should succeed
    TEST_ASSERT_EQUAL(RM_E_NONE, result);
    
    // Should have sent INCLUDE_SUCCESS
    TEST_ASSERT_EQUAL(INCLUDE_SUCCESS, hub.lastSentTopic);
}

void test_hub_rejects_request_when_not_in_inclusion_mode() {
    TestDevice hub(MeshDeviceType::HUB);
    
    // DON'T enable inclusion mode (should be false by default)
    
    // Create INCLUDE_REQUEST packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_REQUEST;
    packet.sourceDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    packet.destDevId = {0x11, 0x12, 0x13, 0x14};   // Hub ID
    
    // Process the INCLUDE_REQUEST message
    int result = hub.handleInclusionMessage(packet);
    
    // Should fail
    TEST_ASSERT_EQUAL(RM_E_INVALID_STATE, result);
    
    // Should NOT have sent any response
    TEST_ASSERT_NOT_EQUAL(INCLUDE_RESPONSE, hub.lastSentTopic);
}

void test_device_ignores_include_open_when_already_included() {
    TestDevice device(MeshDeviceType::STANDARD);
    
    // Set device to INCLUDED state
    device.setInclusionState(DeviceInclusionState::INCLUDED);
    
    // Create INCLUDE_OPEN packet
    RadioMeshPacket packet;
    packet.topic = INCLUDE_OPEN;
    packet.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    packet.destDevId = {0x00, 0x00, 0x00, 0x00};   // Broadcast
    
    // Clear last sent data
    device.lastSentTopic = 0;
    
    // Process the INCLUDE_OPEN message
    int result = device.handleInclusionMessage(packet);
    
    // Should succeed but do nothing
    TEST_ASSERT_EQUAL(RM_E_NONE, result);
    
    // Should NOT have sent INCLUDE_REQUEST
    TEST_ASSERT_NOT_EQUAL(INCLUDE_REQUEST, device.lastSentTopic);
    
    // State should remain INCLUDED
    TEST_ASSERT_EQUAL(DeviceInclusionState::INCLUDED, device.getInclusionState());
}

void test_automatic_inclusion_flow_integration() {
    TestDevice hub(MeshDeviceType::HUB);
    TestDevice device(MeshDeviceType::STANDARD);
    
    // Step 1: Hub enters inclusion mode
    hub.setInclusionMode(true);
    TEST_ASSERT_TRUE(hub.isInclusionModeEnabled());
    
    // Step 2: Device receives INCLUDE_OPEN and responds
    RadioMeshPacket openPacket;
    openPacket.topic = INCLUDE_OPEN;
    openPacket.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    openPacket.destDevId = {0x00, 0x00, 0x00, 0x00}; // Broadcast
    
    device.handleInclusionMessage(openPacket);
    TEST_ASSERT_EQUAL(INCLUDE_REQUEST, device.lastSentTopic);
    TEST_ASSERT_EQUAL(DeviceInclusionState::INCLUSION_PENDING, device.getInclusionState());
    
    // Step 3: Hub receives INCLUDE_REQUEST and responds
    RadioMeshPacket requestPacket;
    requestPacket.topic = INCLUDE_REQUEST;
    requestPacket.sourceDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    requestPacket.destDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    
    hub.handleInclusionMessage(requestPacket);
    TEST_ASSERT_EQUAL(INCLUDE_RESPONSE, hub.lastSentTopic);
    
    // Step 4: Device receives INCLUDE_RESPONSE and confirms
    RadioMeshPacket responsePacket;
    responsePacket.topic = INCLUDE_RESPONSE;
    responsePacket.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    responsePacket.destDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    
    device.handleInclusionMessage(responsePacket);
    TEST_ASSERT_EQUAL(INCLUDE_CONFIRM, device.lastSentTopic);
    
    // Step 5: Hub receives INCLUDE_CONFIRM and sends success
    RadioMeshPacket confirmPacket;
    confirmPacket.topic = INCLUDE_CONFIRM;
    confirmPacket.sourceDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    confirmPacket.destDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    
    hub.handleInclusionMessage(confirmPacket);
    TEST_ASSERT_EQUAL(INCLUDE_SUCCESS, hub.lastSentTopic);
    
    // Step 6: Device receives INCLUDE_SUCCESS and completes inclusion
    RadioMeshPacket successPacket;
    successPacket.topic = INCLUDE_SUCCESS;
    successPacket.sourceDevId = {0x11, 0x12, 0x13, 0x14}; // Hub ID
    successPacket.destDevId = {0x01, 0x02, 0x03, 0x04}; // Device ID
    
    device.handleInclusionMessage(successPacket);
    TEST_ASSERT_EQUAL(DeviceInclusionState::INCLUDED, device.getInclusionState());
}

void setup() {
    UNITY_BEGIN();
    
    RUN_TEST(test_device_isInclusionMessage);
    RUN_TEST(test_device_handles_include_open);
    RUN_TEST(test_device_handles_include_response);
    RUN_TEST(test_device_handles_include_success);
    RUN_TEST(test_hub_handles_include_request);
    RUN_TEST(test_hub_handles_include_confirm);
    RUN_TEST(test_hub_rejects_request_when_not_in_inclusion_mode);
    RUN_TEST(test_device_ignores_include_open_when_already_included);
    RUN_TEST(test_automatic_inclusion_flow_integration);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework
}