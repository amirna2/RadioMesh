#pragma once

#include <string>
#include <vector>
#include <common/inc/Definitions.h>

/**
 * @class IWifiAccessPoint
 * @brief This class is an interface for a WiFi access point.
*/
class IWifiAccessPoint
{
public:
   virtual ~IWifiAccessPoint() {}

   /**
    * @brief Set up the WiFi access point with the given parameters.
    * @param params The parameters of the WiFi access point.
   */
   virtual int setup(WifiAccessPointParams& params) = 0;

   /**
    * @brief Setup the WiFi access point with the currently stored parameters.
    *
    * @returns RM_E_NONE if the WiFi access point was successfully setup, an error code otherwise.
    */
   virtual int setup() = 0;

   /**
    * @brief Start the WiFi access point.
    * @returns RM_E_NONE if the WiFi access point was successfully started, an error code otherwise.
   */
   virtual int start() = 0;

   /**
    * @brief Stop the WiFi access point.
    * @param wifiOff If true, turn off the WiFi radio.
    * @returns RM_E_NONE if the WiFi access point was successfully stopped, an error code otherwise.
   */
   virtual int stop(bool wifiOff = false) = 0;


};
