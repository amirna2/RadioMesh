#pragma once

#include <common/inc/Definitions.h>
#include <memory>
#include <vector>
#include <array>

class AesCrypto;
class AesCmac;
class KeyManager;

/**
 * @class EncryptionService
 * @brief Central encryption service that handles all packet encryption/decryption
 *
 * This service determines the appropriate encryption method and keys based on:
 * - Device type (Hub vs Standard)
 * - Inclusion state (Not included, pending, included)
 * - Message topic (inclusion messages vs regular messages)
 *
 * It replaces direct AesCrypto usage in PacketRouter to provide context-aware encryption.
 */
class EncryptionService
{
public:
    /**
     * @brief Encrypt packet data based on context
     * @param data The data to encrypt
     * @param topic The message topic
     * @param deviceType The type of device performing encryption
     * @param inclusionState The current inclusion state of the device
     * @return Encrypted data (or original data if no encryption needed)
     */
    std::vector<byte> encrypt(const std::vector<byte>& data, uint8_t topic,
                              MeshDeviceType deviceType, DeviceInclusionState inclusionState);

    /**
     * @brief Decrypt packet data based on context
     * @param data The data to decrypt
     * @param topic The message topic
     * @param deviceType The type of device performing decryption
     * @param inclusionState The current inclusion state of the device
     * @return Decrypted data (or original data if no decryption needed)
     */
    std::vector<byte> decrypt(const std::vector<byte>& data, uint8_t topic,
                              MeshDeviceType deviceType, DeviceInclusionState inclusionState);

    /**
     * @brief Set the shared network key for AES encryption
     * @param key The network key
     */
    void setNetworkKey(const std::vector<byte>& key);

    /**
     * @brief Set the device's key pair for direct ECC operations
     * @param privateKey The device's private key
     * @param publicKey The device's public key
     */
    void setDeviceKeys(const std::vector<byte>& privateKey, const std::vector<byte>& publicKey);

    /**
     * @brief Set the hub's public key (for standard devices during inclusion)
     * @param hubPublicKey The hub's public key
     */
    void setHubPublicKey(const std::vector<byte>& hubPublicKey);

    /**
     * @brief Set a temporary device public key (for hub during inclusion)
     * @param devicePublicKey The device's public key received in INCLUDE_REQUEST
     */
    void setTempDevicePublicKey(const std::vector<byte>& devicePublicKey);

    /**
     * @brief Decrypt data using direct ECC (public for manual decryption)
     * @param data Encrypted data
     * @param privateKey Private key for decryption
     * @return Decrypted data
     */
    std::vector<byte> decryptDirectECC(const std::vector<byte>& data,
                                       const std::vector<byte>& privateKey);

    /**
     * @brief Encrypt data using direct ECC (public for manual encryption)
     * @param data Data to encrypt
     * @param publicKey Public key for encryption
     * @return Encrypted data
     */
    std::vector<byte> encryptDirectECC(const std::vector<byte>& data,
                                       const std::vector<byte>& publicKey);

    /**
     * @brief Compute MIC for packet authentication
     * @param packetData Complete packet data (header + encrypted payload)
     * @param mic Output buffer for 4-byte MIC
     * @return RM_E_NONE on success, error code otherwise
     */
    int computeMIC(const std::vector<byte>& packetData, std::array<byte, 4>& mic);

    /**
     * @brief Verify MIC for received packet
     * @param packetData Complete packet data (header + encrypted payload, excluding MIC)
     * @param receivedMic The 4-byte MIC to verify
     * @return true if MIC is valid, false otherwise
     */
    bool verifyMIC(const std::vector<byte>& packetData, const std::array<byte, 4>& receivedMic);

    /**
     * @brief Check if MIC should be used for this packet
     * @param topic Message topic
     * @param protocolVersion Protocol version from packet
     * @return true if MIC authentication is required
     */
    bool shouldUseMIC(uint8_t topic, uint8_t protocolVersion) const;

private:
    enum class EncryptionMethod
    {
        NONE,       // No encryption
        DIRECT_ECC, // Direct Elliptic Curve Encryption (zero overhead)
        AES         // AES with shared network key
    };

    EncryptionMethod determineCryptoMethod(uint8_t topic, MeshDeviceType deviceType,
                                           DeviceInclusionState inclusionState) const;
    std::vector<byte> getEncryptionKey(EncryptionMethod method, uint8_t topic,
                                       MeshDeviceType deviceType) const;
    std::vector<byte> getDecryptionKey(EncryptionMethod method, uint8_t topic,
                                       MeshDeviceType deviceType) const;
    std::vector<byte> encryptAES(const std::vector<byte>& data, const std::vector<byte>& key);
    std::vector<byte> decryptAES(const std::vector<byte>& data, const std::vector<byte>& key);

    // Key storage
    std::vector<byte> networkKey;
    std::vector<byte> devicePrivateKey;
    std::vector<byte> devicePublicKey;
    std::vector<byte> hubPublicKey;
    std::vector<byte> tempDevicePublicKey;

    // Crypto instances
    AesCrypto* aesCrypto = nullptr;
    AesCmac* aesCmac = nullptr;
};
