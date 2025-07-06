#include <common/inc/Logger.h>
#include <framework/device/inc/NetworkKeyManager.h>

int NetworkKeyManager::generateNetworkKey(std::vector<byte>& networkKey)
{
    networkKey.resize(NETWORK_KEY_SIZE);
    
    // Generate random network key
    for (size_t i = 0; i < NETWORK_KEY_SIZE; i++) {
        networkKey[i] = random(256);
    }
    
    loginfo_ln("Generated new network key");
    return RM_E_NONE;
}

int NetworkKeyManager::getCurrentNetworkKey(std::vector<byte>& networkKey)
{
    if (!cacheValid) {
        int rc = refreshCache();
        if (rc != RM_E_NONE) {
            return rc;
        }
    }
    
    networkKey = cachedNetworkKey;
    return RM_E_NONE;
}

uint32_t NetworkKeyManager::getCurrentNetworkKeyVersion()
{
    if (!cacheValid) {
        refreshCache();
    }
    
    return cachedVersion;
}

int NetworkKeyManager::setNetworkKey(const std::vector<byte>& networkKey, uint32_t version)
{
    if (!validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }
    
    int rc = persistNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    rc = persistNetworkKeyVersion(version);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    // Update cache
    cachedNetworkKey = networkKey;
    cachedVersion = version;
    cacheValid = true;
    
    loginfo_ln("Set network key version %u", version);
    return RM_E_NONE;
}

int NetworkKeyManager::encryptNetworkKey(const std::vector<byte>& networkKey,
                                         const std::vector<byte>& recipientPubKey,
                                         std::vector<byte>& encryptedKey)
{
    // TODO: Implement ECIES encryption
    // For now just XOR with recipient public key as placeholder
    if (!validateNetworkKey(networkKey) || recipientPubKey.size() != 32) {
        return RM_E_INVALID_PARAM;
    }
    
    encryptedKey.resize(networkKey.size());
    for (size_t i = 0; i < networkKey.size(); i++) {
        encryptedKey[i] = networkKey[i] ^ recipientPubKey[i % 32];
    }
    
    return RM_E_NONE;
}

int NetworkKeyManager::decryptNetworkKey(const std::vector<byte>& encryptedKey,
                                         const std::vector<byte>& privateKey,
                                         std::vector<byte>& networkKey)
{
    // TODO: Implement ECIES decryption
    // For now just XOR with private key as placeholder
    if (privateKey.size() != 32) {
        return RM_E_INVALID_PARAM;
    }
    
    networkKey.resize(encryptedKey.size());
    for (size_t i = 0; i < encryptedKey.size(); i++) {
        networkKey[i] = encryptedKey[i] ^ privateKey[i % 32];
    }
    
    return RM_E_NONE;
}

int NetworkKeyManager::loadNetworkKey(std::vector<byte>& networkKey)
{
    return storage.loadNetworkKey(networkKey);
}

int NetworkKeyManager::persistNetworkKey(const std::vector<byte>& networkKey)
{
    if (!validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }
    
    invalidateCache();
    return storage.persistNetworkKey(networkKey);
}

int NetworkKeyManager::loadNetworkKeyVersion(uint32_t& version)
{
    return storage.loadNetworkKeyVersion(version);
}

int NetworkKeyManager::persistNetworkKeyVersion(uint32_t version)
{
    invalidateCache();
    return storage.persistNetworkKeyVersion(version);
}

bool NetworkKeyManager::validateNetworkKey(const std::vector<byte>& networkKey)
{
    return networkKey.size() == NETWORK_KEY_SIZE;
}

int NetworkKeyManager::initializeForHub()
{
    // Check if network key already exists
    if (hasNetworkKey()) {
        loginfo_ln("Network key already exists, using existing key");
        return RM_E_NONE;
    }
    
    // Generate new network key for hub
    std::vector<byte> networkKey;
    int rc = generateNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    // Store the new network key with initial version
    rc = setNetworkKey(networkKey, INITIAL_VERSION);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    loginfo_ln("Hub initialized with new network key version %u", INITIAL_VERSION);
    return RM_E_NONE;
}

bool NetworkKeyManager::hasNetworkKey()
{
    std::vector<byte> networkKey;
    int rc = loadNetworkKey(networkKey);
    return rc == RM_E_NONE && validateNetworkKey(networkKey);
}

void NetworkKeyManager::invalidateCache()
{
    cacheValid = false;
    cachedNetworkKey.clear();
    cachedVersion = 0;
}

int NetworkKeyManager::refreshCache()
{
    std::vector<byte> networkKey;
    int rc = loadNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    uint32_t version;
    rc = loadNetworkKeyVersion(version);
    if (rc != RM_E_NONE) {
        // Default to version 1 if not found
        version = INITIAL_VERSION;
    }
    
    cachedNetworkKey = networkKey;
    cachedVersion = version;
    cacheValid = true;
    
    return RM_E_NONE;
}