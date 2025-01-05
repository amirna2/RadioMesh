#include "device_info.h"
#include <Arduino.h>
#include <RadioMesh.h>

enum class InclusionState
{
    IDLE,
    WAITING_RESPONSE,
    CONFIRMING,
    INCLUDED
};

IDevice* device = nullptr;
InclusionState inclusionState = InclusionState::IDLE;
unsigned long stateTimer = 0;
const unsigned long RESPONSE_TIMEOUT = 30000; // 30s timeout waiting for response

volatile uint8_t lastMessageTopic = MessageTopic::UNUSED;
volatile bool messageReceived = false;

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

void setup()
{
    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262)
                 .withRxPacketCallback(RxCallback)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::STANDARD);

    if (!device)
        return;

    IRadio* radio = device->getRadio();
    if (!radio) {
        logerr_ln("Failed to get radio instance");
        return;
    }

    int rc = radio->setup();
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to setup radio: %d", rc);
        return;
    }

    loginfo_ln("Device setup complete");
    inclusionState = InclusionState::IDLE;
}

void handleIncludeOpen()
{
    // Only handle INCLUDE_OPEN if in IDLE state
    if (inclusionState == InclusionState::IDLE && !device->isIncluded()) {
        std::vector<byte> publicKey{1, 2, 3, 4}; // Mock public key
        device->sendInclusionRequest(publicKey, 0);
        loginfo_ln("Sent inclusion request");
        inclusionState = InclusionState::WAITING_RESPONSE;
        stateTimer = millis();
    }
}

void handleIncludeResponse()
{
    if (inclusionState == InclusionState::WAITING_RESPONSE) {
        std::vector<byte> nonce{5, 6, 7, 8}; // Mock nonce
        device->sendInclusionConfirm(nonce);
        loginfo_ln("Sent inclusion confirm");
        inclusionState = InclusionState::CONFIRMING;
    }
}

void handleTimeout()
{
    if (inclusionState == InclusionState::WAITING_RESPONSE) {
        if (millis() - stateTimer >= RESPONSE_TIMEOUT) {
            loginfo_ln("Response timeout, returning to IDLE");
            inclusionState = InclusionState::IDLE;
        }
    }
}

void loop()
{
    if (!device) {
        logerr_ln("Device is null");
        return;
    }

    handleTimeout();
    device->run();

    if (messageReceived) {
        switch (lastMessageTopic) {
        case MessageTopic::INCLUDE_OPEN:
            handleIncludeOpen();
            break;

        case MessageTopic::INCLUDE_RESPONSE:
            handleIncludeResponse();
            break;

        case MessageTopic::INCLUDE_SUCCESS:
            loginfo_ln("Inclusion successful!");
            inclusionState = InclusionState::INCLUDED;
            break;
        }
        messageReceived = false;
        lastMessageTopic = MessageTopic::UNUSED;
    }
    delay(10);
}
