#pragma once

#include <array>
#include <core/protocol/inc/packet/Packet.h>
#include <common/inc/Definitions.h>

// Maximum number of routes to store
#define MAX_ROUTES 10
// Route timeout in milliseconds
#define ROUTE_TIMEOUT 300000  // 5 minutes

// Return value for not found
#define NOT_FOUND -1
struct RouteEntry {
    std::array<byte, DEV_ID_LENGTH> destId;    // 8 bytes for device ID
    std::array<byte, DEV_ID_LENGTH> nextHopId; // 8 bytes for next hop
    uint8_t hops;                   // 1 byte for hop count
    int8_t rssi;                    // 1 byte for RSSI
    uint32_t lastSeen;              // 4 bytes for timestamp
    bool active;                    // 1 byte for active flag
};
