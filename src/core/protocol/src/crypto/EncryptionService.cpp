#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <Arduino.h>
#include <Crypto.h>
#include <Curve25519.h>
#include <SHA256.h>


std::vector<byte> EncryptionService::encrypt(const std::vector<byte>& data, uint8_t topic,
                                             MeshDeviceType deviceType,
                                             DeviceInclusionState inclusionState)
{
    EncryptionMethod method = determineCryptoMethod(topic, deviceType, inclusionState);

    // For NONE encryption, we can return early if data is empty
    if (method == EncryptionMethod::NONE && data.empty()) {
        return data;
    }

    switch (method) {
    case EncryptionMethod::NONE:
        logdbg_ln("No encryption for topic 0x%02X", topic);
        return data;

    case EncryptionMethod::DIRECT_ECC: {
        std::vector<byte> key = getEncryptionKey(method, topic, deviceType);
        if (key.empty()) {
            logerr_ln("No direct ECC key available for topic 0x%02X, deviceType=%d", topic,
                      (int)deviceType);
            return data;
        }
        logdbg_ln("Encrypting with direct ECC for topic 0x%02X, key size=%d", topic, key.size());
        return encryptDirectECC(data, key);
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

    EncryptionMethod method = determineCryptoMethod(topic, deviceType, inclusionState);

    switch (method) {
    case EncryptionMethod::NONE:
        logdbg_ln("No decryption for topic 0x%02X", topic);
        return data;

    case EncryptionMethod::DIRECT_ECC: {
        std::vector<byte> key = getDecryptionKey(method, topic, deviceType);
        if (key.empty()) {
            logerr_ln("No direct ECC key available for topic 0x%02X", topic);
            return data;
        }
        return decryptDirectECC(data, key);
    }

    case EncryptionMethod::AES: {
        std::vector<byte> key = getDecryptionKey(method, topic, deviceType);
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
    // Curve25519 uses 32-byte keys for both private and public
    if (privateKey.size() != 32) {
        logerr_ln("Invalid private key size: %d (expected 32 for Curve25519)", privateKey.size());
        return;
    }
    if (publicKey.size() != 32) {
        logerr_ln("Invalid public key size: %d (expected 32 for Curve25519)", publicKey.size());
        return;
    }
    
    devicePrivateKey = privateKey;
    devicePublicKey = publicKey;
    
    logdbg_ln("Device keys set for EncryptionService (Curve25519)");
}

void EncryptionService::setHubPublicKey(const std::vector<byte>& hubKey)
{
    if (hubKey.size() != 32) {
        logerr_ln("Invalid hub public key size: %d (expected 32 for Curve25519)", hubKey.size());
        return;
    }
    
    hubPublicKey = hubKey;
    logdbg_ln("Hub public key set for EncryptionService, size=%d", hubKey.size());
}

void EncryptionService::setTempDevicePublicKey(const std::vector<byte>& deviceKey)
{
    if (deviceKey.size() != 32) {
        logerr_ln("Invalid temp device public key size: %d (expected 32 for Curve25519)", deviceKey.size());
        return;
    }
    
    tempDevicePublicKey = deviceKey;
    logdbg_ln("Temporary device public key set for EncryptionService");
}

EncryptionService::EncryptionMethod
EncryptionService::determineCryptoMethod(uint8_t topic, MeshDeviceType deviceType,
                                         DeviceInclusionState inclusionState) const
{
    // Simple lookup based on message topic - encryption is determined by protocol design
    switch (topic) {
    case MessageTopic::INCLUDE_OPEN:
        // By design: Always unencrypted (broadcasts hub public key)
        return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_REQUEST:
        // By design: Always unencrypted (public key exchange)
        return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_RESPONSE:
        // By design: Always Direct ECC encrypted (hub -> device)
        return EncryptionMethod::DIRECT_ECC;
        // return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_CONFIRM:
        // By design: Always AES encrypted with network key (device -> hub)
        return EncryptionMethod::AES;
        // return EncryptionMethod::NONE;

    case MessageTopic::INCLUDE_SUCCESS:
        // By design: Always AES encrypted with network key (hub -> device)
        return EncryptionMethod::AES;
        // return EncryptionMethod::NONE;

    default:
        // Regular messages - use AES if device is included or is a hub
        if (inclusionState == DeviceInclusionState::INCLUDED || deviceType == MeshDeviceType::HUB) {
            return EncryptionMethod::AES;
            // return EncryptionMethod::NONE;
        }
        return EncryptionMethod::NONE;
    }
}

std::vector<byte> EncryptionService::getEncryptionKey(EncryptionMethod method, uint8_t topic,
                                                      MeshDeviceType deviceType) const
{
    logdbg_ln("getEncryptionKey: method=%d, topic=0x%02X, deviceType=%d", (int)method, topic,
              (int)deviceType);

    switch (method) {
    case EncryptionMethod::DIRECT_ECC:
        if (topic == MessageTopic::INCLUDE_REQUEST) {
            // Standard device encrypts with hub's public key
            logdbg_ln("Returning hub public key for encryption, size=%d", hubPublicKey.size());
            return hubPublicKey;
        } else if (topic == MessageTopic::INCLUDE_RESPONSE) {
            // Hub encrypts with device's public key
            logdbg_ln("Returning temp device public key for encryption, size=%d",
                      tempDevicePublicKey.size());
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

std::vector<byte> EncryptionService::getDecryptionKey(EncryptionMethod method, uint8_t topic,
                                                      MeshDeviceType deviceType) const
{
    logdbg_ln("getDecryptionKey: method=%d, topic=0x%02X, deviceType=%d", (int)method, topic,
              (int)deviceType);

    switch (method) {
    case EncryptionMethod::DIRECT_ECC:
        if (topic == MessageTopic::INCLUDE_REQUEST) {
            // Hub uses its own private key to decrypt
            if (deviceType == MeshDeviceType::HUB) {
                logdbg_ln("Returning device private key for hub, size=%d", devicePrivateKey.size());
                return devicePrivateKey;
            }
        } else if (topic == MessageTopic::INCLUDE_RESPONSE) {
            // Standard device uses its own private key to decrypt
            if (deviceType == MeshDeviceType::STANDARD) {
                logdbg_ln("Returning device private key for standard device, size=%d",
                          devicePrivateKey.size());
                return devicePrivateKey;
            }
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

std::vector<byte> EncryptionService::encryptDirectECC(const std::vector<byte>& data,
                                                      const std::vector<byte>& publicKey)
{
    if (publicKey.size() != 32) {
        logerr_ln("Invalid public key size for Curve25519: %d (expected 32)", publicKey.size());
        return data;
    }

    if (devicePrivateKey.size() != 32) {
        logerr_ln("Device private key not set for direct ECC encryption");
        return data;
    }

    // Debug: Log the keys being used
    logdbg_ln("ENCRYPT using device private key (first 8 bytes): %s",
              RadioMeshUtils::convertToHex(devicePrivateKey.data(), 8).c_str());
    logdbg_ln("ENCRYPT using recipient public key (first 8 bytes): %s",
              RadioMeshUtils::convertToHex(publicKey.data(), 8).c_str());

    // Perform ECDH using Curve25519
    uint8_t sharedSecret[32];
    if (!Curve25519::eval(sharedSecret, devicePrivateKey.data(), publicKey.data())) {
        logerr_ln("Failed to compute Curve25519 shared secret for direct ECC");
        return data;
    }

    // Debug: Print shared secret for comparison
    logdbg_ln("ENCRYPT ECDH shared secret (first 8 bytes): %s",
              RadioMeshUtils::convertToHex(sharedSecret, 8).c_str());

    // Use SHA256 to derive encryption key from shared secret
    SHA256 sha256;
    sha256.reset();
    sha256.update(sharedSecret, 32);
    uint8_t encryptionKey[32];
    sha256.finalize(encryptionKey, 32);

    // Convert to vector for AES encryption
    std::vector<byte> keyVector(encryptionKey, encryptionKey + 32);

    // Encrypt data with AES using derived key - ZERO OVERHEAD!
    std::vector<byte> encryptedData = encryptAES(data, keyVector);
    if (encryptedData.empty()) {
        logerr_ln("Failed to encrypt data with Curve25519 ECC");
        return data;
    }

    logdbg_ln("Curve25519 ECC encryption: input=%d bytes, output=%d bytes (zero overhead)", data.size(),
              encryptedData.size());

    // Return encrypted data directly - no ephemeral key overhead!
    return encryptedData;
}

std::vector<byte> EncryptionService::decryptDirectECC(const std::vector<byte>& data,
                                                      const std::vector<byte>& privateKey)
{
    if (data.empty()) {
        logerr_ln("Empty data for direct ECC decryption");
        return data;
    }

    if (privateKey.size() != 32) {
        logerr_ln("Invalid private key size for Curve25519: %d (expected 32)", privateKey.size());
        return data;
    }

    // For direct ECC, we need the sender's public key to perform ECDH
    // This should be available in the context (either devicePublicKey or hubPublicKey)
    std::vector<byte> senderPublicKey;

    // Determine sender's public key based on context
    if (!hubPublicKey.empty()) {
        senderPublicKey = hubPublicKey; // Device decrypting, sender is hub
    } else if (!tempDevicePublicKey.empty()) {
        senderPublicKey = tempDevicePublicKey; // Hub decrypting, sender is device
    } else {
        logerr_ln("No sender public key available for direct ECC decryption");
        return data;
    }

    if (senderPublicKey.size() != 32) {
        logerr_ln("Invalid sender public key size for Curve25519: %d (expected 32)", senderPublicKey.size());
        return data;
    }

    // Debug: Log the keys being used
    logdbg_ln("DECRYPT using our private key (first 8 bytes): %s",
              RadioMeshUtils::convertToHex(privateKey.data(), 8).c_str());
    logdbg_ln("DECRYPT using sender public key (first 8 bytes): %s",
              RadioMeshUtils::convertToHex(senderPublicKey.data(), 8).c_str());

    // Perform ECDH using Curve25519
    uint8_t sharedSecret[32];
    if (!Curve25519::eval(sharedSecret, privateKey.data(), senderPublicKey.data())) {
        logerr_ln("Failed to compute Curve25519 shared secret for direct ECC decryption");
        return data;
    }

    // Debug: Print shared secret for comparison
    logdbg_ln("DECRYPT ECDH shared secret (first 8 bytes): %s",
              RadioMeshUtils::convertToHex(sharedSecret, 8).c_str());

    // Use SHA256 to derive encryption key from shared secret
    SHA256 sha256;
    sha256.reset();
    sha256.update(sharedSecret, 32);
    uint8_t encryptionKey[32];
    sha256.finalize(encryptionKey, 32);

    // Convert to vector for AES decryption
    std::vector<byte> keyVector(encryptionKey, encryptionKey + 32);

    logdbg_ln("Curve25519 ECC decryption: input=%d bytes", data.size());

    // Decrypt data with AES using derived key - input data is pure encrypted content
    return decryptAES(data, keyVector);
}

std::vector<byte> EncryptionService::encryptAES(const std::vector<byte>& data,
                                                const std::vector<byte>& key)
{
    if (!aesCrypto) {
        aesCrypto = AesCrypto::getInstance();
    }

    SecurityParams params;
    params.method = SecurityMethod::AES;
    params.key = key; // typically the network key
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
