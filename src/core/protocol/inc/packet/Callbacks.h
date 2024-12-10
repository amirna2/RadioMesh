#pragma once

#include "Packet.h"

/**
 * @typedef PacketReceivedCallback
 * @brief A callback function for handling received packets.
 * @param packet Pointer to the received packet (maybe nullptr if an error occurred).
 * @param int The error code of the reception.
 */
typedef void (*PacketReceivedCallback)(const RadioMeshPacket*, int);

/**
 * @typedef PacketSentCallback
 * @brief A callback function for handling transmitted packets.
 * @param packet Pointer to the transmitted packet.
 * @param int The error code of the transmission.
 */
typedef void (*PacketSentCallback)(const RadioMeshPacket*, int);
