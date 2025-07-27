#include <core/protocol/inc/crypto/MicService.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <core/protocol/inc/packet/Topics.h>
#include <common/inc/Logger.h>
#include <Curve25519.h>
#include <SHA256.h>

MicService::MicService(EncryptionService* encryptionService)
    : encryptionService(encryptionService)
{
}

std::vector<byte> MicService::computePacketMIC(const std::vector<byte>& header,
                                              const std::vector<byte>& encryptedPayload,
                                              uint8_t topic,
                                              MeshDeviceType deviceType,
                                              DeviceInclusionState inclusionState)
{
    if (!requiresMIC(topic)) {
        logdbg_ln("Topic 0x%02X does not require MIC", topic);
        return std::vector<byte>();
    }

    std::vector<byte> micKey = getMICKey(topic, deviceType, inclusionState);
    if (micKey.empty()) {
        logerr_ln("No MIC key available for topic 0x%02X", topic);
        return std::vector<byte>();
    }

    // Combine header and encrypted payload for MIC computation
    std::vector<byte> dataToAuthenticate;
    dataToAuthenticate.reserve(header.size() + encryptedPayload.size());
    dataToAuthenticate.insert(dataToAuthenticate.end(), header.begin(), header.end());
    dataToAuthenticate.insert(dataToAuthenticate.end(), encryptedPayload.begin(), encryptedPayload.end());

    std::vector<byte> mic = AesCmac::computeMIC(micKey, dataToAuthenticate);
    if (mic.size() != 4) {
        logerr_ln("Failed to compute MIC for topic 0x%02X", topic);
        return std::vector<byte>();
    }

    logdbg_ln("Computed MIC for topic 0x%02X, data size=%d", topic, dataToAuthenticate.size());
    return mic;
}

bool MicService::verifyPacketMIC(const std::vector<byte>& header,
                                const std::vector<byte>& encryptedPayload,
                                const std::vector<byte>& receivedMic,
                                uint8_t topic,
                                MeshDeviceType deviceType,
                                DeviceInclusionState inclusionState)
{
    if (!requiresMIC(topic)) {
        logdbg_ln("Topic 0x%02X does not require MIC verification", topic);
        return true;
    }

    if (receivedMic.size() != 4) {
        logerr_ln("Invalid MIC size: %d (expected 4)", receivedMic.size());
        return false;
    }

    std::vector<byte> micKey = getMICKey(topic, deviceType, inclusionState);
    if (micKey.empty()) {
        logerr_ln("No MIC key available for topic 0x%02X verification", topic);
        return false;
    }

    // Combine header and encrypted payload for MIC verification
    std::vector<byte> dataToAuthenticate;
    dataToAuthenticate.reserve(header.size() + encryptedPayload.size());
    dataToAuthenticate.insert(dataToAuthenticate.end(), header.begin(), header.end());
    dataToAuthenticate.insert(dataToAuthenticate.end(), encryptedPayload.begin(), encryptedPayload.end());

    bool isValid = AesCmac::verifyMIC(micKey, dataToAuthenticate, receivedMic);
    
    if (isValid) {
        logdbg_ln("MIC verification passed for topic 0x%02X", topic);
    } else {
        logerr_ln("MIC verification FAILED for topic 0x%02X", topic);
    }

    return isValid;
}

std::vector<byte> MicService::extractMIC(const std::vector<byte>& payloadWithMic)
{
    if (payloadWithMic.size() < 4) {
        logerr_ln("Payload too short to contain MIC: %d bytes", payloadWithMic.size());
        return std::vector<byte>();
    }

    // MIC is the last 4 bytes
    return std::vector<byte>(payloadWithMic.end() - 4, payloadWithMic.end());
}

std::vector<byte> MicService::getPayloadWithoutMIC(const std::vector<byte>& payloadWithMic)
{
    if (payloadWithMic.size() < 4) {
        logerr_ln("Payload too short to contain MIC: %d bytes", payloadWithMic.size());
        return payloadWithMic;
    }

    // Return everything except the last 4 bytes
    return std::vector<byte>(payloadWithMic.begin(), payloadWithMic.end() - 4);
}

std::vector<byte> MicService::appendMIC(const std::vector<byte>& payload, 
                                       const std::vector<byte>& mic)
{
    if (mic.size() != 4) {
        logerr_ln("Invalid MIC size for append: %d (expected 4)", mic.size());
        return payload;
    }

    std::vector<byte> result;
    result.reserve(payload.size() + 4);
    result.insert(result.end(), payload.begin(), payload.end());
    result.insert(result.end(), mic.begin(), mic.end());
    
    return result;
}

bool MicService::requiresMIC(uint8_t topic)
{
    // No MIC for cleartext public key exchange messages
    return (topic != MessageTopic::INCLUDE_OPEN && 
            topic != MessageTopic::INCLUDE_REQUEST);
}

std::vector<byte> MicService::getMICKey(uint8_t topic,
                                       MeshDeviceType deviceType,
                                       DeviceInclusionState inclusionState)
{
    if (!encryptionService) {
        logerr_ln("EncryptionService not available for MIC key");
        return std::vector<byte>();
    }

    switch (topic) {
    case MessageTopic::INCLUDE_OPEN:
    case MessageTopic::INCLUDE_REQUEST:
        // No MIC for cleartext public key exchange messages
        return std::vector<byte>();

    case MessageTopic::INCLUDE_RESPONSE:
        // Use ECIES k_mac from ECDH
        return getECIESMacKey(topic, deviceType);

    case MessageTopic::INCLUDE_CONFIRM:
    case MessageTopic::INCLUDE_SUCCESS:
        // Use network key (device should have it at this point)
        return encryptionService->getNetworkKey();

    default:
        // Regular messages use network key
        if (inclusionState == DeviceInclusionState::INCLUDED || 
            deviceType == MeshDeviceType::HUB) {
            return encryptionService->getNetworkKey();
        }
        
        logerr_ln("Device not included, cannot get network key for MIC");
        return std::vector<byte>();
    }
}

std::vector<byte> MicService::getECIESMacKey(uint8_t topic, MeshDeviceType deviceType)
{
    if (!encryptionService) {
        logerr_ln("EncryptionService not available for ECIES MAC key");
        return std::vector<byte>();
    }

    std::vector<byte> privateKey, publicKey;
    
    if (topic == MessageTopic::INCLUDE_REQUEST) {
        // Device encrypting to hub: use device's private key + hub's public key
        privateKey = encryptionService->getDevicePrivateKey();
        publicKey = encryptionService->getHubPublicKey();
    } else if (topic == MessageTopic::INCLUDE_RESPONSE) {
        // Hub encrypting to device: use hub's private key + device's public key
        if (deviceType == MeshDeviceType::HUB) {
            privateKey = encryptionService->getDevicePrivateKey(); // Hub's private key
            publicKey = encryptionService->getTempDevicePublicKey(); // Device's public key
        } else {
            // Standard device decrypting from hub
            privateKey = encryptionService->getDevicePrivateKey(); // Device's private key
            publicKey = encryptionService->getHubPublicKey(); // Hub's public key
        }
    } else {
        logerr_ln("Invalid topic for ECIES MAC key: 0x%02X", topic);
        return std::vector<byte>();
    }

    if (privateKey.empty() || publicKey.empty()) {
        logerr_ln("Missing keys for ECIES MAC derivation - private key size: %d, public key size: %d", 
                 privateKey.size(), publicKey.size());
        return std::vector<byte>();
    }

    if (privateKey.size() != 32 || publicKey.size() != 32) {
        logerr_ln("Invalid key sizes for ECIES MAC derivation - private: %d, public: %d", 
                 privateKey.size(), publicKey.size());
        return std::vector<byte>();
    }

    // Perform ECDH to get shared secret
    uint8_t sharedSecret[32];
    if (!Curve25519::eval(sharedSecret, privateKey.data(), publicKey.data())) {
        logerr_ln("Failed to compute ECDH shared secret for MIC key");
        return std::vector<byte>();
    }

    // Derive k_mac using SHA256 (same as EncryptionService)
    SHA256 sha256;
    sha256.reset();
    sha256.update(sharedSecret, 32);
    uint8_t kmac[32];
    sha256.finalize(kmac, 32);

    logdbg_ln("Derived ECIES k_mac for topic 0x%02X", topic);
    return std::vector<byte>(kmac, kmac + 32);
}