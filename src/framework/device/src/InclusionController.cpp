#include <common/inc/Logger.h>
#include <framework/device/inc/Device.h>
#include <framework/device/inc/InclusionController.h>

InclusionController::InclusionController(RadioMeshDevice& device) : device(device)
{
    deviceType = device.getDeviceType();
    if (deviceType == MeshDeviceType::HUB) {
        state = DeviceInclusionState::INCLUDED;
    } else {
        state = DeviceInclusionState::NOT_INCLUDED;
    }
}

DeviceInclusionState InclusionController::getState() const
{
    return state;
}

bool InclusionController::canSendMessage(uint8_t topic) const
{
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
    // TODO: Set Inclusion Request payload
    std::vector<byte> emptyData;
    return device.sendData(MessageTopic::INCLUDE_REQUEST, emptyData);
}

int InclusionController::sendInclusionResponse(const std::vector<byte>& publicKey,
                                               const std::vector<byte>& nonce,
                                               uint32_t messageCounter)
{
    if (deviceType != MeshDeviceType::HUB) {
        logerr_ln("Only HUB devices can send inclusion response");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // TODO: Set Inclusion Response payload
    std::vector<byte> emptyData;
    return device.sendData(MessageTopic::INCLUDE_RESPONSE, emptyData);
}

int InclusionController::sendInclusionConfirm(const std::vector<byte>& nonce)
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion confirm");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // TODO: Set Inclusion Confirm payload
    std::vector<byte> emptyData;
    return device.sendData(MessageTopic::INCLUDE_CONFIRM, emptyData);
}

int InclusionController::sendInclusionSuccess()
{
    if (deviceType != MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion success");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // TODO: Set Inclusion Success payload
    std::vector<byte> emptyData;
    return device.sendData(MessageTopic::INCLUDE_SUCCESS, emptyData);
}
