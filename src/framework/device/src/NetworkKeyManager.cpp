#include <common/inc/Logger.h>
#include <framework/device/inc/NetworkKeyManager.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <common/utils/Utils.h>

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
    return storage.loadNetworkKey(networkKey);
}


int NetworkKeyManager::setNetworkKey(const std::vector<byte>& networkKey)
{
    if (!validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }
    
    int rc = persistNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    loginfo_ln("Set network key");
    return RM_E_NONE;
}

int NetworkKeyManager::encryptNetworkKey(const std::vector<byte>& networkKey,
                                         const std::vector<byte>& recipientPubKey,
                                         std::vector<byte>& encryptedKey)
{
    if (!validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }
    
    if (recipientPubKey.size() != 64) {
        logerr_ln("Invalid recipient public key size: %d (expected 64)", recipientPubKey.size());
        return RM_E_INVALID_PARAM;
    }
    
    // Use proper ECIES encryption via EncryptionService
    EncryptionService encryptionService;
    encryptedKey = encryptionService.encryptECIES(networkKey, recipientPubKey);
    
    if (encryptedKey.size() <= networkKey.size()) {
        logerr_ln("ECIES encryption failed - no size increase");
        return RM_E_CRYPTO_SETUP;
    }
    
    logdbg_ln("ECIES encrypted network key: encrypted_size=%d", encryptedKey.size());
    
    return RM_E_NONE;
}

int NetworkKeyManager::decryptNetworkKey(const std::vector<byte>& encryptedKey,
                                         const std::vector<byte>& privateKey,
                                         std::vector<byte>& networkKey)
{
    // Expected format: [ephemeral_pub_key(64)] + [encrypted_network_key]
    if (encryptedKey.size() < 64 + 1) {
        logerr_ln("Invalid encrypted key size: %d", encryptedKey.size());
        return RM_E_INVALID_LENGTH;
    }
    
    if (privateKey.size() != 32) {
        logerr_ln("Invalid private key size: %d", privateKey.size());
        return RM_E_INVALID_PARAM;
    }
    
    // Use proper ECIES decryption via EncryptionService
    EncryptionService encryptionService;
    networkKey = encryptionService.decryptECIES(encryptedKey, privateKey);
    
    if (networkKey.empty() || !validateNetworkKey(networkKey)) {
        logerr_ln("Failed to decrypt network key or invalid size");
        return RM_E_CRYPTO_SETUP;
    }
    
    logdbg_ln("ECIES decrypted network key successfully");
    
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
    
    return storage.persistNetworkKey(networkKey);
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
    
    // Store the new network key
    rc = setNetworkKey(networkKey);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    loginfo_ln("Hub initialized with new network key");
    return RM_E_NONE;
}

bool NetworkKeyManager::hasNetworkKey()
{
    std::vector<byte> networkKey;
    int rc = loadNetworkKey(networkKey);
    return rc == RM_E_NONE && validateNetworkKey(networkKey);
}

