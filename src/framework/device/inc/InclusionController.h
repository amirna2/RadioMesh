#pragma once

#include "DeviceStorage.h"
#include "KeyManager.h"
#include <common/inc/Definitions.h>
#include <vector>

class RadioMeshDevice;

/**
 * @class InclusionController
 * @brief Controls the device inclusion sequence into a RadioMesh network
 *
 * The inclusion sequence follows this order:
 * 1. Hub is put in inclusion mode (by user/application)
 * 2. Hub broadcasts INCLUDE_OPEN (unencrypted)
 * 3. Device sends INCLUDE_REQUEST (with public key, initial counter)
 * 4. Hub sends INCLUDE_RESPONSE (with hub public key, encrypted session key)
 * 5. Device sends INCLUDE_CONFIRM (encrypted nonce)
 * 6. Hub sends INCLUDE_SUCCESS
 */
class InclusionController
{
public:
    explicit InclusionController(RadioMeshDevice& device);

    /**
     * @brief Get current device inclusion state
     * @return Current DeviceInclusionState
     */
    DeviceInclusionState getState() const;

    /**
     * @brief Check if device can send message type based on inclusion state
     * @param topic Message topic to check
     * @return true if message can be sent
     */
    bool canSendMessage(uint8_t topic) const;

    // Hub inclusion mode control
    /**
     * @brief Enter inclusion mode (Hub only)
     * @return RM_E_NONE on success, error code otherwise
     */
    int enterInclusionMode();

    /**
     * @brief Exit inclusion mode (Hub only)
     * @return RM_E_NONE on success, error code otherwise
     */
    int exitInclusionMode();

    /**
     * @brief Check if hub is in inclusion mode
     * @return true if inclusion mode enabled
     */
    bool isInclusionModeEnabled() const;

    // Inclusion sequence messages
    /**
     * @brief Send inclusion open message (Hub only)
     * Hub must be in inclusion mode
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionOpen();

    /**
     * @brief Send inclusion request (Device only)
     * Sent after receiving INCLUDE_OPEN
     * @param publicKey Device's public key
     * @param messageCounter Initial message counter
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionRequest(const std::vector<byte>& publicKey, uint32_t messageCounter);

    /**
     * @brief Send inclusion response (Hub only)
     * Sent after validating INCLUDE_REQUEST
     * @param publicKey Hub's public key
     * @param nonce Random nonce for session
     * @param messageCounter Hub's initial counter
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionResponse(const std::vector<byte>& publicKey, const std::vector<byte>& nonce,
                              uint32_t messageCounter);

    /**
     * @brief Send inclusion confirm (Device only)
     * Sent after processing INCLUDE_RESPONSE successfully
     * @param nonce Encrypted nonce value
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionConfirm(const std::vector<byte>& nonce);

    /**
     * @brief Send inclusion success (Hub only)
     * Final message in inclusion sequence
     * Sent after validating INCLUDE_CONFIRM
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionSuccess();

private:
    const std::string STATE_KEY = "is"; // inclusion state
    const std::string CTR_KEY = "mc";   // message counter
    const std::string SKEY = "sk";      // session key
    const std::string PRIV_KEY = "pk";  // device private key
    const std::string HUB_KEY = "hk";   // hub public key

    RadioMeshDevice& device;
    DeviceInclusionState state;
    MeshDeviceType deviceType;
    bool inclusionModeEnabled{false};

    std::unique_ptr<DeviceStorage> storage;
    std::unique_ptr<KeyManager> keyManager;

    int initializeKeys();
};
