#include <string>
#include <vector>

#include <core/protocol/inc/routing/PacketRouter.h>
#include <hardware/inc/radio/LoraRadio.h>

PacketRouter* PacketRouter::instance = nullptr;

int PacketRouter::routePacket(RadioMeshPacket packet, const byte* ourDeviceId,
                              MeshDeviceType deviceType, DeviceInclusionState inclusionState)
{
    RadioMeshUtils::CRC32 crc32;
    RadioMeshPacket packetCopy = packet;
    uint32_t key = RadioMeshUtils::toUint32(packetCopy.packetId.data());

    loginfo_ln("Routing packet with ID: 0x%X", key);

    if (checkMaxHops(packetCopy)) {
        return RM_E_MAX_HOPS;
    }

    updateLastHopId(packetCopy, ourDeviceId);
    loginfo_ln("Routing packet with ID: 0x%X, hop count: %d", key, packetCopy.hopCount);

    routeToNextHop(packetCopy);

    packetCopy.reserved.fill(0);

    // For relayed packets, ensure any existing MIC is stripped before recomputation
    if (packetCopy.hasMIC()) {
        logdbg_ln("Stripping existing MIC from relayed packet for topic 0x%02X", packetCopy.topic);
        packetCopy.packetData = packetCopy.getDataWithoutMIC();
    }

    if (packetCopy.topic != MessageTopic::INCLUDE_OPEN) {
        encryptPacketData(packetCopy, deviceType, inclusionState);
    }

    // Compute and append MIC after encryption
    int micResult = computeAndAppendMIC(packetCopy, deviceType, inclusionState);
    if (micResult != RM_E_NONE) {
        return micResult;
    }

    calculatePacketCrc(packetCopy, crc32, key);
    int rc = sendPacket(packetCopy);
    if (rc != RM_E_NONE) {
        return rc;
    }

    trackPacket(packetCopy, key);

    return RM_E_NONE;
}

bool PacketRouter::checkMaxHops(RadioMeshPacket& packetCopy)
{
    if (packetCopy.hopCount >= MAX_HOPS) {
        loginfo_ln("Max hops reached, dropping packet ID: %s",
                   RadioMeshUtils::convertToHex(packetCopy.packetId.data(), MSG_ID_LENGTH).c_str());
        return true;
    }
    return false;
}

void PacketRouter::updateLastHopId(RadioMeshPacket& packetCopy, const byte* ourDeviceId)
{
    std::copy_n(ourDeviceId, DEV_ID_LENGTH, packetCopy.lastHopId.begin());
    packetCopy.hopCount++;
}

void PacketRouter::routeToNextHop(RadioMeshPacket& packetCopy)
{
    if (!RadioMeshUtils::isBroadcastAddress(packetCopy.destDevId)) {
        byte nextHop[DEV_ID_LENGTH];
        if (RoutingTable::getInstance()->findNextHop(packetCopy.destDevId.data(), nextHop)) {
            loginfo_ln(
                "Found route to %s via %s",
                RadioMeshUtils::convertToHex(packetCopy.destDevId.data(), DEV_ID_LENGTH).c_str(),
                RadioMeshUtils::convertToHex(nextHop, DEV_ID_LENGTH).c_str());
            memcpy(packetCopy.nextHopId.data(), nextHop, DEV_ID_LENGTH);
        } else {
            loginfo_ln("No route found, broadcasting");
            memset(packetCopy.nextHopId.data(), 0, DEV_ID_LENGTH);
        }
    }
}

void PacketRouter::encryptPacketData(RadioMeshPacket& packetCopy, MeshDeviceType deviceType,
                                     DeviceInclusionState inclusionState)
{
    if (packetCopy.packetData.size() == 0) {
        return;
    }

    if (encryptionService == nullptr) {
        logerr_ln("Encryption service not set, cannot encrypt packet data");
        return;
    }
    packetCopy.packetData = encryptionService->encrypt(packetCopy.packetData, packetCopy.topic,
                                                       deviceType, inclusionState);
}

void PacketRouter::calculatePacketCrc(RadioMeshPacket& packetCopy, RadioMeshUtils::CRC32& crc32,
                                      uint32_t key)
{
    loginfo_ln("Calculating packet crc for packet ID: 0x%X", key);
    loginfo_ln("  Frame Counter: %d", packetCopy.fcounter);
    loginfo_ln("  Data: %s", RadioMeshUtils::convertToHex(packetCopy.packetData.data(),
                                                          packetCopy.packetData.size())
                                 .c_str());
    crc32.update(packetCopy.fcounter);
    if (packetCopy.packetData.size() > 0) {
        crc32.update(packetCopy.packetData.data(), packetCopy.packetData.size());
    }
    packetCopy.packetCrc = crc32.finalize();
    std::string pktId =
        RadioMeshUtils::convertToHex(packetCopy.packetId.data(), packetCopy.packetId.size());
    loginfo_ln("Routing packet with id: %s crc: 0x%4X", pktId.c_str(), packetCopy.packetCrc);
    packetCopy.log();
}

int PacketRouter::sendPacket(RadioMeshPacket& packetCopy)
{
    std::vector<byte> buffer = packetCopy.toByteBuffer();
    int rc = LoraRadio::getInstance()->sendPacket(buffer);
    if (rc != RM_E_NONE) {
        logerr_ln("Failed to send packet");
    }
    return rc;
}

void PacketRouter::trackPacket(RadioMeshPacket& packetCopy, uint32_t key)
{
    loginfo_ln("Tracking packet with ID: 0x%X, data crc: 0x%X", key, packetCopy.packetCrc);
    packetTracker.addEntry(key, packetCopy.packetCrc);
}

int PacketRouter::computeAndAppendMIC(RadioMeshPacket& packetCopy, MeshDeviceType deviceType,
                                      DeviceInclusionState inclusionState)
{
    if (!micService) {
        logerr_ln("CRITICAL: MIC service not available");
        return RM_E_DEVICE_INITIALIZATION_FAILED;
    }

    if (packetCopy.topic == MessageTopic::INCLUDE_OPEN || 
        packetCopy.topic == MessageTopic::INCLUDE_REQUEST) {
        return RM_E_NONE;
    }

    std::vector<byte> header = packetCopy.getHeaderBytes();
    std::vector<byte> encryptedPayload = packetCopy.packetData;

    std::vector<byte> mic = micService->computePacketMIC(header, encryptedPayload, 
                                                        packetCopy.topic, deviceType, inclusionState);
    
    if (mic.empty()) {
        logerr_ln("Failed to compute MIC for topic 0x%02X", packetCopy.topic);
        return RM_E_AUTH_FAILED;
    }

    packetCopy.appendMIC(mic);
    return RM_E_NONE;
}

bool PacketRouter::isPacketFoundInTracker(RadioMeshPacket packet)
{
    uint32_t key = RadioMeshUtils::toUint32(packet.packetId.data());
    uint32_t value = packet.packetCrc;

    // find the packet ID key in our tracker and compare the data crc.

    uint32_t foundValue = packetTracker.findOrDefault(key, 0);
    if (foundValue == value) {
        loginfo_ln("Packet with ID [%s] already seen.", std::to_string(key).c_str());
        return true;
    }
    return false;
}
