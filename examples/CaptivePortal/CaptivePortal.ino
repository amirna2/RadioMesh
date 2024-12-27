#include "device_info.h"
#include "portal_html.h"
#include <Arduino.h>
#include <RadioMesh.h>

void RxCallback(const RadioMeshPacket* packet, int err);
bool setupAccessPoint();
void handleWifiConfig(void* client, const std::vector<byte>&);
void handleProvision(void* client, const std::vector<byte>&);
void handleFirmware(void* client, const std::vector<byte>&);
void handleMessage(void* client, const std::vector<byte>&);

IDevice* device = nullptr;
IWifiAccessPoint* wifiAP = nullptr;
bool setupOk = false;

// Simple message handler
void handleMessage(void* client, const std::vector<byte>& data)
{
    loginfo_ln("Handling Message!");
    // device->getCaptivePortal()->sendToClients("status", data);
    auto wsClient = static_cast<AsyncWebSocketClient*>(client);
    std::string msg(data.begin(), data.end());
    device->getCaptivePortal()->sendToClient(wsClient->id(), "status", msg);
}

// Handler for WiFi configuration
void handleWifiConfig(void* client, const std::vector<byte>& data)
{
    // Parse JSON and update WiFi settings
    // Store in flash/EEPROM
    loginfo_ln("Handling WiFi config!");
    device->getCaptivePortal()->sendToClients("status", "WiFi configuration updated");
}

// Handler for device provisioning
void handleProvision(void* client, const std::vector<byte>& data)
{
    // Parse JSON and provision device
    // Update mesh network settings
    loginfo_ln("Handling device provision!");
    device->getCaptivePortal()->sendToClients("status", "Device provisioned");
}

// Handler for firmware update
void handleFirmware(void* client, const std::vector<byte>& data)
{
    // Handle firmware update
    // Verify and flash new firmware
    loginfo_ln("Handling firmware update!");
    device->getCaptivePortal()->sendToClients("status", "Firmware updated");
}

// Portal params with constructor
CaptivePortalParams portalParams{
    "RadioMesh Portal", SIMPLE_PORTAL_HTML, 80, 53, {PortalEventHandler{"message", handleMessage}}};

/*
CaptivePortalParams portalParams{"RadioMesh Portal",
                                 PORTAL_HTML,
                                 80,
                                 53,
                                 {{"wifi_config", handleWifiConfig},
                                  {"provision", handleProvision},
                                  {"firmware", handleFirmware}}};
*/

void RxCallback(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Error: %d", err);
        return;
    }

    // Forward mesh packet data to portal
    if (device && device->getCaptivePortal()) {
        device->getCaptivePortal()->sendToClients("status", packet->packetData);
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
    auto portal = device->getCaptivePortal();
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
                 .withCaptivePortal(portalParams)
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
