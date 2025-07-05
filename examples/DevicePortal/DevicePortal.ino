#include "chat.h"
#include "device_info.h"
#include <Arduino.h>
#include <RadioMesh.h>

void RxCallback(const RadioMeshPacket* packet, int err);
bool setupAccessPoint();

IDevice* device = nullptr;
IWifiAccessPoint* wifiAP = nullptr;
bool setupOk = false;

void RxCallback(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Error: %d", err);
        return;
    }
}

bool setupAccessPoint()
{
    loginfo_ln("Setting up acces point : %s", apParams.ssid.c_str());

    wifiAP = device->getWifiAccessPoint();
    if (wifiAP == nullptr) {
        logerr_ln("ERROR: wifiAP is null");
        return false;
    }

    if (wifiAP->setup() != RM_E_NONE) {
        logerr_ln("ERROR: WifiAP setup failed");
        return false;
    }

    loginfo_ln("Starting access point : %s", apParams.ssid.c_str());

    if (wifiAP->start() != RM_E_NONE) {
        logerr_ln("ERROR: WifiAP start failed");
        return false;
    }

    // Proper portal setup and start
    auto portal = device->getDevicePortal();
    if (!portal) {
        logerr_ln("ERROR: portal is null");
        return false;
    }

    loginfo_ln("Starting portal...");

    int rc = portal->start();
    if (rc != RM_E_NONE) {
        logerr_ln("ERROR: Portal start failed: %d", rc);
        return false;
    }

    loginfo_ln("Portal started successfully");
    return true;
}

void setup()
{
    // Create device with minimal required capabilities for portal
    device = DeviceBuilder()
                 .start()
                 .withLoraRadio(radioParams)
                 .withWifiAccessPoint(apParams)
                 .withDevicePortal(portalParams)
                 .withRxPacketCallback(RxCallback)
                 .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::HUB);

    if (device == nullptr) {
        logerr_ln("ERROR: device is null");
        setupOk = false;
        return;
    }

    setupOk = setupAccessPoint();
    if (!setupOk) {
        logerr_ln("ERROR: wifiAP setup failed");
        return;
    }

    setupOk = true;
}

void loop()
{
    if (!setupOk) {
        return;
    }
    device->run();
}
