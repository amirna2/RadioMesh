#pragma once

#include "DeviceStorage.h"
#include <common/inc/Definitions.h>
#include <memory>
#include <vector>

class KeyManager
{
public:
    explicit KeyManager(DeviceStorage& storage) : storage(storage)
    {
    }

    // Key generation
    int generateKeyPair(std::vector<byte>& publicKey, std::vector<byte>& privateKey);

    // Session key operations
    int generateSessionKey(std::vector<byte>& sessionKey);
    int encryptSessionKey(const std::vector<byte>& sessionKey,
                          const std::vector<byte>& recipientPubKey,
                          std::vector<byte>& encryptedKey);
    int decryptSessionKey(const std::vector<byte>& encryptedKey,
                          const std::vector<byte>& privateKey, std::vector<byte>& sessionKey);

    // Key storage operations
    int loadPrivateKey(std::vector<byte>& privateKey);
    int persistPrivateKey(const std::vector<byte>& privateKey);
    int loadHubKey(std::vector<byte>& hubKey);
    int persistHubKey(const std::vector<byte>& hubKey);
    int loadSessionKey(std::vector<byte>& sessionKey);
    int persistSessionKey(const std::vector<byte>& sessionKey);

    // Key validation
    bool validatePublicKey(const std::vector<byte>& publicKey);
    bool validatePrivateKey(const std::vector<byte>& privateKey);
    bool validateSessionKey(const std::vector<byte>& sessionKey);

private:
    DeviceStorage& storage;
    static constexpr size_t PUBLIC_KEY_SIZE = 32; // Example size, adjust based on chosen curve
    static constexpr size_t PRIVATE_KEY_SIZE = 32;
    static constexpr size_t SESSION_KEY_SIZE = 32;
};
