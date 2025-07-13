#pragma once

#include "DeviceStorage.h"
#include <common/inc/Definitions.h>
#include <memory>
#include <vector>

/**
 * @class KeyManager
 * @brief Manages cryptographic keys for RadioMesh devices
 *
 */
class KeyManager
{
public:
    static constexpr size_t PUBLIC_KEY_SIZE = 32; // Curve25519 public key
    static constexpr size_t PRIVATE_KEY_SIZE = 32;
    static constexpr size_t NETWORK_KEY_SIZE = 32;

    explicit KeyManager(DeviceStorage& storage) : storage(storage)
    {
    }

    // Key generation
    int generateKeyPair(std::vector<byte>& publicKey, std::vector<byte>& privateKey);
    int derivePublicKey(const std::vector<byte>& privateKey, std::vector<byte>& publicKey);

    // Network key generation and management
    int generateNetworkKey(std::vector<byte>& networkKey);
    int getCurrentNetworkKey(std::vector<byte>& networkKey);
    int setNetworkKey(const std::vector<byte>& networkKey);
    int initializeForHub();
    bool hasNetworkKey();

    // Network key operations
    int encryptNetworkKey(const std::vector<byte>& networkKey,
                          const std::vector<byte>& recipientPubKey,
                          std::vector<byte>& encryptedKey);
    int decryptNetworkKey(const std::vector<byte>& encryptedKey,
                          const std::vector<byte>& privateKey, std::vector<byte>& networkKey);

    // Key storage operations
    int loadPrivateKey(std::vector<byte>& privateKey);
    int persistPrivateKey(const std::vector<byte>& privateKey);
    int loadHubKey(std::vector<byte>& hubKey);
    int persistHubKey(const std::vector<byte>& hubKey);
    int loadNetworkKey(std::vector<byte>& networkKey);
    int persistNetworkKey(const std::vector<byte>& networkKey);

    // Key validation
    bool validatePublicKey(const std::vector<byte>& publicKey);
    bool validatePrivateKey(const std::vector<byte>& privateKey);
    bool validateNetworkKey(const std::vector<byte>& networkKey);

private:
    DeviceStorage& storage;

    // network key from storage
    std::vector<byte> networkKey; // Cached network key for quick access
    // Device keys from storage
    std::vector<byte> devicePrivateKey; // Device's private key for direct ECC
    std::vector<byte> devicePublicKey;  // Device's public key for direct ECC
};
