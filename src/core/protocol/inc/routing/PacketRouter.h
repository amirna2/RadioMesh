#pragma once

#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <common/utils/RadioMeshCrc32.h>
#include <functional>
#include <string>
#include <vector>

#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <core/protocol/inc/crypto/MicService.h>
#include <core/protocol/inc/packet/Packet.h>
#include <core/protocol/inc/routing/PacketTracker.h>
#include <core/protocol/inc/routing/RoutingTable.h>

/**
 * @class PacketRouter
 * @brief This class is responsible for routing packets in the mesh network.
 *
 * It provides methods to route packets to the next hop in the mesh network,
 * handle packets received from the mesh network, and check if a packet has already been processed.
 */
class PacketRouter
{
public:
    /**
     * @brief Get the instance of the PacketRouter.
     * @return PacketRouter instance
     */
    static PacketRouter* getInstance()
    {
        if (!instance) {
            instance = new PacketRouter();
        }
        return instance;
    }

    virtual ~PacketRouter()
    {
    }
    /**
     * @brief Route a packet to the next hop in the mesh network.
     * @param packet RadioMeshPacket to route
     * @param ourDeviceId Our device ID
     * @param deviceType Type of device (Hub or Standard)
     * @param inclusionState Current inclusion state
     * @return RM_E_NONE if the packet was successfully routed, an error code otherwise.
     */
    int routePacket(RadioMeshPacket packet, const byte* ourDeviceId, 
                   MeshDeviceType deviceType, DeviceInclusionState inclusionState);

    /**
     * @brief Check if a packet has already been tracked.
     * @param packet RadioMeshPacket to check
     * @return true if the packet has already been tracked, false otherwise.
     */
    bool isPacketFoundInTracker(RadioMeshPacket packet);

    /**
     * @brief Set the encryption service to use for encrypting and decrypting packets.
     * @param encryptionService EncryptionService component to use
     */
    void setEncryptionService(EncryptionService* encryptionService)
    {
        this->encryptionService = encryptionService;
    }

    /**
     * @brief Set the MIC service for packet authentication
     * @param micService MicService component to use
     */
    void setMicService(MicService* micService)
    {
        this->micService = micService;
    }


    /**
     * @brief Set the crypto component to use for encrypting and decrypting packets.
     * @param crypto AesCrypto component to use
     * @deprecated Use setEncryptionService instead
     */
    void setCrypto(AesCrypto* crypto)
    {
        this->crypto = crypto;
    }

private:
    PacketRouter()
    {
    }
    PacketRouter(const PacketRouter&) = delete;
    void operator=(const PacketRouter&) = delete;

    PacketTracker packetTracker = PacketTracker(50);
    AesCrypto* crypto = nullptr;
    EncryptionService* encryptionService = nullptr;
    MicService* micService = nullptr;

    static PacketRouter* instance;

    bool checkMaxHops(RadioMeshPacket& packetCopy);
    void updateLastHopId(RadioMeshPacket& packetCopy, const byte* ourDeviceId);
    void routeToNextHop(RadioMeshPacket& packetCopy);
    void encryptPacketData(RadioMeshPacket& packetCopy, MeshDeviceType deviceType, DeviceInclusionState inclusionState);
    int computeAndAppendMIC(RadioMeshPacket& packetCopy, MeshDeviceType deviceType, DeviceInclusionState inclusionState);
    void calculatePacketCrc(RadioMeshPacket& packetCopy, RadioMeshUtils::CRC32& crc32,
                            uint32_t key);
    int sendPacket(RadioMeshPacket& packetCopy);
    void trackPacket(RadioMeshPacket& packetCopy, uint32_t key);
};
