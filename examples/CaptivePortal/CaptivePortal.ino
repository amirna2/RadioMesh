#include "device_info.h"
#include <Arduino.h>
#include <RadioMesh.h>

void RxCallback(const RadioMeshPacket* packet, int err);
bool setupAccessPoint();
void handleWifiConfig(void* client, const std::string& data);
void handleProvision(void* client, const std::string& data);
void handleFirmware(void* client, const std::string& data);

IDevice* device = nullptr;
IWifiAccessPoint* wifiAP = nullptr;
bool setupOk = false;

// Portal page with minimal interface
const char* PORTAL_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>RadioMesh Hub Setup</title>
    <style>
        body { font-family: Arial; margin: 20px; max-width: 800px; margin: 0 auto; }
        .tab { overflow: hidden; border: 1px solid #ccc; background-color:rgb(70, 75, 59); }
        .tab button { background-color: inherit; float: left; border: none; padding: 14px 16px; cursor: pointer; }
        .tab button:hover { background-color: #ddd; }
        .tab button.active { background-color: #ccc; }
        .tabcontent { display: none; padding: 20px; border: 1px solid #ccc; border-top: none; }
        .form-group { margin: 15px 0; }
        label { display: block; margin-bottom: 5px; }
        input { width: 100%; padding: 8px; margin-bottom: 10px; }
        button { background: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }
        .status { margin: 10px 0; padding: 10px; background: #f0f0f0; }
    </style>
</head>
<body>
    <h2>RadioMesh Hub Setup</h2>
    <div class="tab">
        <button class="tablinks" onclick="openTab(event, 'Wifi')">WiFi Setup</button>
        <button class="tablinks" onclick="openTab(event, 'Provision')">Device Provisioning</button>
        <button class="tablinks" onclick="openTab(event, 'Firmware')">Firmware Update</button>
    </div>

    <div id="Wifi" class="tabcontent">
        <h3>WiFi Configuration</h3>
        <form id="wifiForm">
            <div class="form-group">
                <label>SSID:</label>
                <input type="text" id="wifi_ssid" required>
            </div>
            <div class="form-group">
                <label>Password:</label>
                <input type="password" id="wifi_password" required>
            </div>
            <button type="submit">Save WiFi Config</button>
        </form>
    </div>

    <div id="Provision" class="tabcontent">
        <h3>Device Provisioning</h3>
        <form id="provisionForm">
            <div class="form-group">
                <label>Device ID:</label>
                <input type="text" id="device_id" required>
            </div>
            <div class="form-group">
                <label>Network Key:</label>
                <input type="password" id="network_key" required>
            </div>
            <button type="submit">Provision Device</button>
        </form>
    </div>

    <div id="Firmware" class="tabcontent">
        <h3>Firmware Update</h3>
        <form id="firmwareForm">
            <div class="form-group">
                <label>Firmware File:</label>
                <input type="file" id="firmware_file" accept=".bin">
            </div>
            <button type="submit">Upload Firmware</button>
        </form>
        <div id="upload_progress"></div>
    </div>

    <div id="status" class="status">Ready...</div>

    <script>
        function openTab(evt, tabName) {
            var tabcontent = document.getElementsByClassName("tabcontent");
            for (var i = 0; i < tabcontent.length; i++) {
                tabcontent[i].style.display = "none";
            }
            var tablinks = document.getElementsByClassName("tablinks");
            for (var i = 0; i < tablinks.length; i++) {
                tablinks[i].className = tablinks[i].className.replace(" active", "");
            }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }

        document.getElementById("wifiForm").onsubmit = function(e) {
            e.preventDefault();
            ws.send(JSON.stringify({
                type: "wifi_config",
                data: JSON.stringify({
                    ssid: document.getElementById("wifi_ssid").value,
                    password: document.getElementById("wifi_password").value
                })
            }));
        };

        document.getElementById("provisionForm").onsubmit = function(e) {
            e.preventDefault();
            ws.send(JSON.stringify({
                type: "provision",
                data: JSON.stringify({
                    device_id: document.getElementById("device_id").value,
                    network_key: document.getElementById("network_key").value
                })
            }));
        };

        document.getElementById("firmwareForm").onsubmit = function(e) {
            e.preventDefault();
            const file = document.getElementById("firmware_file").files[0];
            const reader = new FileReader();
            reader.onload = function() {
                ws.send(JSON.stringify({
                    type: "firmware",
                    data: reader.result
                }));
            };
            reader.readAsDataURL(file);
        };

        // Handle status updates
        window.addEventListener('status', function(e) {
            document.getElementById('status').textContent = e.detail;
        });
    </script>
</body>
</html>
)=====";

// Create handlers for each function
void handleWifiConfig(void* client, const std::string& data)
{
   // Parse JSON and update WiFi settings
   // Store in flash/EEPROM
   loginfo_ln("Handling WiFi config!");
   device->getCaptivePortal()->sendToClients("status", "WiFi configuration updated");
}

void handleProvision(void* client, const std::string& data)
{
   // Parse JSON and provision device
   // Update mesh network settings
   loginfo_ln("Handling device provision!");
   device->getCaptivePortal()->sendToClients("status", "Device provisioned");
}

void handleFirmware(void* client, const std::string& data)
{
   // Handle firmware update
   // Verify and flash new firmware
   loginfo_ln("Handling firmware update!");
   device->getCaptivePortal()->sendToClients("status", "Firmware updated");
}

// Portal params with constructor
CaptivePortalParams portalParams{"RadioMesh Portal",
                                 PORTAL_HTML,
                                 80,
                                 53,
                                 {{"wifi_config", handleWifiConfig},
                                  {"provision", handleProvision},
                                  {"firmware", handleFirmware}}};

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
