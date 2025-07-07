#include <common/inc/Logger.h>
#include <framework/device/inc/KeyManager.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
extern "C" {
typedef struct uECC_Curve_t* uECC_Curve;
uECC_Curve uECC_secp256r1(void);
void uECC_set_rng(int (*rng_function)(uint8_t *dest, unsigned size));
int uECC_make_key(uint8_t *public_key, uint8_t *private_key, uECC_Curve curve);
}

// RNG function for ECC
static int rng_function(uint8_t *dest, unsigned size) {
    while (size) {
        *dest++ = random(256);
        size--;
    }
    return 1;
}

int KeyManager::generateKeyPair(std::vector<byte>& publicKey, std::vector<byte>& privateKey)
{
    // Initialize ECC
    static bool initialized = false;
    if (!initialized) {
        uECC_set_rng(&rng_function);
        initialized = true;
    }
    
    // Use proper ECC key generation
    privateKey.resize(PRIVATE_KEY_SIZE);  // 32 bytes
    publicKey.resize(64);  // 64 bytes for full public key
    
    uECC_Curve curve = uECC_secp256r1();
    if (!uECC_make_key(publicKey.data(), privateKey.data(), curve)) {
        logerr_ln("Failed to generate ECC key pair");
        return RM_E_CRYPTO_SETUP;
    }

    return RM_E_NONE;
}

int KeyManager::generateSessionKey(std::vector<byte>& sessionKey)
{
    sessionKey.resize(SESSION_KEY_SIZE);

    // Generate random session key
    for (size_t i = 0; i < SESSION_KEY_SIZE; i++) {
        sessionKey[i] = random(256);
    }

    return RM_E_NONE;
}

int KeyManager::encryptSessionKey(const std::vector<byte>& sessionKey,
                                  const std::vector<byte>& recipientPubKey,
                                  std::vector<byte>& encryptedKey)
{
    if (recipientPubKey.size() != 64 || !validateSessionKey(sessionKey)) {
        return RM_E_INVALID_PARAM;
    }

    // Use proper ECIES encryption
    EncryptionService encryptionService;
    encryptedKey = encryptionService.encryptECIES(sessionKey, recipientPubKey);
    
    if (encryptedKey.size() <= sessionKey.size()) {
        logerr_ln("ECIES encryption failed for session key");
        return RM_E_CRYPTO_SETUP;
    }

    return RM_E_NONE;
}

int KeyManager::decryptSessionKey(const std::vector<byte>& encryptedKey,
                                  const std::vector<byte>& privateKey,
                                  std::vector<byte>& sessionKey)
{
    if (!validatePrivateKey(privateKey)) {
        return RM_E_INVALID_PARAM;
    }

    // Use proper ECIES decryption
    EncryptionService encryptionService;
    sessionKey = encryptionService.decryptECIES(encryptedKey, privateKey);
    
    if (!validateSessionKey(sessionKey)) {
        logerr_ln("ECIES decryption failed for session key");
        return RM_E_CRYPTO_SETUP;
    }

    return RM_E_NONE;
}

int KeyManager::encryptNetworkKey(const std::vector<byte>& networkKey,
                                  const std::vector<byte>& recipientPubKey,
                                  std::vector<byte>& encryptedKey)
{
    if (recipientPubKey.size() != 64 || !validateNetworkKey(networkKey)) {
        return RM_E_INVALID_PARAM;
    }

    // Use proper ECIES encryption
    EncryptionService encryptionService;
    encryptedKey = encryptionService.encryptECIES(networkKey, recipientPubKey);
    
    if (encryptedKey.size() <= networkKey.size()) {
        logerr_ln("ECIES encryption failed for network key");
        return RM_E_CRYPTO_SETUP;
    }

    return RM_E_NONE;
}

int KeyManager::decryptNetworkKey(const std::vector<byte>& encryptedKey,
                                  const std::vector<byte>& privateKey,
                                  std::vector<byte>& networkKey)
{
    if (!validatePrivateKey(privateKey)) {
        return RM_E_INVALID_PARAM;
    }

    // Use proper ECIES decryption
    EncryptionService encryptionService;
    networkKey = encryptionService.decryptECIES(encryptedKey, privateKey);
    
    if (!validateNetworkKey(networkKey)) {
        logerr_ln("ECIES decryption failed for network key");
        return RM_E_CRYPTO_SETUP;
    }

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

int KeyManager::loadSessionKey(std::vector<byte>& sessionKey)
{
    return storage.loadSessionKey(sessionKey);
}

int KeyManager::persistSessionKey(const std::vector<byte>& sessionKey)
{
    if (!validateSessionKey(sessionKey)) {
        return RM_E_INVALID_PARAM;
    }
    return storage.persistSessionKey(sessionKey);
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

int KeyManager::loadNetworkKeyVersion(uint32_t& version)
{
    return storage.loadNetworkKeyVersion(version);
}

int KeyManager::persistNetworkKeyVersion(uint32_t version)
{
    return storage.persistNetworkKeyVersion(version);
}

bool KeyManager::validatePublicKey(const std::vector<byte>& publicKey)
{
    return publicKey.size() == 64; // Full ECC public key is 64 bytes
}

bool KeyManager::validatePrivateKey(const std::vector<byte>& privateKey)
{
    return privateKey.size() == PRIVATE_KEY_SIZE;
}

bool KeyManager::validateSessionKey(const std::vector<byte>& sessionKey)
{
    return sessionKey.size() == SESSION_KEY_SIZE;
}

bool KeyManager::validateNetworkKey(const std::vector<byte>& networkKey)
{
    return networkKey.size() == NETWORK_KEY_SIZE;
}
