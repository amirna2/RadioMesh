#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <hardware/inc/wifi/WifiAccessPoint.h>

#ifndef RM_NO_WIFI
#include <WiFi.h>
#endif

WifiAccessPoint* WifiAccessPoint::instance = nullptr;

#ifdef RM_NO_WIFI
int WifiAccessPoint::setup(WifiAccessPointParams& params)
{
    return RM_E_NOT_SUPPORTED;
}
int WifiAccessPoint::setup()
{
    return RM_E_NOT_SUPPORTED;
}
int WifiAccessPoint::start()
{
    return RM_E_NOT_SUPPORTED;
}
int WifiAccessPoint::stop(bool wifiOff)
{
    return RM_E_NOT_SUPPORTED;
}

int setParams(const WifiAccessPointParams& params)
{
    return RM_E_NOT_SUPPORTED;
}
bool isStarted()
{
    return false;
}

#else

int WifiAccessPoint::setParams(const WifiAccessPointParams& params)
{
    if (params.ssid.empty() || params.password.empty() || params.ipAddress.empty()) {
        logerr_ln("ERROR: SSID or password not provided");
        return RM_E_INVALID_AP_PARAMS;
    }

    if (params.ssid.length() > 64) {
        logerr_ln("ERROR: SSID too long");
        return RM_E_INVALID_AP_PARAMS;
    }

    if (params.password.length() < 8) {
        logerr_ln("ERROR: passphrase must be at least 8 characters");
        return RM_E_INVALID_AP_PARAMS;
    }

    IPAddress ip;
    bool success = ip.fromString(params.ipAddress.c_str());
    if (!success) {
        logerr_ln("ERROR: parsing IP address");
        return RM_E_INVALID_AP_PARAMS;
    }

    this->params.ssid = params.ssid;
    this->params.password = params.password;
    this->params.ipAddress = params.ipAddress;

    return RM_E_NONE;
}

bool WifiAccessPoint::isStarted()
{
    return started;
}

int WifiAccessPoint::setup(WifiAccessPointParams& params)
{
    if (isStarted()) {
        logerr_ln("ERROR: Wifi access point already started. Stop it first.");
        return RM_E_WIFI_AP_ALREADY_STARTED;
    }

    if (params.ssid.empty() || params.password.empty() || params.ipAddress == "0.0.0.0") {
        logerr_ln("ERROR: AccessPoint parameters not provided.");
        return RM_E_INVALID_PARAM;
    }

    if (params.ssid.length() > 64) {
        logerr_ln("ERROR: SSID too long");
        return RM_E_INVALID_PARAM;
    }

    if (params.password.length() < 8) {
        logerr_ln("ERROR: passphrase must be at least 8 characters");
        return RM_E_INVALID_PARAM;
    }

    IPAddress ip;
    bool success = ip.fromString(params.ipAddress.c_str());
    if (!success) {
        logerr_ln("ERROR: parsing IP address");
        return RM_E_INVALID_AP_PARAMS;
    }

    this->params.ipAddress = params.ipAddress;
    this->params.ssid = params.ssid;
    this->params.password = params.password;

    return RM_E_NONE;
}

int WifiAccessPoint::setup()
{
    return setup(params);
}

int WifiAccessPoint::start()
{
    IPAddress ip;
    bool success = false;

    if (isStarted()) {
        logerr_ln("ERROR: Wifi access point already started. Stop it first.");
        return RM_E_WIFI_AP_ALREADY_STARTED;
    }

    success = ip.fromString(params.ipAddress.c_str());
    if (!success) {
        logerr_ln("ERROR: parsing IP address");
        return RM_E_WIFI_AP_START_FAILED;
    }

    success = WiFi.mode(WIFI_AP_STA);
    if (!success) {
        logerr_ln("ERROR: setting WiFi mode to AP");
        return RM_E_WIFI_AP_START_FAILED;
    }

    success = WiFi.softAP(params.ssid.c_str(), params.password.c_str());
    if (!success) {
        logerr_ln("ERROR: setting WiFi AP");
        return RM_E_WIFI_AP_START_FAILED;
    }

    // TODO: need to find out why there is a delay here
    delay(200);
    success = WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
    if (!success) {
        logerr_ln("ERROR: setting WiFi AP config");
        return RM_E_WIFI_AP_START_FAILED;
    }

    started = true;
    return RM_E_NONE;
}

int WifiAccessPoint::stop(bool wifiOff)
{
    if (!started) {
        logdbg_ln("Wifi access point not started. Nothing to stop.");
        return RM_E_NONE;
    }

    if (!WiFi.softAPdisconnect(wifiOff)) {
        logerr_ln("ERROR: stopping WiFi AP");
        return RM_E_WIFI_AP_STOP_FAILED;
    }
    started = false;
    params.ssid = "";
    params.password = "";
    params.ipAddress = "0.0.0.0";

    return RM_E_NONE;
}

#endif // RM_NO_WIFI
