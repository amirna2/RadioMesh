#pragma once

#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <framework/interfaces/IWifiConnector.h>

class WifiConnector : public IWifiConnector
{
public:
   /**
    * @brief Get the instance of the WifiConnector.
    * @returns A pointer to the instance of the WifiConnector.
    */
   static WifiConnector* getInstance()
   {
#ifdef RM_NO_WIFI
      return nullptr;
#endif

      if (!instance) {
         instance = new WifiConnector();
      }
      return instance;
   }

   virtual ~WifiConnector()
   {
   }

   // IWifiConnector interface

   int connect(const std::string ssid = "", const std::string password = "") override;
   int disconnect(bool wifiOff) override;
   int reconnect() override;
   std::string getIpAddress() override;
   std::string getMacAddress() override;
   SignalIndicator getSignalIndicator() override;
   int getSignalStrength() override;
   std::string getSSID() override;
   std::vector<std::string> getAvailableNetworks() override;

   // WifiConnector specific

   /**
    * @brief Set the parameters for the WiFi connection.
    * @param params The parameters to set.
    * @return int RM_E_NONE if successful, otherwise an error code.
    */
   int setParams(const WifiParams& params);

private:
   WifiConnector() {};
   WifiConnector(WifiConnector const&) = delete;
   WifiConnector& operator=(WifiConnector const&) = delete;

   static WifiConnector* instance;

   std::string ipAddress = "";
   std::string macAddress = "";
   WifiParams params = {"", ""};
   bool connected = false;
   SignalIndicator signalIndicator = NO_SIGNAL;

   bool ssidAvailable(const std::string ssid);
   void updateConnectionDetails(const std::string ssid, const std::string password);
   void resetConnectionDetails();

   static const uint32_t AP_SCAN_INTERVAL_MS = 800;
};
