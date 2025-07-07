#pragma once

#include "DeviceStorage.h"
#include <common/inc/Definitions.h>
#include <memory>
#include <vector>

class NetworkKeyManager
{
public:
    static constexpr size_t NETWORK_KEY_SIZE = 32;
    static constexpr uint32_t INITIAL_VERSION = 1;

    explicit NetworkKeyManager(DeviceStorage& storage) : storage(storage)
    {
    }

    // Network key generation (for hubs)
    int generateNetworkKey(std::vector<byte>& networkKey);
    
    // Network key operations
    int getCurrentNetworkKey(std::vector<byte>& networkKey);
    int setNetworkKey(const std::vector<byte>& networkKey);
    
    // Network key encryption/decryption for distribution
    int encryptNetworkKey(const std::vector<byte>& networkKey,
                          const std::vector<byte>& recipientPubKey,
                          std::vector<byte>& encryptedKey);
    int decryptNetworkKey(const std::vector<byte>& encryptedKey,
                          const std::vector<byte>& privateKey,
                          std::vector<byte>& networkKey);

    // Storage operations
    int loadNetworkKey(std::vector<byte>& networkKey);
    int persistNetworkKey(const std::vector<byte>& networkKey);
    
    // Key validation
    bool validateNetworkKey(const std::vector<byte>& networkKey);
    
    // Initialization for hubs
    int initializeForHub();
    
    // Check if network key is available
    bool hasNetworkKey();

private:
    DeviceStorage& storage;
};