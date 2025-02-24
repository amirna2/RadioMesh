#include "device_info.h"
#include <Arduino.h>
#include <RadioMesh.h>

// States for inclusion process
enum class InclusionState
{
    IDLE,
    BROADCASTING_OPEN,
    WAITING_REQUEST,
    SENDING_RESPONSE,
    WAITING_CONFIRM,
    COMPLETE
};

// Constants
const unsigned long BROADCAST_INTERVAL = 10000; // 10s between OPEN broadcasts
const unsigned long REQUEST_TIMEOUT = 30000;    // 30s to wait for REQUEST
const unsigned long CONFIRM_TIMEOUT = 10000;    // 10s to wait for CONFIRM

// Global variables
IDevice* device = nullptr;
IDisplay* display = nullptr;
IRadio* radio = nullptr;

volatile uint8_t lastMessageTopic = MessageTopic::UNUSED;
volatile bool messageReceived = false;

InclusionState inclusionState = InclusionState::IDLE;
unsigned long stateTimer = 0;
unsigned long lastBroadcast = 0;

void displayMessage(const std::string& message, bool clear = true)
{
    if (!display)
        return;
    if (clear)
        display->clear();
    display->drawString(10, 25, message);
}

void RxCallback(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Failed [%d]", err);
        return;
    }
    loginfo_ln("Received message, topic: %d", packet->topic);
    lastMessageTopic = packet->topic;
    messageReceived = true;
}

bool initializeHardware()
{
    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(radioParams)
                 .withOledDisplay(displayParams)
                 .withRxPacketCallback(RxCallback)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::HUB);

    if (!device)
        return false;

    radio = device->getRadio();
    display = device->getDisplay();

    if (!radio || !display)
        return false;

    int rc = radio->setup();
    if (rc != RM_E_NONE)
        return false;

    rc = display->setup();
    if (rc != RM_E_NONE)
        return false;

    return true;
}

void handleInclusionState()
{
    unsigned long now = millis();

    std::vector<byte> hubKey;
    std::vector<byte> nonce;

    switch (inclusionState) {
    case InclusionState::IDLE: {
        inclusionState = InclusionState::BROADCASTING_OPEN;
        stateTimer = now;
        lastBroadcast = 0; // Force immediate broadcast
        device->enableInclusionMode(true);
        displayMessage("Inclusion Mode");
    } break;

    case InclusionState::BROADCASTING_OPEN: {
        if (now - lastBroadcast >= BROADCAST_INTERVAL) {
            loginfo_ln("Broadcasting INCLUDE_OPEN");
            device->sendInclusionOpen();
            lastBroadcast = now;
            inclusionState = InclusionState::WAITING_REQUEST;
            stateTimer = now;
            displayMessage("Waiting Request");
        }
    } break;

    case InclusionState::WAITING_REQUEST: {
        if (now - stateTimer >= REQUEST_TIMEOUT) {
            inclusionState = InclusionState::BROADCASTING_OPEN;
            stateTimer = now;
        }
    } break;

    case InclusionState::SENDING_RESPONSE: {
        hubKey = {9, 8, 7, 6};
        nonce = {5, 4, 3, 2};
        device->sendInclusionResponse(hubKey, nonce, 0);
        inclusionState = InclusionState::WAITING_CONFIRM;
        stateTimer = now;
        displayMessage("Waiting Confirm");
    } break;

    case InclusionState::WAITING_CONFIRM: {
        if (now - stateTimer >= CONFIRM_TIMEOUT) {
            inclusionState = InclusionState::BROADCASTING_OPEN;
            stateTimer = now;
        }
    } break;

    case InclusionState::COMPLETE: {
        device->sendInclusionSuccess();
        displayMessage("Inclusion Success");
        inclusionState = InclusionState::IDLE;
    } break;
    }
}
void handleMessage()
{
    if (!messageReceived)
        return;

    switch (lastMessageTopic) {
    case MessageTopic::INCLUDE_REQUEST:
        if (inclusionState == InclusionState::WAITING_REQUEST) {
            inclusionState = InclusionState::SENDING_RESPONSE;
        }
        break;

    case MessageTopic::INCLUDE_CONFIRM:
        if (inclusionState == InclusionState::WAITING_CONFIRM) {
            inclusionState = InclusionState::COMPLETE;
        }
        break;
    }

    messageReceived = false;
    lastMessageTopic = MessageTopic::UNUSED;
}

void setup()
{
    if (!initializeHardware()) {
        logerr_ln("Hardware initialization failed");
        return;
    }
    displayMessage("MiniHub Ready");
    inclusionState = InclusionState::IDLE;
}

void loop()
{
    if (!device)
        return;

    handleInclusionState();
    handleMessage();
    device->run();
    delay(10);
}
