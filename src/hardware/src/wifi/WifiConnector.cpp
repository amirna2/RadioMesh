#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <hardware/inc/wifi/WifiConnector.h>

#ifndef RM_NO_WIFI
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif

WifiConnector* WifiConnector::instance = nullptr;

#ifdef RM_NO_WIFI
// IWifiConnector interface
int WifiConnector::connect(const std::string ssid, const std::string password)
{
   return RM_E_NOT_SUPPORTED;
}
int WifiConnector::disconnect(bool wifiOff)
{
   return RM_E_NOT_SUPPORTED;
}
int WifiConnector::reconnect()
{
   return RM_E_NOT_SUPPORTED;
}
std::string WifiConnector::getIpAddress()
{
   return "";
}
std::string WifiConnector::getMacAddress()
{
   return "";
}
SignalIndicator WifiConnector::getSignalIndicator()
{
   return NO_SIGNAL;
}
int WifiConnector::getSignalStrength()
{
   return 0;
}
std::string WifiConnector::getSSID()
{
   return "";
}
std::vector<std::string> WifiConnector::getAvailableNetworks()
{
   return std::vector<std::string>();
}

// WifiConnector specific
int WifiConnector::setParams(const WifiParams& params)
{
   return RM_E_NOT_SUPPORTED;
}

#else
int WifiConnector::setParams(const WifiParams& params)
{
   if (params.ssid.empty() || params.password.empty()) {
      logerr_ln("ERROR: SSID or password not provided");
      return RM_E_INVALID_WIFI_PARAMS;
   }
   if (params.ssid.size() > 32) {
      logerr_ln("ERROR: SSID too long");
      return RM_E_INVALID_WIFI_PARAMS;
   }
   if (params.password.size() > 64) {
      logerr_ln("ERROR: Password too long");
      return RM_E_INVALID_WIFI_PARAMS;
   }

   this->params.password = params.password;
   this->params.ssid = params.ssid;

   return RM_E_NONE;
}

std::vector<std::string> WifiConnector::getAvailableNetworks()
{
   std::vector<std::string> networks;
   int16_t n = WiFi.scanNetworks(false, false, false, AP_SCAN_INTERVAL_MS);

   if (n < 0) {
      logerr_ln("ERROR: scanning for WiFi networks: rc=%d", n);
      return networks;
   }

   if (n != 0) {
      logdbg_ln("Networks found: %d", n);
      for (int i = 0; i < n; ++i) {
         networks.push_back(WiFi.SSID(i).c_str());
         delay(AP_SCAN_INTERVAL_MS);
      }
   } else {
      loginfo_ln("No networks found");
   }
   return networks;
}

bool WifiConnector::ssidAvailable(std::string ssidToCheck)
{
   int16_t n = WiFi.scanNetworks(false, false, false, AP_SCAN_INTERVAL_MS);

   if (n < 0) {
      logerr_ln("Error scanning for WiFi networks: rc=%d", n);
      return false;
   }

   if (n != 0) {
      logdbg_ln("Networks found: %d", n);
      for (int i = 0; i < n; ++i) {
         if (WiFi.SSID(i) == ssidToCheck.c_str()) {
            logdbg_ln("Given ssid is available!");
            return true;
         }
         delay(AP_SCAN_INTERVAL_MS);
      }
   }
   loginfo_ln("Given ssid is not available");

   return false;
}

void WifiConnector::updateConnectionDetails(const std::string ssid, const std::string password)
{
   if (connected) {
      params.ssid = ssid;
      params.password = password;
      ipAddress = WiFi.localIP().toString().c_str();
      macAddress = WiFi.macAddress().c_str();
   }
}

void WifiConnector::resetConnectionDetails()
{
   ipAddress = "";
   macAddress = "";
   signalIndicator = NO_SIGNAL;
   params.ssid = "";
   params.password = "";
}

int WifiConnector::connect(const std::string ssid, const std::string password)
{
   const uint32_t WIFI_CONNECTION_TIMEOUT_MS = 3000;
   int rc = RM_E_NONE;
   std::string _ssid;
   std::string _password;

   if (ssid.empty() || password.empty()) {
      if (params.ssid.empty() || params.password.empty()) {
         logerr_ln("ERROR: SSID or password not provided");
         return RM_E_INVALID_PARAM;
      } else {
         _ssid = params.ssid;
         _password = params.password;
      }
   } else {
      _ssid = ssid;
      _password = password;
   }

   if (_ssid.size() > 32) {
      logerr_ln("ERROR: SSID too long");
      return RM_E_INVALID_PARAM;
   }
   if (_password.size() > 64) {
      logerr_ln("ERROR: Password too long");
      return RM_E_INVALID_PARAM;
   }
   if (_ssid == params.ssid && connected) {
      loginfo_ln("Already connected to %s", _ssid.c_str());
      return RM_E_NONE;
   }
   if (!ssidAvailable(_ssid)) {
      logerr_ln("SSID %s not available", _ssid.c_str());
      return RM_E_WIFI_SSID_NOT_AVAILABLE;
   }

   //  Connect to Access Point
   loginfo_ln("Connecting to WiFi access point SSID: %s", _ssid.c_str());
   WiFi.begin(_ssid.c_str(), _password.c_str(), false);

   // We need to wait here for the connection to estanlish. Otherwise the WiFi.status() may return a
   // false negative
   loginfo_ln("Waiting for connect results for %s", _ssid.c_str());
   WiFi.waitForConnectResult(WIFI_CONNECTION_TIMEOUT_MS);

   if (WiFi.status() == WL_CONNECTED) {
      loginfo_ln("Device connected to internet!");
      rc = RM_E_NONE;
   } else {
      logerr_ln("ERROR: failed to connect to %s (status: %d)", _ssid.c_str(), WiFi.status());
      rc = RM_E_WIFI_CONNECTION_FAILED;
   };

   if (rc == RM_E_NONE) {
      connected = true;
      updateConnectionDetails(_ssid, _password);
   }
   return rc;
}

int WifiConnector::reconnect()
{
   return connect(params.ssid, params.password);
}

SignalIndicator WifiConnector::getSignalIndicator()
{
   if (!connected) {
      signalIndicator = NO_SIGNAL;
      return signalIndicator;
   }

   int8_t rssi = WiFi.RSSI();

   // Signal strength indicator. Adjust the values as needed.
   if (rssi > -60) {
      signalIndicator = EXCELLENT;
   } else if (rssi > -70) {
      signalIndicator = GOOD;
   } else if (rssi > -80) {
      signalIndicator = FAIR;
   } else if (rssi > -90) {
      signalIndicator = WEAK;
   } else {
      signalIndicator = NO_SIGNAL;
   }

   return signalIndicator;
}

int WifiConnector::disconnect(bool wifiOff)
{
   bool result = WiFi.disconnect(wifiOff);
   int rc = RM_E_NONE;
   if (result) {
      loginfo_ln("Disconnected from WiFi network");
      rc = RM_E_NONE;
      connected = false;
      resetConnectionDetails();
   } else {
      logerr_ln("Failed to disconnect from WiFi network");
      rc = RM_E_WIFI_DISCONNECT_FAILED;
   }
   return rc;
}

std::string WifiConnector::getIpAddress()
{
   return ipAddress;
}

std::string WifiConnector::getMacAddress()
{
   return macAddress;
}

std::string WifiConnector::getSSID()
{
   return WiFi.SSID().c_str();
}

int WifiConnector::getSignalStrength()
{
   return WiFi.RSSI();
}
#endif // RM_NO_WIFI
