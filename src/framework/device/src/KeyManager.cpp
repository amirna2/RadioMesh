#include <Arduino.h>
#include <common/inc/Logger.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <framework/device/inc/KeyManager.h>
#ifdef ESP32
#include <esp_system.h>
#endif
extern "C"
{
    typedef struct uECC_Curve_t* uECC_Curve;
    uECC_Curve uECC_secp256r1(void);
    void uECC_set_rng(int (*rng_function)(uint8_t* dest, unsigned size));
    int uECC_make_key(uint8_t* public_key, uint8_t* private_key, uECC_Curve curve);
    int uECC_compute_public_key(const uint8_t* private_key, uint8_t* public_key, uECC_Curve curve);
}

static uint32_t seededRngState = 0;
static bool rngSeeded = false;

// Deterministic RNG using ESP32 chipID as seed
static int deterministic_rng_function(uint8_t* dest, unsigned size)
{
    if (!rngSeeded) {
#ifdef ESP32
        uint64_t chipId = ESP.getEfuseMac();
        seededRngState = (uint32_t)(chipId ^ (chipId >> 32));
#else
        seededRngState = 12345; // Fallback for non-ESP32 platforms
#endif
        rngSeeded = true;
        logdbg_ln("Seeded deterministic RNG with chipID-based seed: 0x%08X", seededRngState);
    }

    // Simple linear congruential generator with ESP32 chipID seed
    for (unsigned i = 0; i < size; i++) {
        seededRngState = seededRngState * 1103515245 + 12345;
        dest[i] = (seededRngState >> 16) & 0xFF;
    }
    return 1;
}

int KeyManager::generateKeyPair(std::vector<byte>& publicKey, std::vector<byte>& privateKey)
{
    // Initialize ECC with deterministic RNG
    static bool initialized = false;
    if (!initialized) {
        uECC_set_rng(&deterministic_rng_function);
        initialized = true;
    }

    // Use proper ECC key generation with deterministic seed
    privateKey.resize(PRIVATE_KEY_SIZE); // 32 bytes
    publicKey.resize(64);                // 64 bytes for full public key

    uECC_Curve curve = uECC_secp256r1();
    if (!uECC_make_key(publicKey.data(), privateKey.data(), curve)) {
        logerr_ln("Failed to generate ECC key pair");
        return RM_E_CRYPTO_SETUP;
    }

    logdbg_ln("Generated deterministic ECC key pair based on ESP32 chipID");
    return RM_E_NONE;
}

int KeyManager::derivePublicKey(const std::vector<byte>& privateKey, std::vector<byte>& publicKey)
{
    if (privateKey.size() != 32) {
        logerr_ln("Invalid private key size: %d", privateKey.size());
        return RM_E_INVALID_PARAM;
    }

    publicKey.resize(64);
    uECC_Curve curve = uECC_secp256r1();

    if (!uECC_compute_public_key(privateKey.data(), publicKey.data(), curve)) {
        logerr_ln("Failed to derive public key from private key");
        return RM_E_CRYPTO_SETUP;
    }

    return RM_E_NONE;
}

int KeyManager::generateNetworkKey(std::vector<byte>& networkKey)
{
    networkKey.resize(NETWORK_KEY_SIZE);

    // Generate random network key
    for (size_t i = 0; i < NETWORK_KEY_SIZE; i++) {
        networkKey[i] = random(256);
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
    if (recipientPubKey.size() != 64 || !validateNetworkKey(networkKey)) {
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
    return publicKey.size() == 64; // Full ECC public key is 64 bytes
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
