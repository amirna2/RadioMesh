#pragma once

#include "RoutingTypes.h"
#include <core/protocol/inc/packet/Packet.h>
#include <common/inc/Logger.h>

class RoutingTable
{
public:
    /*
        Typical LoRa RSSI ranges:

        Excellent: -90 dBm or higher
        Good: -100 to -90 dBm
        Fair: -110 to -100 dBm
        Poor: -120 to -110 dBm
        Very Poor: Below -120 dBm
    */
    //  RSSI typically changes in steps of about 6dB for meaningful signal strength changes
    static const int8_t RSSI_UPPER_THRESHOLD = 12;  // Clear improvement needed to switch
    static const int8_t RSSI_LOWER_THRESHOLD = 6;   // More tolerant of existing route degradation

    static RoutingTable* getInstance();

    // Update or add route based on received packet
    void updateRoute(const RadioMeshPacket& packet, int8_t rssi);

    // Find next hop for destination
    bool findNextHop(const byte* destId, byte* nextHop);

    // Debug function to print current routes
    void printRoutes();

private:
    RoutingTable();
    static RoutingTable* instance;
    RouteEntry routes[MAX_ROUTES];

    int findRoute(const byte* destId);
    int findEmptySlot();
    bool isBetterRoute(const RouteEntry& newRoute, const RouteEntry& existingRoute);
};
