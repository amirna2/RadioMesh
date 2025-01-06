#include <common/inc/Logger.h>
#include <framework/device/inc/Device.h>
#include <framework/device/inc/InclusionController.h>

InclusionController::InclusionController(RadioMeshDevice& device) : device(device)
{
    deviceType = device.getDeviceType();

    initStorage();

    if (deviceType == MeshDeviceType::HUB) {
        state = DeviceInclusionState::INCLUDED;
    } else {
        // Standard device - try to load state
        DeviceInclusionState loadedState;
        int rc = storage->loadState(loadedState);

        if (rc == RM_E_NONE) {
            state = loadedState;
            loginfo_ln("Loaded inclusion state: %d", static_cast<int>(state));
        } else {
            state = DeviceInclusionState::NOT_INCLUDED;
            loginfo_ln("No stored state found, starting as NOT_INCLUDED");
        }
    }

    logdbg_ln("InclusionController created for device type %d", deviceType);
}

DeviceInclusionState InclusionController::getState() const
{
    return state;
}

bool InclusionController::canSendMessage(uint8_t topic) const
{
    // Allow all messages from HUB devices
    if (deviceType == MeshDeviceType::HUB) {
        logdbg_ln("Hub device can send any message");
        return true;
    }

    // Inclusion messages are allowed during inclusion process
    if (TopicUtils::isInclusionTopic(topic)) {
        return true;
    }
    // All other messages require device to be included
    return state == DeviceInclusionState::INCLUDED;
}

bool InclusionController::isInclusionModeEnabled() const
{
    return inclusionModeEnabled;
}

int InclusionController::enterInclusionMode()
{
    if (device.getDeviceType() != MeshDeviceType::HUB) {
        logerr_ln("Only HUB devices can enter inclusion mode");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    inclusionModeEnabled = true;

    return RM_E_NONE;
}

int InclusionController::exitInclusionMode()
{
    if (device.getDeviceType() != MeshDeviceType::HUB) {
        logerr_ln("Only HUB devices can exit inclusion mode");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    inclusionModeEnabled = false;

    return RM_E_NONE;
}

int InclusionController::sendInclusionOpen()
{
    if (!inclusionModeEnabled) {
        logerr_ln("Inclusion mode is not enabled");
        return RM_E_INVALID_STATE;
    }

    // Send empty broadcast
    std::vector<byte> emptyData;
    return device.sendData(MessageTopic::INCLUDE_OPEN, emptyData);
}

int InclusionController::sendInclusionRequest(const std::vector<byte>& publicKey,
                                              uint32_t messageCounter)
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion request");
        return RM_E_INVALID_DEVICE_TYPE;
    }
    std::vector<byte> payload;
    payload.insert(payload.end(), publicKey.begin(), publicKey.end());
    payload.insert(payload.end(), (byte*)&messageCounter,
                   (byte*)&messageCounter + sizeof(uint32_t));

    return device.sendData(MessageTopic::INCLUDE_REQUEST, payload);
}

int InclusionController::sendInclusionResponse(const std::vector<byte>& publicKey,
                                               const std::vector<byte>& nonce,
                                               uint32_t messageCounter)
{
    if (deviceType != MeshDeviceType::HUB) {
        logerr_ln("Only HUB devices can send inclusion response");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    std::vector<byte> payload;
    payload.insert(payload.end(), publicKey.begin(), publicKey.end());
    payload.insert(payload.end(), nonce.begin(), nonce.end());
    payload.insert(payload.end(), (byte*)&messageCounter,
                   (byte*)&messageCounter + sizeof(uint32_t));

    return device.sendData(MessageTopic::INCLUDE_RESPONSE, payload);
}

int InclusionController::sendInclusionConfirm(const std::vector<byte>& nonce)
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion confirm");
        return RM_E_INVALID_DEVICE_TYPE;
    }
    std::vector<byte> payload;
    payload.insert(payload.end(), nonce.begin(), nonce.end());
    return device.sendData(MessageTopic::INCLUDE_CONFIRM, payload);
}

int InclusionController::sendInclusionSuccess()
{
    if (deviceType != MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion success");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // TODO: Set Inclusion Success payload
    std::vector<byte> payload;
    return device.sendData(MessageTopic::INCLUDE_SUCCESS, payload);
}
