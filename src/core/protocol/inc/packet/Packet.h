#pragma once

#include <array>
#include <cstring>
#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <common/inc/Logger.h>
#include <common/utils/Utils.h>

// Protocol version
#define RM_PROTOCOL_VERSION 3
#define PROTOCOL_VERSION_LENGTH 1

// Packet size constants
#define PACKET_LENGTH 256
#define MAX_HOPS 7

// Header field lengths in bytes
#define PROTOCOL_VERSION_LENGTH 1
#define DEV_ID_LENGTH RM_ID_LENGTH
#define MSG_ID_LENGTH RM_ID_LENGTH
#define TOPIC_LENGTH 1
#define DEVICE_TYPE_LENGTH 1
#define HOP_COUNT_LENGTH 1
#define DATA_CRC_LENGTH 4
#define FCOUNTER_LENGTH 4
#define RESERVED_LENGTH 3

// Field positions in packet
#define VERSION_POS 0
#define SDEV_ID_POS (VERSION_POS + PROTOCOL_VERSION_LENGTH)
#define DDEV_ID_POS (SDEV_ID_POS + DEV_ID_LENGTH)
#define PKT_ID_POS (DDEV_ID_POS + DEV_ID_LENGTH)
#define TOPIC_POS (PKT_ID_POS + MSG_ID_LENGTH)
#define DEVICE_TYPE_POS (TOPIC_POS + 1)
#define HOP_COUNT_POS (DEVICE_TYPE_POS + 1)
#define DATA_CRC_POS (HOP_COUNT_POS + 1)
#define FCOUNTER_POS (DATA_CRC_POS + DATA_CRC_LENGTH)
#define LAST_HOP_ID_POS (FCOUNTER_POS + FCOUNTER_LENGTH)
#define NEXT_HOP_POS (LAST_HOP_ID_POS + DEV_ID_LENGTH)
#define RESERVED_POS (NEXT_HOP_POS + DEV_ID_LENGTH)
#define DATA_POS (RESERVED_POS + RESERVED_LENGTH)

#define HEADER_LENGTH DATA_POS

// Data/packet range constants
#define MAX_DATA_LENGTH (PACKET_LENGTH - HEADER_LENGTH)
#define MIN_PACKET_LENGTH (HEADER_LENGTH + 1)

/**
 * @class RadioMeshPacket
 * @brief Optimized packet structure for the mesh protocol
 *
 * Header Structure (35 bytes total):
 * - Protocol Version      (1 byte)  : Protocol version
 * - Source Device ID      (4 bytes) : Origin device identifier
 * - Destination Device ID (4 bytes) : Final destination identifier
 * - Packet ID             (4 bytes) : Unique message identifier
 * - Topic                 (1 byte)  : Message type/purpose
 * - Device Type           (1 byte)  : Source device type
 * - Hop Count             (1 byte)  : Number of relays
 * - Data CRC              (4 bytes) : Payload integrity check
 * - Frame Counter         (4 bytes) : Sequence number
 * - Last Hop ID           (4 bytes) : Previous relay identifier
 * - Next Hop ID           (4 bytes) : Next relay identifier
 * - Reserved              (3 bytes) : Future use
 */
class RadioMeshPacket
{
public:
    // Packet fields
    uint8_t protocolVersion;
    std::array<byte, DEV_ID_LENGTH> sourceDevId;
    std::array<byte, DEV_ID_LENGTH> destDevId;
    std::array<byte, MSG_ID_LENGTH> packetId;
    uint8_t topic;
    uint8_t deviceType;
    uint8_t hopCount;
    uint32_t packetCrc;
    uint32_t fcounter;
    std::array<byte, DEV_ID_LENGTH> lastHopId;
    std::array<byte, DEV_ID_LENGTH> nextHopId;
    std::array<byte, RESERVED_LENGTH> reserved;
    std::vector<byte> packetData;
    /**
     * @brief Default constructor - initializes empty packet
     */
    RadioMeshPacket()
    {
        protocolVersion = RM_PROTOCOL_VERSION;
        sourceDevId.fill(0);
        destDevId.fill(0);
        packetId.fill(0);
        lastHopId.fill(0);
        nextHopId.fill(0);
        reserved.fill(0);

        topic = MessageTopic::UNUSED;
        deviceType = MeshDeviceType::UNKNOWN;
        hopCount = 0;
        packetCrc = 0;
        fcounter = 0;
    }

    /**
     * @brief Construct packet from byte buffer
     * @param buffer Raw packet data
     */
    explicit RadioMeshPacket(const std::vector<byte>& buffer)
    {
        // Simple sequential field parsing
        protocolVersion = buffer[VERSION_POS];

        // IDs (4 bytes each)

        // use array copy for better performance
        std::copy_n(buffer.begin() + SDEV_ID_POS, DEV_ID_LENGTH, sourceDevId.begin());
        std::copy_n(buffer.begin() + DDEV_ID_POS, DEV_ID_LENGTH, destDevId.begin());
        std::copy_n(buffer.begin() + PKT_ID_POS, MSG_ID_LENGTH, packetId.begin());

        // Single byte fields
        topic = buffer[TOPIC_POS];
        deviceType = buffer[DEVICE_TYPE_POS];
        hopCount = buffer[HOP_COUNT_POS];

        // Multi-byte fields
        packetCrc = (buffer[DATA_CRC_POS] << 24) | (buffer[DATA_CRC_POS + 1] << 16) |
                    (buffer[DATA_CRC_POS + 2] << 8) | buffer[DATA_CRC_POS + 3];

        fcounter = (buffer[FCOUNTER_POS] << 24) | (buffer[FCOUNTER_POS + 1] << 16) |
                   (buffer[FCOUNTER_POS + 2] << 8) | buffer[FCOUNTER_POS + 3];

        // Routing IDs
        std::copy_n(buffer.begin() + LAST_HOP_ID_POS, DEV_ID_LENGTH, lastHopId.begin());
        std::copy_n(buffer.begin() + NEXT_HOP_POS, DEV_ID_LENGTH, nextHopId.begin());
        // Reserved and data
        std::copy_n(buffer.begin() + RESERVED_POS, RESERVED_LENGTH, reserved.begin());
        packetData.assign(buffer.begin() + DATA_POS, buffer.end());
    }

    /**
     * @brief Convert packet to byte buffer
     * @return Vector containing serialized packet
     */
    std::vector<byte> toByteBuffer() const
    {
        std::vector<byte> buffer;
        buffer.reserve(HEADER_LENGTH + packetData.size());

        // Add header fields
        buffer.push_back(protocolVersion);
        buffer.insert(buffer.end(), sourceDevId.begin(), sourceDevId.end());
        buffer.insert(buffer.end(), destDevId.begin(), destDevId.end());
        buffer.insert(buffer.end(), packetId.begin(), packetId.end());
        buffer.push_back(topic);
        buffer.push_back(deviceType);
        buffer.push_back(hopCount);

        buffer.push_back((packetCrc >> 24) & 0xFF);
        buffer.push_back((packetCrc >> 16) & 0xFF);
        buffer.push_back((packetCrc >> 8) & 0xFF);
        buffer.push_back(packetCrc & 0xFF);

        buffer.push_back((fcounter >> 24) & 0xFF);
        buffer.push_back((fcounter >> 16) & 0xFF);
        buffer.push_back((fcounter >> 8) & 0xFF);
        buffer.push_back(fcounter & 0xFF);

        buffer.insert(buffer.end(), lastHopId.begin(), lastHopId.end());
        buffer.insert(buffer.end(), nextHopId.begin(), nextHopId.end());
        buffer.insert(buffer.end(), reserved.begin(), reserved.end());

        // Add data section
        buffer.insert(buffer.end(), packetData.begin(), packetData.end());

        return buffer;
    }

    /**
     * @brief Reset packet to initial state
     */
    void reset()
    {
        protocolVersion = RM_PROTOCOL_VERSION;
        sourceDevId.fill(0);
        destDevId.fill(0);
        packetId.fill(0);
        lastHopId.fill(0);
        nextHopId.fill(0);
        reserved.fill(0);
        std::vector<byte>().swap(packetData);

        topic = MessageTopic::UNUSED;
        deviceType = MeshDeviceType::UNKNOWN;
        hopCount = 0;
        packetCrc = 0;
        fcounter = 0;
    }

    /**
     * @brief Log packet contents for debugging
     */
    void log() const
    {
        logdbg_ln("Packet dump:");
        logdbg_ln("  Protocol Version: %d", protocolVersion);
        logdbg_ln("  Source ID: %s",
                  RadioMeshUtils::convertToHex(sourceDevId.data(), sourceDevId.size()).c_str());
        logdbg_ln("  Dest ID: %s",
                  RadioMeshUtils::convertToHex(destDevId.data(), destDevId.size()).c_str());
        logdbg_ln("  Packet ID: 0x%s",
                  RadioMeshUtils::convertToHex(packetId.data(), packetId.size()).c_str());
        logdbg_ln("  Topic: %s", TopicUtils::topicToString(topic).c_str());
        logdbg_ln("  Device Type: %d", deviceType);
        logdbg_ln("  Hop Count: %d", hopCount);
        logdbg_ln("  CRC: 0x%04X", packetCrc);
        logdbg_ln("  Frame Counter: %d", fcounter);
        logdbg_ln("  Last Hop: %s",
                  RadioMeshUtils::convertToHex(lastHopId.data(), lastHopId.size()).c_str());
        logdbg_ln("  Next Hop: %s",
                  RadioMeshUtils::convertToHex(nextHopId.data(), nextHopId.size()).c_str());
        logdbg_ln("  Reserved: %s",
                  RadioMeshUtils::convertToHex(reserved.data(), reserved.size()).c_str());
        logdbg_ln("  Data Length: %d bytes", packetData.size());
        if (!packetData.empty()) {
            logdbg_ln("  Data: %s",
                      RadioMeshUtils::convertToHex(packetData.data(), packetData.size()).c_str());
        }
    }

    /**
     * @brief Get maximum allowed data length
     * @return Maximum data payload size
     */
    static constexpr size_t getMaxDataLength()
    {
        return MAX_DATA_LENGTH;
    }

    static bool isInclusionTopic(uint8_t topic)
    {
        return (topic == MessageTopic::INCLUDE_REQUEST || topic == MessageTopic::INCLUDE_RESPONSE ||
                topic == MessageTopic::INCLUDE_OPEN || topic == MessageTopic::INCLUDE_CONFIRM);
    }
    /**
     * @brief Assignment operator
     */
    RadioMeshPacket& operator=(const RadioMeshPacket& other)
    {
        if (this != &other) {
            sourceDevId = other.sourceDevId;
            destDevId = other.destDevId;
            packetId = other.packetId;
            topic = other.topic;
            deviceType = other.deviceType;
            hopCount = other.hopCount;
            packetCrc = other.packetCrc;
            fcounter = other.fcounter;
            lastHopId = other.lastHopId;
            nextHopId = other.nextHopId;
            reserved = other.reserved;
            packetData = other.packetData;
        }
        return *this;
    }
};
