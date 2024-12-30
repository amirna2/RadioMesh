#include <Arduino.h>
#include <RadioMesh.h>

IDevice* device = nullptr;
const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x11, 0x22, 0x33, 0x44};
const std::string DEVICE_NAME = "MinimalDevice";

// Track received message
volatile uint8_t lastMessageTopic = MessageTopic::UNUSED;
volatile bool messageReceived = false;

void handleIncludeOpen()
{
    if (!device->isIncluded()) {
        std::vector<byte> publicKey{1, 2, 3, 4}; // Dummy key for now
        device->sendInclusionRequest(publicKey, 0);
        loginfo_ln("Sent inclusion request");
    }
}

void handleIncludeResponse()
{
    std::vector<byte> nonce{5, 6, 7, 8}; // Dummy nonce
    device->sendInclusionConfirm(nonce);
    loginfo_ln("Sent inclusion confirm");
}

void RxCallback(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        return;
    }
    lastMessageTopic = packet->topic;
    messageReceived = true;
}

void setup()
{
    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(LoraRadioPresets::HELTEC_WIFI_LORA_32_V3)
                 .withRxPacketCallback(RxCallback)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::STANDARD);
}

void loop()
{
    if (!device)
        return;

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
            break;
        }
        messageReceived = false;
        lastMessageTopic = MessageTopic::UNUSED;
    }
}
