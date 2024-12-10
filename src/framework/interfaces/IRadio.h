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
 * It currently provides methods to setup a LoRa radio.
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
};
