#pragma once

#include <common/inc/Definitions.h>
#include <string>

/**
 * @class IWifiConnector
 * @brief This class is an interface for a WiFi connector.
 *
 * It provides the methods for the RadioMesh device to connect, disconnect, and reconnect to a WiFi
 * network,
 */
class IWifiConnector
{
public:
   virtual ~IWifiConnector()
   {
   }

   /**
    * @brief Connect to the specified WiFi network.
    * @param ssid The SSID of the network to connect to. If empty, use the stored SSID.
    * @param password The password of the network to connect to. If empty, use the stored password.
    * @returns RM_E_NONE if the connection was successful, an error code otherwise.
    */
   virtual int connect(const std::string ssid = "", const std::string password = "") = 0;

   /**
    * @brief Disconnect from the currently connected WiFi network.
    * @param wifiOff If true, turn off the WiFi radio after disconnecting.
    * @returns RM_E_NONE if the disconnection was successful, an error code otherwise.
    */
   virtual int disconnect(bool wifiOff) = 0;

   /**
    * @brief Reconnect to the last connected WiFi network.
    * @returns RM_E_NONE if the reconnection was successful, an error code otherwise.
    */
   virtual int reconnect() = 0;
   /**
    * @brief Get the IP address of the device.
    * @returns std::string containing the IP address of the device.
    */
   virtual std::string getIpAddress() = 0;

   /**
    * @brief Get the MAC address of the device.
    * @returns std::string containing the MAC address of the device.
    */

   virtual std::string getMacAddress() = 0;

   /**
    * @brief Get the signal indicator of the WiFi connection.
    * @returns SignalStrength enum value indicating the signal strength.
    */
   virtual SignalStrength getSignalIndicator() = 0;

   /**
    * @brief Get the signal strength (RRSI) of the WiFi connection.
    * @returns int containing the rssi value in dBm.
    */
   virtual int getSignalStrength() = 0;

   /**
    * @brief Get the SSID of the currently connected WiFi network.
    * @returns std::string containing the SSID of the network.
    */
   virtual std::string getSSID() = 0;

   /**
    * @brief Get a list of available WiFi networks.
    * @returns std::vector<std::string> containing the SSIDs of the available networks.
    */
   virtual std::vector<std::string> getAvailableNetworks() = 0;
};
