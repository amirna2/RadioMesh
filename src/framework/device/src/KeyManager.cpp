#include <Arduino.h>
#include <common/inc/Logger.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <framework/device/inc/KeyManager.h>
#include <Crypto.h>
#include <Curve25519.h>
#include <RNG.h>
#ifdef ESP32
#include <esp_system.h>
#endif

// Deterministic key generation using ESP32 chipID as seed
static void generateDeterministicKey(uint8_t* key, size_t keySize)
{
    uint32_t seed = 0;
#ifdef ESP32
    uint64_t chipId = ESP.getEfuseMac();
    seed = (uint32_t)(chipId ^ (chipId >> 32));
#else
    seed = 12345; // Fallback for non-ESP32 platforms
#endif
    
    logdbg_ln("Generating deterministic key with chipID-based seed: 0x%08X", seed);
    
    // Use simple LCG to generate deterministic bytes
    for (size_t i = 0; i < keySize; i++) {
        seed = seed * 1103515245 + 12345;
        key[i] = (seed >> 16) & 0xFF;
    }
    
    // Apply Curve25519 private key mask
    if (keySize == 32) {
        key[0] &= 0xF8;  // Clear bottom 3 bits
        key[31] = (key[31] & 0x7F) | 0x40;  // Clear top bit, set second-to-top bit
    }
}

int KeyManager::generateKeyPair(std::vector<byte>& publicKey, std::vector<byte>& privateKey)
{
    // Generate deterministic Curve25519 key pair
    privateKey.resize(32); // Curve25519 private key is 32 bytes
    publicKey.resize(32);  // Curve25519 public key is 32 bytes

    // Generate deterministic private key based on ESP32 chipID
    generateDeterministicKey(privateKey.data(), 32);
    
    // Compute public key from private key using Curve25519
    if (!Curve25519::eval(publicKey.data(), privateKey.data(), nullptr)) {
        logerr_ln("Failed to generate Curve25519 public key from private key");
        return RM_E_CRYPTO_SETUP;
    }

    logdbg_ln("Generated deterministic Curve25519 key pair based on ESP32 chipID");
    return RM_E_NONE;
}

int KeyManager::derivePublicKey(const std::vector<byte>& privateKey, std::vector<byte>& publicKey)
{
    if (privateKey.size() != 32) {
        logerr_ln("Invalid private key size: %d (expected 32 for Curve25519)", privateKey.size());
        return RM_E_INVALID_PARAM;
    }

    publicKey.resize(32); // Curve25519 public key is 32 bytes

    if (!Curve25519::eval(publicKey.data(), privateKey.data(), nullptr)) {
        logerr_ln("Failed to derive Curve25519 public key from private key");
        return RM_E_CRYPTO_SETUP;
    }

    return RM_E_NONE;
}

int KeyManager::generateNetworkKey(std::vector<byte>& networkKey)
{
    networkKey.resize(NETWORK_KEY_SIZE);
    // Generate random network key
    for (size_t i = 0; i < NETWORK_KEY_SIZE; i++) {
        // networkKey[i] = random(256);
        networkKey[i] = 0x02; // For testing, use a simple pattern
    }
    loginfo_ln("Generated new network key");
    return RM_E_NONE;
}

int KeyManager::getCurrentNetworkKey(std::vector<byte>& networkKey)
{
    return storage.loadNetworkKey(networkKey);
}

int KeyManager::setNetworkKey(const std::vector<byte>& networkKey)
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

int KeyManager::encryptNetworkKey(const std::vector<byte>& networkKey,
                                  const std::vector<byte>& recipientPubKey,
                                  std::vector<byte>& encryptedKey)
{
    if (recipientPubKey.size() != 32 || !validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }

    // Load our private key and public key for direct ECC encryption
    std::vector<byte> privateKey, publicKey;
    int rc = loadPrivateKey(privateKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to load private key for network key encryption");
        return rc;
    }

    // Re-derive public key from private key
    rc = derivePublicKey(privateKey, publicKey);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to derive public key for network key encryption");
        return rc;
    }

    // Use direct ECC encryption (zero overhead)
    EncryptionService encryptionService;
    encryptionService.setDeviceKeys(privateKey, publicKey);
    encryptedKey = encryptionService.encryptDirectECC(networkKey, recipientPubKey);

    if (encryptedKey.size() != networkKey.size()) {
        logerr_ln("Direct ECC encryption failed - unexpected size change: %d -> %d",
                  networkKey.size(), encryptedKey.size());
        return RM_E_CRYPTO_SETUP;
    }

    logdbg_ln("Network key encrypted with direct ECC (zero overhead): %d bytes",
              encryptedKey.size());
    return RM_E_NONE;
}

int KeyManager::decryptNetworkKey(const std::vector<byte>& encryptedKey,
                                  const std::vector<byte>& privateKey,
                                  std::vector<byte>& networkKey)
{
    if (!validatePrivateKey(privateKey)) {
        return RM_E_INVALID_PARAM;
    }

    // For direct ECC decryption, we need the sender's public key
    // This should be set in the EncryptionService context
    EncryptionService encryptionService;
    networkKey = encryptionService.decryptDirectECC(encryptedKey, privateKey);

    if (!validateNetworkKey(networkKey)) {
        logerr_ln("Direct ECC decryption failed for network key");
        return RM_E_CRYPTO_SETUP;
    }

    logdbg_ln("Network key decrypted with direct ECC: %d bytes", networkKey.size());
    return RM_E_NONE;
}

// Storage operations simply delegate to DeviceStorage
int KeyManager::loadPrivateKey(std::vector<byte>& privateKey)
{
    return storage.loadPrivateKey(privateKey);
}

int KeyManager::persistPrivateKey(const std::vector<byte>& privateKey)
{
    if (!validatePrivateKey(privateKey)) {
        return RM_E_INVALID_PARAM;
    }
    return storage.persistPrivateKey(privateKey);
}

int KeyManager::loadHubKey(std::vector<byte>& hubKey)
{
    return storage.loadHubKey(hubKey);
}

int KeyManager::persistHubKey(const std::vector<byte>& hubKey)
{
    if (!validatePublicKey(hubKey)) {
        return RM_E_INVALID_PARAM;
    }
    return storage.persistHubKey(hubKey);
}

int KeyManager::loadNetworkKey(std::vector<byte>& networkKey)
{
    return storage.loadNetworkKey(networkKey);
}

int KeyManager::persistNetworkKey(const std::vector<byte>& networkKey)
{
    if (!validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }
    return storage.persistNetworkKey(networkKey);
}

bool KeyManager::validatePublicKey(const std::vector<byte>& publicKey)
{
    return publicKey.size() == 32; // Curve25519 public key is 32 bytes
}

bool KeyManager::validatePrivateKey(const std::vector<byte>& privateKey)
{
    return privateKey.size() == PRIVATE_KEY_SIZE;
}

bool KeyManager::validateNetworkKey(const std::vector<byte>& networkKey)
{
    return networkKey.size() == NETWORK_KEY_SIZE;
}

int KeyManager::initializeForHub()
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

bool KeyManager::hasNetworkKey()
{
    std::vector<byte> networkKey;
    int rc = loadNetworkKey(networkKey);
    return rc == RM_E_NONE && validateNetworkKey(networkKey);
}
