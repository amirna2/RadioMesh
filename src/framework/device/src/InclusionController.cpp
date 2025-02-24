#include <common/inc/Logger.h>
#include <framework/device/inc/Device.h>
#include <framework/device/inc/InclusionController.h>

InclusionController::InclusionController(RadioMeshDevice& device) : device(device)
{
    deviceType = device.getDeviceType();

    storage = std::make_unique<DeviceStorage>(storage);
    keyManager = std::make_unique<KeyManager>(*storage);

    if (deviceType == MeshDeviceType::HUB) {
        state = DeviceInclusionState::INCLUDED;
    } else {
        int rc = initializeKeys();
        if (rc != RM_E_NONE) {
            logerr_ln("Failed to initialize device keys");
            // TODO: We should probably handle this failure case better
            // Maybe set device to a failed state?
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
    }

    logdbg_ln("InclusionController created for device type %d", deviceType);
}

int InclusionController::initializeKeys()
{
    std::vector<byte> privateKey;

    // Try to load existing private key
    int rc = keyManager->loadPrivateKey(privateKey);

    if (rc == RM_E_STORAGE_KEY_NOT_FOUND) {
        loginfo_ln("No existing private key found, generating new key pair");

        std::vector<byte> publicKey;
        rc = keyManager->generateKeyPair(publicKey, privateKey);
        if (rc != RM_E_NONE) {
            logerr_ln("Failed to generate key pair: %d", rc);
            return rc;
        }

        rc = keyManager->persistPrivateKey(privateKey);
        if (rc != RM_E_NONE) {
            logerr_ln("Failed to persist private key: %d", rc);
            return rc;
        }

        loginfo_ln("Generated and stored new key pair");
    } else if (rc != RM_E_NONE) {
        logerr_ln("Error loading private key: %d", rc);
        return rc;
    } else {
        loginfo_ln("Loaded existing private key");
    }

    return RM_E_NONE;
}

int InclusionController::getPublicKey(std::vector<byte>& publicKey)
{
    std::vector<byte> privateKey;
    int rc = keyManager->loadPrivateKey(privateKey);
    if (rc != RM_E_NONE) {
        return rc;
    }

    // Re-derive public key from private key
    // TODO: This could be cached to avoid re-derivation
    std::vector<byte> newPublicKey;
    rc = keyManager->generateKeyPair(publicKey, privateKey);
    return rc;
}

int InclusionController::handleHubKey(const std::vector<byte>& hubKey)
{
    loginfo_ln("Storing hub public key");
    return keyManager->persistHubKey(hubKey);
}

int InclusionController::handleSessionKey(const std::vector<byte>& encryptedKey)
{
    std::vector<byte> privateKey;
    int rc = keyManager->loadPrivateKey(privateKey);
    if (rc != RM_E_NONE) {
        return rc;
    }

    std::vector<byte> sessionKey;
    rc = keyManager->decryptSessionKey(encryptedKey, privateKey, sessionKey);
    if (rc != RM_E_NONE) {
        return rc;
    }

    return keyManager->persistSessionKey(sessionKey);
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

int InclusionController::sendInclusionRequest()
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion request");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    std::vector<byte> publicKey;
    int rc = getPublicKey(publicKey);
    if (rc != RM_E_NONE) {
        return rc;
    }

    // Build payload as before but now with real public key
    std::vector<byte> payload;
    payload.insert(payload.end(), publicKey.begin(), publicKey.end());
    return device.sendData(MessageTopic::INCLUDE_REQUEST, payload);
}

int InclusionController::sendInclusionResponse(const RadioMeshPacket& packet)
{
    if (deviceType != MeshDeviceType::HUB) {
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // 1. Generate new session key
    std::vector<byte> sessionKey;
    int rc = keyManager->generateSessionKey(sessionKey);
    if (rc != RM_E_NONE)
        return rc;

    // 2. Encrypt session key with device's public key

    // Get device public key from the packet
    std::vector<byte> devicePublicKey;
    devicePublicKey = packet.packetData;
    if (devicePublicKey.size() != KeyManager::PUBLIC_KEY_SIZE) {
        logerr_ln("Invalid public key size: %d", devicePublicKey.size());
        return RM_E_INVALID_LENGTH;
    }
    std::vector<byte> encryptedKey;
    rc = keyManager->encryptSessionKey(sessionKey, devicePublicKey, encryptedKey);
    if (rc != RM_E_NONE)
        return rc;

    // 3. Build response payload
    std::vector<byte> payload;

    // Add hub public key
    std::vector<byte> hubKey;
    rc = getPublicKey(hubKey);
    if (rc != RM_E_NONE)
        return rc;

    payload.insert(payload.end(), hubKey.begin(), hubKey.end());
    payload.insert(payload.end(), encryptedKey.begin(), encryptedKey.end());

    // TODO: Generate new random nonce
    std::vector<byte> nonce = {0, 0, 0, 0};
    payload.insert(payload.end(), nonce.begin(), nonce.end());

    return device.sendData(MessageTopic::INCLUDE_RESPONSE, payload);
}

int InclusionController::sendInclusionConfirm()
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion confirm");
        return RM_E_INVALID_DEVICE_TYPE;
    }
    std::vector<byte> payload;
    // inrement nonce from response
    // TODO: Actually use the nonce from the response
    std::vector<byte> nonce = {0, 0, 0, 1};

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
