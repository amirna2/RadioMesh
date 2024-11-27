#pragma once

#include <vector>
#include <string>

#include <common/inc/Definitions.h>
#include <framework/interfaces/IWifiAccessPoint.h>

class WifiAccessPoint : public IWifiAccessPoint
{
public:

   /**
    * @brief Get the instance of the WifiAccessPoint.
    * @returns A pointer to the instance of the WifiAccessPoint.
   */
   static WifiAccessPoint* getInstance() {
   #ifdef RM_NO_WIFI
      return nullptr;
   #endif
      if (!instance) {
         instance = new WifiAccessPoint();
      }
      return instance;
   }

   virtual ~WifiAccessPoint() {}

   // IWifiAccessPoint interface
   int setup(WifiAccessPointParams& params) override;
   int setup() override;
   int start() override;
   int stop(bool wifiOff) override;

   // WifiAccessPoint specific
   /**
    * @brief Set the parameters of the WiFi access point.
    * @param params The parameters of the WiFi access point.
    * @returns RM_E_NONE if the parameters were successfully set, an error code otherwise.
   */
   int setParams(const WifiAccessPointParams& params);

   /**
    * @brief Get the access point status
    * @returns true if the access point is started, false otherwise.
   */
   bool isStarted();

private:
   WifiAccessPoint() {};
   WifiAccessPoint(WifiAccessPoint const&) = delete;
   WifiAccessPoint& operator=(WifiAccessPoint const&) = delete;

   static WifiAccessPoint* instance;

   WifiAccessPointParams params = {"","", {0,0,0,0}};
   bool started = false;

};
