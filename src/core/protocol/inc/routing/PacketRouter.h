#pragma once

#include <vector>
#include <string>
#include <functional>
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <common/utils/RadioMeshCrc32.h>

#include <core/protocol/inc/packet/Packet.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
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
   static PacketRouter *getInstance()
   {
      if (!instance) {
         instance = new PacketRouter();
      }
      return instance;
   }

   virtual ~PacketRouter() {}
   /**
    * @brief Route a packet to the next hop in the mesh network.
    * @param packet RadioMeshPacket to route
    * @return RM_E_NONE if the packet was successfully routed, an error code otherwise.
    */
   int routePacket(RadioMeshPacket packet, const byte *ourDeviceId);

   /**
    * @brief Check if a packet has already been tracked.
    * @param packet RadioMeshPacket to check
    * @return true if the packet has already been tracked, false otherwise.
    */
   bool isPacketFoundInTracker(RadioMeshPacket packet);

   /**
    * @brief Set the crypto component to use for encrypting and decrypting packets.
    * @param crypto AesCrypto component to use
    */
   void setCrypto(AesCrypto *crypto) { this->crypto = crypto; }

private:
   PacketRouter() {}
   PacketRouter(const PacketRouter &) = delete;
   void operator=(const PacketRouter &) = delete;

   PacketTracker packetTracker = PacketTracker(50);
   AesCrypto *crypto = nullptr;

   static PacketRouter *instance;

   bool checkMaxHops(RadioMeshPacket& packetCopy);
   void updateLastHopId(RadioMeshPacket& packetCopy, const byte* ourDeviceId);
   void routeToNextHop(RadioMeshPacket& packetCopy);
   void encryptPacketData(RadioMeshPacket& packetCopy);
   void calculatePacketCrc(RadioMeshPacket& packetCopy, RadioMeshUtils::CRC32& crc32, uint32_t key);
   int sendPacket(RadioMeshPacket& packetCopy);
   void trackPacket(RadioMeshPacket& packetCopy, uint32_t key);


};
