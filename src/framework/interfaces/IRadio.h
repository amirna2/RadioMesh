#pragma once

#include <string>
#include <vector>

#include <common/inc/Options.h>
#include <common/inc/RadioConfigs.h>
#include <core/protocol/inc/packet/Packet.h>

/**
 * @class IRadio
 * @brief This class is an interface for a radio.
 *
 * It provides methods to setup a LoRa radio, transmit and receive packets,
 * and query the radio state.
 */
class IRadio
{
public:
    virtual ~IRadio()
    {
    }

    /**
     * @brief Setup the radio with the given parameters.
     *
     * @param param LoraRadioParams struct containing the radio parameters
     * @returns RM_E_NONE if the radio was successfully setup, an error code otherwise.
     */
    virtual int setup(const LoraRadioParams& params) = 0;

    /**
     * @brief Setup the radio with the currently stored parameters.
     *
     * @returns RM_E_NONE if the radio was successfully setup, an error code otherwise.
     */
    virtual int setup() = 0;

    /**
     * @brief Get the current SNR value.
     *
     * @returns A float representing the snr value.
     */
    virtual float getSNR() = 0;

    /**
     * @brief Get the current RSSI value.
     *
     * @returns An integer representing the rssi value.
     */
    virtual int getRSSI() = 0;

    /**
     * @brief Set the radio into standby mode.
     *
     * @returns RM_E_NONE if the radio is successfully set in standby mode, an error code otherwise.
     */
    virtual int standBy() = 0;

    /**
     * @brief Set the radio into sleep mode.
     *
     * @returns RM_E_NONE if the radio is successfully set in sleep mode, an error code otherwise.
     */
    virtual int sleep() = 0;

    /**
     * @brief Set the radio parameters.
     *
     * @param params LoraRadioParams object containing the radio parameters
     * @returns RM_E_NONE if the parameters were successfully set, an error code otherwise.
     */
    virtual int setParams(LoraRadioParams params) = 0;

    /**
     * @brief Send a packet of data.
     *
     * @param data vector of bytes containing the data to send
     * @returns RM_E_NONE if the packet was successfully sent, an error code otherwise.
     */
    virtual int sendPacket(std::vector<byte>& data) = 0;

    /**
     * @brief Switch the radio to receive mode.
     *
     * @returns RM_E_NONE if the radio was successfully switched to receive mode, an error code
     * otherwise.
     */
    virtual int startReceive() = 0;

    /**
     * @brief Read the received data from the radio.
     *
     * @param packetData vector of bytes to store the received data
     * @returns RM_E_NONE if the data was successfully read, an error code otherwise.
     */
    virtual int readReceivedData(std::vector<byte>* packetData) = 0;

    /**
     * @brief Check if a packet was received and clear the receive flag.
     *
     * @returns true if a packet was received, false otherwise.
     */
    virtual bool checkAndClearRxFlag() = 0;

    /**
     * @brief Check if a packet was transmitted and clear the transmit flag.
     *
     * @returns true if a packet was transmitted, false otherwise.
     */
    virtual bool checkAndClearTxFlag() = 0;

    /**
     * @brief Get the radio state error.
     *
     * @returns The radio state error.
     */
    virtual int getRadioStateError() = 0;
};
