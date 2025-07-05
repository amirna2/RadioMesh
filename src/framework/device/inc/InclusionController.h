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
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionRequest();

    /**
     * @brief Send inclusion response (Hub only)
     * Sent after validating INCLUDE_REQUEST
     * @param packet The request packet received from the device
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionResponse(const RadioMeshPacket& packet);

    /**
     * @brief Send inclusion confirm (Device only)
     * Sent after processing INCLUDE_RESPONSE successfully
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionConfirm();

    /**
     * @brief Send inclusion success (Hub only)
     * Final message in inclusion sequence
     * Sent after validating INCLUDE_CONFIRM
     * @return RM_E_NONE on success, error code otherwise
     */
    int sendInclusionSuccess();

    /**
     * @brief Handle received inclusion message automatically
     * Routes to appropriate handler based on device type and message type
     * @param packet The received inclusion message packet
     * @return RM_E_NONE on success, error code otherwise
     */
    int handleInclusionMessage(const RadioMeshPacket& packet);

    /**
     * @brief Check for protocol timeouts and handle them
     * Should be called periodically from Device::run()
     * @return RM_E_NONE on success, error code otherwise
     */
    int checkProtocolTimeouts();

private:
    const std::string STATE_KEY = "is"; // inclusion state
    const std::string CTR_KEY = "mc";   // message counter
    const std::string SKEY = "sk";      // session key
    const std::string PRIV_KEY = "pk";  // device private key
    const std::string HUB_KEY = "hk";   // hub public key
    const size_t NONCE_SIZE = 4;

    RadioMeshDevice& device;
    DeviceInclusionState state;
    MeshDeviceType deviceType;
    bool inclusionModeEnabled{false};

    std::unique_ptr<DeviceStorage> storage;
    std::unique_ptr<KeyManager> keyManager;

    // Protocol state machine
    enum InclusionProtocolState {
        PROTOCOL_IDLE = 0,                  // Ready to start inclusion
        WAITING_FOR_REQUEST,                // Hub: Sent INCLUDE_OPEN, waiting for device request
        WAITING_FOR_RESPONSE,               // Device: Sent INCLUDE_REQUEST, waiting for hub response
        WAITING_FOR_CONFIRMATION,           // Hub: Sent INCLUDE_RESPONSE, waiting for confirmation
        WAITING_FOR_SUCCESS                 // Device: Sent INCLUDE_CONFIRM, waiting for success
    };

    InclusionProtocolState protocolState = PROTOCOL_IDLE;
    uint32_t stateStartTime = 0;            // When current state was entered
    uint8_t retryCount = 0;                 // Number of retries for current state

    // State machine configuration
    static const uint32_t BASE_TIMEOUT_MS = 60000;    // 60 seconds inclusion session timeout
    static const uint8_t MAX_RETRIES = 3;              // Maximum retry attempts (unused)
    static const uint32_t MAX_TOTAL_TIMEOUT_MS = 60000; // 60 seconds total timeout

    int initializeKeys();

    // State machine management
    void transitionToState(InclusionProtocolState newState);
    bool isStateTimedOut() const;
    uint32_t getStateTimeoutMs() const;
    void handleStateTimeout();
    void resetProtocolState();
    const char* getProtocolStateString(InclusionProtocolState state) const;

    // Used when sending INCLUDE_REQUEST
    int getPublicKey(std::vector<byte>& publicKey);
    // Used when processing INCLUDE_RESPONSE
    int handleHubKey(const std::vector<byte>& hubKey);
    // Used when processing INCLUDE_RESPONSE
    int handleSessionKey(const std::vector<byte>& encryptedKey);

    std::vector<byte> currentNonce; // Store current session nonce

    std::vector<byte> generateNonce()
    {
        std::vector<byte> nonce(NONCE_SIZE);
        // Use RadioMeshUtils::simpleRNG for entropy
        for (size_t i = 0; i < NONCE_SIZE; i++) {
            nonce[i] = RadioMeshUtils::simpleRNG(1);
        }
        return nonce;
    }

    bool verifyNonce(const std::vector<byte>& receivedNonce)
    {
        // Verify the received nonce matches our expected nonce
        if (receivedNonce.size() != currentNonce.size()) {
            return false;
        }
        return std::equal(receivedNonce.begin(), receivedNonce.end(), currentNonce.begin());
    }
};
