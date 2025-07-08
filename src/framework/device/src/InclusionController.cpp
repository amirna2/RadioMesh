#include <common/inc/Logger.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <framework/device/inc/Device.h>
#include <framework/device/inc/InclusionController.h>

InclusionController::InclusionController(RadioMeshDevice& device) : device(device)
{
    deviceType = device.getDeviceType();

    // Get the byte storage from the device
    auto* byteStorage = device.getByteStorage();
    if (byteStorage) {
        storage = std::make_unique<DeviceStorage>(byteStorage);
        keyManager = std::make_unique<KeyManager>(*storage);

        // FOR TESTING ONLY
        byteStorage->clear(); // Clear storage on initialization
    } else {
        logerr_ln("No byte storage available for InclusionController");
        // TODO: Handle this error case properly
    }

    // All devices need keys for inclusion protocol
    std::vector<byte> privateKey, publicKey;
    int rc = initializeKeys(privateKey, publicKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to initialize device keys");
        // TODO: We should probably handle this failure case better
        // Maybe set device to a failed state?
    } else {
    }

    // Initialize network key manager for hub devices
    if (deviceType == MeshDeviceType::HUB) {
        rc = keyManager->initializeForHub();
        if (rc != RM_E_NONE) {
            logerr_ln("Failed to initialize hub network key");
        }
    }

    if (deviceType == MeshDeviceType::HUB) {
        state = DeviceInclusionState::INCLUDED;
    } else {
        if (rc == RM_E_NONE) {
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

int InclusionController::initializeKeys(std::vector<byte>& privateKey, std::vector<byte>& publicKey)
{
    // Try to load existing private key
    int rc = keyManager->loadPrivateKey(privateKey);

    if (rc == RM_E_STORAGE_KEY_NOT_FOUND) {
        loginfo_ln("No existing private key found, generating new key pair");

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
        loginfo_ln("Deriving public key from existing private key");

        // Derive public key from existing private key without regenerating
        rc = keyManager->derivePublicKey(privateKey, publicKey);
        if (rc != RM_E_NONE) {
            logerr_ln("Failed to derive public key from private key: %d", rc);
            return rc;
        }
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
    rc = keyManager->derivePublicKey(privateKey, publicKey);
    return rc;
}

int InclusionController::handleHubKey(const std::vector<byte>& hubKey)
{
    loginfo_ln("Storing hub public key");
    return keyManager->persistHubKey(hubKey);
}

int InclusionController::handleNetworkKey(const std::vector<byte>& encryptedKey)
{
    std::vector<byte> privateKey;
    int rc = keyManager->loadPrivateKey(privateKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to load private key for network key decryption: %d", rc);
        return rc;
    }

    std::vector<byte> networkKey;
    // Decrypt the network key using KeyManager's ECC decryption
    rc = keyManager->decryptNetworkKey(encryptedKey, privateKey, networkKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to decrypt network key: %d", rc);
        return rc;
    }

    rc = keyManager->setNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to store network key: %d", rc);
        return rc;
    }

    loginfo_ln("Successfully stored network key");

    // Configure EncryptionService with the network key for INCLUDE_CONFIRM encryption
    if (device.getEncryptionService()) {
        device.getEncryptionService()->setNetworkKey(networkKey);
        loginfo_ln("Configured EncryptionService with network key");
    }

    return RM_E_NONE;
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
    resetProtocolState();

    return RM_E_NONE;
}

int InclusionController::sendInclusionOpen()
{
    if (!inclusionModeEnabled) {
        logerr_ln("Inclusion mode is not enabled");
        return RM_E_INVALID_STATE;
    }

    // Start the protocol state machine
    transitionToState(WAITING_FOR_REQUEST);

    // Get hub public key to include in broadcast
    std::vector<byte> hubPublicKey;
    int rc = getPublicKey(hubPublicKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to get hub public key: %d", rc);
        return rc;
    }

    loginfo_ln("Broadcasting INCLUDE_OPEN with hub public key");
    return device.sendData(MessageTopic::INCLUDE_OPEN, hubPublicKey);
}

int InclusionController::sendInclusionRequest()
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion request");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // Ensure we have the hub's public key from INCLUDE_OPEN
    if (tempHubPublicKey.empty()) {
        logerr_ln("No hub public key available for encryption");
        return RM_E_INVALID_STATE;
    }

    std::vector<byte> devicePublicKey;
    int rc = getPublicKey(devicePublicKey);
    if (rc != RM_E_NONE) {
        return rc;
    }

    // Build payload: device public key ONLY
    std::vector<byte> payload;

    // Add device public key (64 bytes)
    payload.insert(payload.end(), devicePublicKey.begin(), devicePublicKey.end());

    loginfo_ln("Sending INCLUDE_REQUEST with device public key");
    return device.sendData(MessageTopic::INCLUDE_REQUEST, payload);
}

int InclusionController::sendInclusionResponse(const RadioMeshPacket& packet)
{
    if (deviceType != MeshDeviceType::HUB) {
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // 1. Get current network key from KeyManager
    std::vector<byte> networkKey;
    int rc = keyManager->getCurrentNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to get network key: %d", rc);
        return rc;
    }

    loginfo_ln("Distributing network key to device");

    // 2. Extract device public key from INCLUDE_REQUEST
    // Device.handleReceivedData() already decrypted the payload via EncryptionService
    const std::vector<byte>& decryptedPayload = packet.packetData;

    // Expected payload: device_public_key(64) = 64 bytes
    const size_t expectedSize = 64;
    if (decryptedPayload.size() != expectedSize) {
        logerr_ln("Invalid decrypted payload size: %d, expected: %d", decryptedPayload.size(),
                  expectedSize);
        return RM_E_INVALID_LENGTH;
    }

    // Extract device public key (all 64 bytes)
    std::vector<byte> devicePublicKey = decryptedPayload;

    // Keep the device public key for later use
    if (device.getEncryptionService()) {
        device.getEncryptionService()->setTempDevicePublicKey(devicePublicKey);
        logdbg_ln("Configured EncryptionService with device public key for INCLUDE_RESPONSE");
    }

    // Set the network key in the EncryptionService for encryption of all outgoing messages
    // starting from INCLUDE_CONFIRM and INCLUDE_SUCCESS. All non-inclusion messages
    // will also use this key for encryption.
    if (device.getEncryptionService()) {
        device.getEncryptionService()->setNetworkKey(networkKey);
        loginfo_ln("Configured hub EncryptionService with network key for INCLUDE_CONFIRM");
    }

    loginfo_ln("Hub distributing network key to device");

    // 4. Build response payload: [plain_network_key][plain_nonce]
    // PacketRouter will encrypt entire payload via EncryptionService
    std::vector<byte> payload;

    // Add plain network key (PacketRouter will encrypt via EncryptionService)
    payload.insert(payload.end(), networkKey.begin(), networkKey.end());

    // Generate random nonce and store it for verification
    currentNonce = generateNonce();
    logdbg_ln("Generated nonce: %s",
              RadioMeshUtils::convertToHex(currentNonce.data(), currentNonce.size()).c_str());

    // For now, send nonce unencrypted (will be encrypted by device with network key)
    // The device will encrypt it after receiving and verifying the network key
    payload.insert(payload.end(), currentNonce.begin(), currentNonce.end());

    return device.sendData(MessageTopic::INCLUDE_RESPONSE, payload);
}

int InclusionController::sendInclusionConfirm()
{
    if (deviceType == MeshDeviceType::HUB) {
        logerr_ln("HUB cannot send inclusion confirm");
        return RM_E_INVALID_DEVICE_TYPE;
    }

    // Increment the nonce
    uint32_t nonceValue = RadioMeshUtils::bytesToNumber<uint32_t>(currentNonce);
    logdbg_ln("Original nonce value: %u (0x%08X)", nonceValue, nonceValue);
    nonceValue++;
    std::vector<byte> incrementedNonce = RadioMeshUtils::numberToBytes(nonceValue);

    logdbg_ln(
        "Incremented nonce: %u (0x%08X), bytes: %s", nonceValue, nonceValue,
        RadioMeshUtils::convertToHex(incrementedNonce.data(), incrementedNonce.size()).c_str());

    // Send incremented nonce - EncryptionService will automatically encrypt with network key
    logdbg_ln(
        "Sending incremented nonce: %s",
        RadioMeshUtils::convertToHex(incrementedNonce.data(), incrementedNonce.size()).c_str());

    return device.sendData(MessageTopic::INCLUDE_CONFIRM, incrementedNonce);
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

int InclusionController::handleInclusionMessage(const RadioMeshPacket& packet)
{
    logdbg_ln("Handling inclusion message, topic: 0x%02X, device type: %d", packet.topic,
              static_cast<int>(deviceType));

    // Handle messages based on device type
    if (deviceType == MeshDeviceType::HUB) {
        // Hub handles requests from devices
        switch (packet.topic) {
        case MessageTopic::INCLUDE_REQUEST:
            if (isInclusionModeEnabled() && protocolState == WAITING_FOR_REQUEST) {
                loginfo_ln("Hub received INCLUDE_REQUEST from device");
                transitionToState(WAITING_FOR_CONFIRMATION);
                // TODO: Process the request and send response
                return sendInclusionResponse(packet);
            } else {
                logwarn_ln("Hub received INCLUDE_REQUEST but not ready (mode: %s, state: %s)",
                           isInclusionModeEnabled() ? "enabled" : "disabled",
                           getProtocolStateString(protocolState));
                return RM_E_INVALID_STATE;
            }
            break;

        case MessageTopic::INCLUDE_CONFIRM:
            if (protocolState == WAITING_FOR_CONFIRMATION) {
                loginfo_ln("Hub received INCLUDE_CONFIRM from device");

                // Device.handleReceivedData() already decrypted INCLUDE_CONFIRM via
                // EncryptionService
                const std::vector<byte>& decryptedNonce = packet.packetData;
                logdbg_ln("Received decrypted nonce: %s",
                          RadioMeshUtils::convertToHex(decryptedNonce.data(), decryptedNonce.size())
                              .c_str());

                // Verify the nonce is incremented by 1
                uint32_t originalNonceValue = RadioMeshUtils::bytesToNumber<uint32_t>(currentNonce);
                uint32_t receivedNonceValue =
                    RadioMeshUtils::bytesToNumber<uint32_t>(decryptedNonce);

                logdbg_ln("Decrypted nonce bytes: %s",
                          RadioMeshUtils::convertToHex(decryptedNonce.data(), decryptedNonce.size())
                              .c_str());
                logdbg_ln("Original nonce: %u (0x%08X), Received nonce: %u (0x%08X)",
                          originalNonceValue, originalNonceValue, receivedNonceValue,
                          receivedNonceValue);

                if (receivedNonceValue != originalNonceValue + 1) {
                    logerr_ln("Nonce verification failed! Expected %u, got %u",
                              originalNonceValue + 1, receivedNonceValue);
                    return RM_E_INVALID_PARAM;
                }

                loginfo_ln("Nonce verified successfully!");
                transitionToState(PROTOCOL_IDLE);
                return sendInclusionSuccess();
            } else {
                logwarn_ln("Hub received INCLUDE_CONFIRM in wrong state: %s",
                           getProtocolStateString(protocolState));
                return RM_E_INVALID_STATE;
            }
            break;

        default:
            logwarn_ln("Hub received unexpected inclusion message: 0x%02X", packet.topic);
            break;
        }
    } else {
        // Standard device handles messages from hub
        switch (packet.topic) {
        case MessageTopic::INCLUDE_OPEN:
            if (state == DeviceInclusionState::NOT_INCLUDED && protocolState == PROTOCOL_IDLE) {
                loginfo_ln("Device received INCLUDE_OPEN, starting inclusion");

                // Extract hub public key from INCLUDE_OPEN message
                if (packet.packetData.size() != KeyManager::PUBLIC_KEY_SIZE) {
                    logerr_ln("Invalid hub public key size in INCLUDE_OPEN: %d",
                              packet.packetData.size());
                    return RM_E_INVALID_LENGTH;
                }

                // Store hub public key temporarily for encrypting INCLUDE_REQUEST
                tempHubPublicKey = packet.packetData;
                logdbg_ln("Received hub public key: %s",
                          RadioMeshUtils::convertToHex(
                              tempHubPublicKey.data(),
                              std::min(8U, static_cast<unsigned int>(tempHubPublicKey.size())))
                              .c_str());

                // Configure EncryptionService with hub's public key for INCLUDE_REQUEST encryption
                if (device.getEncryptionService()) {
                    device.getEncryptionService()->setHubPublicKey(tempHubPublicKey);
                    logdbg_ln("Configured EncryptionService with hub public key");
                }

                state = DeviceInclusionState::INCLUSION_PENDING;
                storage->persistState(state);
                transitionToState(WAITING_FOR_RESPONSE);
                return sendInclusionRequest();
            } else {
                logdbg_ln("Device received INCLUDE_OPEN but not ready (state: %s, protocol: %s)",
                          state == DeviceInclusionState::INCLUDED ? "INCLUDED" : "PENDING",
                          getProtocolStateString(protocolState));
            }
            break;

        case MessageTopic::INCLUDE_RESPONSE:
            if (state == DeviceInclusionState::INCLUSION_PENDING &&
                protocolState == WAITING_FOR_RESPONSE) {
                loginfo_ln("Device received INCLUDE_RESPONSE from hub");

                // Parse the response payload:
                // [network_key][nonce]
                const std::vector<byte>& payload = packet.packetData;
                const size_t NETWORK_KEY_SIZE = 32;
                const size_t NONCE_SIZE = 4;

                if (payload.size() < NETWORK_KEY_SIZE + NONCE_SIZE) {
                    logerr_ln("INCLUDE_RESPONSE payload too small: %d", payload.size());
                    return RM_E_INVALID_LENGTH;
                }

                // Extract plain network key (Device.handleReceivedData already decrypted via
                // EncryptionService)
                std::vector<byte> networkKey(payload.begin(), payload.begin() + NETWORK_KEY_SIZE);

                // Extract plain nonce
                currentNonce = std::vector<byte>(payload.begin() + NETWORK_KEY_SIZE,
                                                 payload.begin() + NETWORK_KEY_SIZE + NONCE_SIZE);
                logdbg_ln(
                    "Received nonce: %s",
                    RadioMeshUtils::convertToHex(currentNonce.data(), currentNonce.size()).c_str());

                // Store the network key directly
                int rc = keyManager->setNetworkKey(networkKey);
                if (rc != RM_E_NONE) {
                    logerr_ln("Failed to store network key: %d", rc);
                    return rc;
                }

                loginfo_ln("Successfully stored network key");

                // Configure EncryptionService with the network key for INCLUDE_CONFIRM encryption
                if (device.getEncryptionService()) {
                    device.getEncryptionService()->setNetworkKey(networkKey);
                    loginfo_ln("Configured EncryptionService with network key");
                }

                transitionToState(WAITING_FOR_SUCCESS);
                return sendInclusionConfirm();
            } else {
                logwarn_ln(
                    "Device received INCLUDE_RESPONSE in wrong state (device: %d, protocol: %s)",
                    static_cast<int>(state), getProtocolStateString(protocolState));
            }
            break;

        case MessageTopic::INCLUDE_SUCCESS:
            if (state == DeviceInclusionState::INCLUSION_PENDING &&
                protocolState == WAITING_FOR_SUCCESS) {
                loginfo_ln("Device received INCLUDE_SUCCESS, inclusion complete!");
                state = DeviceInclusionState::INCLUDED;
                storage->persistState(state);
                transitionToState(PROTOCOL_IDLE);

                std::vector<byte> networkKey;
                int rc = keyManager->getCurrentNetworkKey(networkKey);
                if (rc == RM_E_NONE) {
                    SecurityParams newParams;
                    newParams.method = SecurityMethod::AES;
                    newParams.key = networkKey;
                    newParams.iv = std::vector<byte>(16, 0);

                    rc = device.updateSecurityParams(newParams);
                    if (rc == RM_E_NONE) {
                        loginfo_ln("Applied network key to crypto system");
                    } else {
                        logerr_ln("Failed to apply network key: %d", rc);
                    }
                } else {
                    logerr_ln("Failed to load network key: %d", rc);
                }
            } else {
                logwarn_ln(
                    "Device received INCLUDE_SUCCESS in wrong state (device: %d, protocol: %s)",
                    static_cast<int>(state), getProtocolStateString(protocolState));
            }
            break;

        default:
            logwarn_ln("Device received unexpected inclusion message: 0x%02X", packet.topic);
            break;
        }
    }

    return RM_E_NONE;
}

void InclusionController::transitionToState(InclusionProtocolState newState)
{
    if (protocolState != newState) {
        loginfo_ln("Inclusion protocol: %s -> %s", getProtocolStateString(protocolState),
                   getProtocolStateString(newState));

        protocolState = newState;
        stateStartTime = millis();
        retryCount = 0;
    }
}

bool InclusionController::isStateTimedOut() const
{
    if (protocolState == PROTOCOL_IDLE) {
        return false;
    }

    uint32_t elapsed = millis() - stateStartTime;
    return elapsed > getStateTimeoutMs();
}

uint32_t InclusionController::getStateTimeoutMs() const
{
    return BASE_TIMEOUT_MS;
}

void InclusionController::handleStateTimeout()
{
    loginfo_ln("Inclusion session timeout (60s), stopping inclusion mode");
    inclusionModeEnabled = false;
    resetProtocolState();
}

void InclusionController::resetProtocolState()
{
    loginfo_ln("Resetting inclusion protocol state");
    transitionToState(PROTOCOL_IDLE);
    retryCount = 0;

    // If this was a device trying to join, reset to NOT_INCLUDED
    if (deviceType == MeshDeviceType::STANDARD &&
        state == DeviceInclusionState::INCLUSION_PENDING) {
        state = DeviceInclusionState::NOT_INCLUDED;
        storage->persistState(state);
    }
}

int InclusionController::checkProtocolTimeouts()
{
    if (isStateTimedOut()) {
        handleStateTimeout();
    }
    return RM_E_NONE;
}

const char* InclusionController::getProtocolStateString(InclusionProtocolState state) const
{
    switch (state) {
    case PROTOCOL_IDLE:
        return "IDLE";
    case WAITING_FOR_REQUEST:
        return "WAITING_FOR_REQUEST";
    case WAITING_FOR_RESPONSE:
        return "WAITING_FOR_RESPONSE";
    case WAITING_FOR_CONFIRMATION:
        return "WAITING_FOR_CONFIRMATION";
    case WAITING_FOR_SUCCESS:
        return "WAITING_FOR_SUCCESS";
    default:
        return "UNKNOWN";
    }
}

int InclusionController::loadAndApplyNetworkKey()
{
    // For hub devices, always apply network key
    // For standard devices, only apply if included
    if (deviceType != MeshDeviceType::HUB && state != DeviceInclusionState::INCLUDED) {
        logdbg_ln("Device not included, no network key to load");
        return RM_E_INVALID_STATE;
    }

    std::vector<byte> networkKey;
    int rc = keyManager->getCurrentNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to load network key: %d", rc);
        return rc;
    }

    SecurityParams params;
    params.method = SecurityMethod::AES;
    params.key = networkKey;
    params.iv = std::vector<byte>(16, 0);

    rc = device.updateSecurityParams(params);
    if (rc == RM_E_NONE) {
        loginfo_ln("Applied stored network key to crypto system");
    } else {
        logerr_ln("Failed to apply network key: %d", rc);
    }

    return rc;
}

int InclusionController::getDevicePublicKey(std::vector<byte>& publicKey)
{
    return getPublicKey(publicKey);
}
