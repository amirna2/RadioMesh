#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
// Use ESP32's built-in ECC from TinyCrypt  
extern "C" {
typedef struct uECC_Curve_t* uECC_Curve;
uECC_Curve uECC_secp256r1(void);
void uECC_set_rng(int (*rng_function)(uint8_t *dest, unsigned size));
int uECC_make_key(uint8_t *public_key, uint8_t *private_key, uECC_Curve curve);
int uECC_shared_secret(const uint8_t *public_key, const uint8_t *private_key, uint8_t *secret, uECC_Curve curve);
int uECC_valid_public_key(const uint8_t *public_key, uECC_Curve curve);
// Note: ESP32 TinyCrypt doesn't have compress/decompress
}
#include <Crypto.h>
#include <SHA256.h>
#include <Arduino.h>

// RNG function for micro-ecc
static int RNG(uint8_t *dest, unsigned size) {
    while (size) {
        *dest++ = random(256);
        size--;
    }
    return 1;
}

// Initialize micro-ecc RNG on first use
static void initECC() {
    static bool initialized = false;
    if (!initialized) {
        uECC_set_rng(&RNG);
        initialized = true;
    }
}

std::vector<byte> EncryptionService::encrypt(const std::vector<byte>& data, uint8_t topic,
                                             MeshDeviceType deviceType,
                                             DeviceInclusionState inclusionState)
{
    EncryptionMethod method = determineEncryptionMethod(topic, deviceType, inclusionState);
    
    // For NONE encryption, we can return early if data is empty
    if (method == EncryptionMethod::NONE && data.empty()) {
        return data;
    }

    switch (method) {
    case EncryptionMethod::NONE:
        logdbg_ln("No encryption for topic 0x%02X", topic);
        return data;

    case EncryptionMethod::ECIES: {
        std::vector<byte> key = getEncryptionKey(method, topic, deviceType);
        if (key.empty()) {
            logerr_ln("No ECIES key available for topic 0x%02X, deviceType=%d", topic, (int)deviceType);
            return data;
        }
        logdbg_ln("Encrypting with ECIES for topic 0x%02X, key size=%d", topic, key.size());
        return encryptECIES(data, key);
    }

    case EncryptionMethod::AES: {
        std::vector<byte> key = getEncryptionKey(method, topic, deviceType);
        if (key.empty()) {
            logerr_ln("No AES key available for topic 0x%02X", topic);
            return data;
        }
        return encryptAES(data, key);
    }

    default:
        logerr_ln("Unknown encryption method for topic 0x%02X", topic);
        return data;
    }
}

std::vector<byte> EncryptionService::decrypt(const std::vector<byte>& data, uint8_t topic,
                                             MeshDeviceType deviceType,
                                             DeviceInclusionState inclusionState)
{
    if (data.empty()) {
        return data;
    }

    EncryptionMethod method = determineEncryptionMethod(topic, deviceType, inclusionState);

    switch (method) {
    case EncryptionMethod::NONE:
        logdbg_ln("No decryption for topic 0x%02X", topic);
        return data;

    case EncryptionMethod::ECIES: {
        std::vector<byte> key = getEncryptionKey(method, topic, deviceType);
        if (key.empty()) {
            logerr_ln("No ECIES key available for topic 0x%02X", topic);
            return data;
        }
        return decryptECIES(data, key);
    }

    case EncryptionMethod::AES: {
        std::vector<byte> key = getEncryptionKey(method, topic, deviceType);
        if (key.empty()) {
            logerr_ln("No AES key available for topic 0x%02X", topic);
            return data;
        }
        return decryptAES(data, key);
    }

    default:
        logerr_ln("Unknown decryption method for topic 0x%02X", topic);
        return data;
    }
}

void EncryptionService::setNetworkKey(const std::vector<byte>& key)
{
    networkKey = key;
    logdbg_ln("Network key set for EncryptionService");
}

void EncryptionService::setDeviceKeys(const std::vector<byte>& privateKey,
                                      const std::vector<byte>& publicKey)
{
    devicePrivateKey = privateKey;
    devicePublicKey = publicKey;
    logdbg_ln("Device keys set for EncryptionService");
}

void EncryptionService::setHubPublicKey(const std::vector<byte>& hubKey)
{
    hubPublicKey = hubKey;
    logdbg_ln("Hub public key set for EncryptionService, size=%d", hubKey.size());
}

void EncryptionService::setTempDevicePublicKey(const std::vector<byte>& deviceKey)
{
    tempDevicePublicKey = deviceKey;
    logdbg_ln("Temporary device public key set for EncryptionService");
}

EncryptionService::EncryptionMethod
EncryptionService::determineEncryptionMethod(uint8_t topic, MeshDeviceType deviceType,
                                             DeviceInclusionState inclusionState) const
{
    // Handle inclusion messages based on the encryption table
    switch (topic) {
    case MessageTopic::INCLUDE_OPEN:
        // Always unencrypted
        return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_REQUEST:
        // Standard device encrypts with hub's public key, Hub decrypts with hub's private key
        if ((deviceType == MeshDeviceType::STANDARD &&
             inclusionState == DeviceInclusionState::INCLUSION_PENDING) ||
            (deviceType == MeshDeviceType::HUB)) {
            return EncryptionMethod::ECIES;
        }
        return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_RESPONSE:
        // Hub encrypts with device's public key, Standard device decrypts with device's private key
        if ((deviceType == MeshDeviceType::HUB) ||
            (deviceType == MeshDeviceType::STANDARD &&
             inclusionState == DeviceInclusionState::INCLUSION_PENDING)) {
            return EncryptionMethod::ECIES;
        }
        return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_CONFIRM:
        // Standard device encrypts with shared network key, Hub decrypts with shared network key
        if ((deviceType == MeshDeviceType::STANDARD &&
             inclusionState == DeviceInclusionState::INCLUSION_PENDING) ||
            (deviceType == MeshDeviceType::HUB)) {
            return EncryptionMethod::AES;
        }
        return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_SUCCESS:
        // Hub encrypts with shared network key, Standard device decrypts with shared network key
        if ((deviceType == MeshDeviceType::HUB) ||
            (deviceType == MeshDeviceType::STANDARD &&
             inclusionState == DeviceInclusionState::INCLUSION_PENDING)) {
            return EncryptionMethod::AES;
        }
        return EncryptionMethod::NONE;

    default:
        // Regular messages - use AES if device is included
        if (inclusionState == DeviceInclusionState::INCLUDED) {
            return EncryptionMethod::AES;
        }
        return EncryptionMethod::NONE;
    }
}

std::vector<byte> EncryptionService::getEncryptionKey(EncryptionMethod method, uint8_t topic,
                                                      MeshDeviceType deviceType) const
{
    logdbg_ln("getEncryptionKey: method=%d, topic=0x%02X, deviceType=%d", (int)method, topic, (int)deviceType);
    
    switch (method) {
    case EncryptionMethod::ECIES:
        if (topic == MessageTopic::INCLUDE_REQUEST) {
            // Standard device uses hub's public key
            logdbg_ln("Returning hub public key, size=%d", hubPublicKey.size());
            return hubPublicKey;
        } else if (topic == MessageTopic::INCLUDE_RESPONSE) {
            // Hub uses device's public key
            logdbg_ln("Returning temp device public key, size=%d", tempDevicePublicKey.size());
            return tempDevicePublicKey;
        }
        break;

    case EncryptionMethod::AES:
        // Always use shared network key for AES
        return networkKey;

    case EncryptionMethod::NONE:
    default:
        break;
    }

    return std::vector<byte>();
}

std::vector<byte> EncryptionService::encryptECIES(const std::vector<byte>& data,
                                                  const std::vector<byte>& publicKey)
{
    if (publicKey.size() != 64) {
        logerr_ln("Invalid public key size for ECIES: %d (expected 64)", publicKey.size());
        return data;
    }

    // Initialize ECC if needed
    initECC();

    // Use secp256r1 curve (ESP32's TinyCrypt supports this)
    uECC_Curve curve = uECC_secp256r1();
    
    // Generate ephemeral key pair  
    uint8_t ephemeralPrivKey[32];
    uint8_t ephemeralPubKey[64]; // Uncompressed public key
    
    // Generate random private key
    if (!uECC_make_key(ephemeralPubKey, ephemeralPrivKey, curve)) {
        logerr_ln("Failed to generate ephemeral key pair");
        return data;
    }
    
    // For this implementation, we'll use the first 32 bytes of the uncompressed public key
    // as the ephemeral public key to match the expected test format
    uint8_t ephemeralPubKeyX[32];
    memcpy(ephemeralPubKeyX, ephemeralPubKey, 32);
    
    // Use the full 64-byte public key directly
    uint8_t recipientFullPubKey[64];
    memcpy(recipientFullPubKey, publicKey.data(), 64);
    
    // Perform ECDH to get shared secret
    uint8_t sharedSecret[32];
    if (!uECC_shared_secret(recipientFullPubKey, ephemeralPrivKey, sharedSecret, curve)) {
        logerr_ln("Failed to compute ECDH shared secret");
        return data;
    }
    
    // Use SHA256 to derive encryption key from shared secret
    SHA256 sha256;
    sha256.reset();
    sha256.update(sharedSecret, 32);
    uint8_t encryptionKey[32];
    sha256.finalize(encryptionKey, 32);
    
    // Convert to vector for AES encryption
    std::vector<byte> keyVector(encryptionKey, encryptionKey + 32);
    
    // Encrypt data with AES using derived key
    std::vector<byte> encryptedData = encryptAES(data, keyVector);
    if (encryptedData.empty()) {
        logerr_ln("Failed to encrypt data with ECIES");
        return data;
    }

    // Return: [ephemeral_pub_key(64)] + [encrypted_data]
    std::vector<byte> result;
    result.insert(result.end(), ephemeralPubKey, ephemeralPubKey + 64);
    result.insert(result.end(), encryptedData.begin(), encryptedData.end());

    return result;
}

std::vector<byte> EncryptionService::decryptECIES(const std::vector<byte>& data,
                                                  const std::vector<byte>& privateKey)
{
    // Expected format: [ephemeral_pub_key(64)] + [encrypted_data]
    if (data.size() < 64 + 1) { // (at least 1 byte of encrypted data)
        logerr_ln("Invalid ECIES data size: %d", data.size());
        return data;
    }

    if (privateKey.size() != 32) {
        logerr_ln("Invalid private key size for ECIES: %d", privateKey.size());
        return data;
    }

    // Initialize ECC if needed
    initECC();

    // Use secp256r1 curve (ESP32's TinyCrypt supports this)
    uECC_Curve curve = uECC_secp256r1();
    
    // Extract ephemeral public key (64 bytes) and encrypted data
    uint8_t ephemeralFullPubKey[64];
    memcpy(ephemeralFullPubKey, data.data(), 64);
    
    std::vector<byte> encryptedData(data.begin() + 64, data.end());
    
    // Perform ECDH to get shared secret
    uint8_t sharedSecret[32];
    if (!uECC_shared_secret(ephemeralFullPubKey, privateKey.data(), sharedSecret, curve)) {
        logerr_ln("Failed to compute ECDH shared secret for decryption");
        return data;
    }
    
    // Use SHA256 to derive encryption key from shared secret
    SHA256 sha256;
    sha256.reset();
    sha256.update(sharedSecret, 32);
    uint8_t encryptionKey[32];
    sha256.finalize(encryptionKey, 32);
    
    // Convert to vector for AES decryption
    std::vector<byte> keyVector(encryptionKey, encryptionKey + 32);
    
    // Decrypt data with AES using derived key
    return decryptAES(encryptedData, keyVector);
}

std::vector<byte> EncryptionService::encryptAES(const std::vector<byte>& data,
                                                const std::vector<byte>& key)
{
    if (!aesCrypto) {
        aesCrypto = AesCrypto::getInstance();
    }

    SecurityParams params;
    params.method = SecurityMethod::AES;
    params.key = key;
    params.iv = std::vector<byte>(16, 0);

    aesCrypto->setParams(params);
    
    // CTR mode - no padding needed, output size = input size
    return aesCrypto->encrypt(data);
}

std::vector<byte> EncryptionService::decryptAES(const std::vector<byte>& data,
                                                const std::vector<byte>& key)
{
    if (!aesCrypto) {
        aesCrypto = AesCrypto::getInstance();
    }

    SecurityParams params;
    params.method = SecurityMethod::AES;
    params.key = key;
    params.iv = std::vector<byte>(16, 0);

    aesCrypto->setParams(params);
    return aesCrypto->decrypt(data);
}
