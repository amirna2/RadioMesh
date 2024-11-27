
#include <Arduino.h>
#include  <core/protocol/inc/routing/RoutingTable.h>

RoutingTable* RoutingTable::instance = nullptr;

RoutingTable* RoutingTable::getInstance()
{
    if (!instance) {
        instance = new RoutingTable();
    }
    return instance;
}

RoutingTable::RoutingTable() {
    for (int i = 0; i < MAX_ROUTES; i++) {
        routes[i].active = false;
    }
}

void RoutingTable::updateRoute(const RadioMeshPacket& packet, int8_t rssi)
{
    // Don't store routes for packets that are near hop limit
    if (packet.hopCount >= (MAX_HOPS - 1)) {
        loginfo_ln("Not storing route for packet near hop limit");
        return;
    }

    RouteEntry newRoute;
    std::copy(packet.sourceDevId.begin(), packet.sourceDevId.end(), newRoute.destId.begin());
    std::copy(packet.lastHopId.begin(), packet.lastHopId.end(), newRoute.nextHopId.begin());
    newRoute.hops = packet.hopCount;
    newRoute.rssi = rssi;
    newRoute.lastSeen = millis();
    newRoute.active = true;

    int index = findRoute(packet.sourceDevId.data());
    if (index != NOT_FOUND) {
        // Update existing route if new route is better
        if (isBetterRoute(newRoute, routes[index])) {
            routes[index] = newRoute;
            loginfo_ln("Updated route via better relay: RSSI=%d, hops=%d",
                rssi, packet.hopCount);
        }
    } else {
        // Add new route
        index = findEmptySlot();
        if (index != NOT_FOUND) {
            routes[index] = newRoute;
            loginfo_ln("Added new route via relay: RSSI=%d, hops=%d",
                rssi, packet.hopCount);
        } else {
            // This should not happen since findEmptySlot() checks for oldest route as last resort
            // but we log it just in case
            logwarn_ln("No empty slots for new route");
        }
    }
}

bool RoutingTable::findNextHop(const byte* destId, byte* nextHop)
{
    int index = findRoute(destId);
    if (index != NOT_FOUND && routes[index].active) {
        if (millis() - routes[index].lastSeen < ROUTE_TIMEOUT) {
            std::copy(routes[index].nextHopId.begin(), routes[index].nextHopId.end(), nextHop);
            return true;
        } else {
            routes[index].active = false;
            loginfo_ln("Route to %s expired", RadioMeshUtils::convertToHex(routes[index].destId.data(), DEV_ID_LENGTH).c_str());
        }
    }
    return false;
}

int RoutingTable::findRoute(const byte* destId)
{
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (std::equal(routes[i].destId.begin(), routes[i].destId.end(), destId)) {
            return i;
        }
    }
    return NOT_FOUND;
}

int RoutingTable::findEmptySlot()
{
    // First try to find an inactive slot
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (!routes[i].active) {
            return i;
        }
    }
    // If no inactive slots, find oldest route
    uint32_t oldestTime = millis();
    int oldestIndex = 0;
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (routes[i].lastSeen < oldestTime) {
            oldestTime = routes[i].lastSeen;
            oldestIndex = i;
        }
    }
    return oldestIndex;
}

bool RoutingTable::isBetterRoute(const RouteEntry& newRoute, const RouteEntry& existingRoute)
{
    // Switch to new route if it's RSSI_UPPER_THRESHOLD or better
    if (newRoute.rssi > (existingRoute.rssi + RSSI_UPPER_THRESHOLD)) {
        return true;
    }

    // Keep existing route unless it degrades by RSSI_LOWER_THRESHOLD or more
    if (newRoute.rssi < (existingRoute.rssi - RSSI_LOWER_THRESHOLD)) {
        return false;
    }

    // Within acceptable band, use hop count
    return newRoute.hops < existingRoute.hops;
}

void RoutingTable::printRoutes()
{
    loginfo_ln("Current Routes:");
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (routes[i].active) {
            std::string destId = RadioMeshUtils::convertToHex(routes[i].destId.data(), DEV_ID_LENGTH);
            std::string nextHopId = RadioMeshUtils::convertToHex(routes[i].nextHopId.data(), DEV_ID_LENGTH);
            loginfo_ln("Route %d: Dest=%s NextHop=%s Hops=%d RSSI=%d Age=%lums",
                i,
                destId.c_str(),
                nextHopId.c_str(),
                routes[i].hops,
                routes[i].rssi,
                millis() - routes[i].lastSeen);
        }
    }
}
