#pragma once

#include <common/inc/Definitions.h>
#include <core/protocol/inc/crypto/cmac/AesCmac.h>
#include <vector>

class EncryptionService;

/**
 * @class MicService
 * @brief Service for context-aware MIC computation and verification
 *
 * Handles Message Integrity Check (MIC) operations for RadioMesh packets.
 * Selects appropriate keys based on message type and inclusion state:
 * - INCLUDE_REQUEST/RESPONSE: ECIES k_mac from ECDH
 * - INCLUDE_CONFIRM/SUCCESS: Network key
 * - Regular messages: Network key
 * - INCLUDE_OPEN: No MIC (returns empty)
 */
class MicService
{
public:
    /**
     * @brief Constructor
     * @param encryptionService Reference to encryption service for key access
     */
    explicit MicService(EncryptionService* encryptionService);

    /**
     * @brief Compute MIC for packet data
     * @param header Complete packet header (35 bytes)
     * @param encryptedPayload Encrypted payload data
     * @param topic Message topic for key selection
     * @param deviceType Device type (hub vs standard)
     * @param inclusionState Current inclusion state
     * @return 4-byte MIC or empty vector if no MIC needed
     */
    std::vector<byte> computePacketMIC(const std::vector<byte>& header,
                                      const std::vector<byte>& encryptedPayload,
                                      uint8_t topic,
                                      MeshDeviceType deviceType,
                                      DeviceInclusionState inclusionState);

    /**
     * @brief Verify packet MIC
     * @param header Complete packet header (35 bytes)
     * @param encryptedPayload Encrypted payload data (without MIC)
     * @param receivedMic MIC to verify (4 bytes)
     * @param topic Message topic for key selection
     * @param deviceType Device type (hub vs standard)
     * @param inclusionState Current inclusion state
     * @return true if MIC is valid, false otherwise
     */
    bool verifyPacketMIC(const std::vector<byte>& header,
                        const std::vector<byte>& encryptedPayload,
                        const std::vector<byte>& receivedMic,
                        uint8_t topic,
                        MeshDeviceType deviceType,
                        DeviceInclusionState inclusionState);

    /**
     * @brief Extract MIC from payload
     * @param payloadWithMic Complete payload including MIC
     * @return MIC bytes (4 bytes) or empty if payload too short
     */
    static std::vector<byte> extractMIC(const std::vector<byte>& payloadWithMic);

    /**
     * @brief Get payload without MIC
     * @param payloadWithMic Complete payload including MIC
     * @return Payload data without MIC
     */
    static std::vector<byte> getPayloadWithoutMIC(const std::vector<byte>& payloadWithMic);

    /**
     * @brief Append MIC to payload
     * @param payload Original payload data
     * @param mic MIC to append (4 bytes)
     * @return Combined payload + MIC
     */
    static std::vector<byte> appendMIC(const std::vector<byte>& payload, 
                                      const std::vector<byte>& mic);

    /**
     * @brief Check if topic requires MIC
     * @param topic Message topic
     * @return true if MIC is required, false otherwise
     */
    static bool requiresMIC(uint8_t topic);

private:
    /**
     * @brief Get MIC key based on context
     * @param topic Message topic
     * @param deviceType Device type
     * @param inclusionState Inclusion state
     * @return Key for MIC computation or empty if no MIC needed
     */
    std::vector<byte> getMICKey(uint8_t topic,
                               MeshDeviceType deviceType,
                               DeviceInclusionState inclusionState);

    /**
     * @brief Get ECIES MAC key for inclusion messages
     * @param topic Message topic (INCLUDE_REQUEST or INCLUDE_RESPONSE)
     * @param deviceType Device type
     * @return ECIES k_mac key or empty if unavailable
     */
    std::vector<byte> getECIESMacKey(uint8_t topic, MeshDeviceType deviceType);

    EncryptionService* encryptionService;
};