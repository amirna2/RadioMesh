#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <RadioMesh.h>
#include <map>
#include <vector>

extern IDevice* device;
extern bool inclusionModeActive;

// Function declarations from MiniHub.ino
void webStartInclusionMode();
void webStopInclusionMode();

// Device info structure
struct DeviceInfo
{
    std::string id;
    std::string name;
    unsigned long lastSeen;
    int rssi;
};

extern std::map<std::string, DeviceInfo> connectedDevicesMap;

// Status message for WebSocket
class StatusMessage : public PortalMessage
{
    std::string jsonData;

public:
    StatusMessage(const std::string& data) : jsonData(data)
    {
    }

    std::string getType() const override
    {
        return "status_update";
    }

    std::string serialize() const override
    {
        // Escape the JSON string for embedding in the data field
        std::string escaped;
        for (char c : jsonData) {
            if (c == '"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else escaped += c;
        }
        return escaped;
    }
};

// Device list message
class DeviceListMessage : public PortalMessage
{
    std::string jsonData;

public:
    DeviceListMessage(const std::string& data) : jsonData(data)
    {
    }

    std::string getType() const override
    {
        return "device_list";
    }

    std::string serialize() const override
    {
        return jsonData;
    }
};

// Log entry message
class LogMessage : public PortalMessage
{
    std::string jsonData;

public:
    LogMessage(const std::string& data) : jsonData(data)
    {
    }

    std::string getType() const override
    {
        return "log_entry";
    }

    std::string serialize() const override
    {
        return jsonData;
    }
};

// Inclusion event message
class InclusionEventMessage : public PortalMessage
{
    std::string jsonData;

public:
    InclusionEventMessage(const std::string& data) : jsonData(data)
    {
    }

    std::string getType() const override
    {
        return "inclusion_event";
    }

    std::string serialize() const override
    {
        return jsonData;
    }
};

// Handler: Get current status
void handleGetStatus(void* client, const std::vector<byte>& data)
{
    if (!device)
        return;

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();

    // Get hub ID
    auto hubId = device->getDeviceId();
    char hubIdStr[16];
    snprintf(hubIdStr, sizeof(hubIdStr), "%02X%02X%02X%02X", hubId[0], hubId[1], hubId[2],
             hubId[3]);

    root["hubId"] = hubIdStr;
    // Check if inclusion mode is actually enabled on the device
    bool inclusionMode = device->isInclusionModeEnabled();
    root["inclusionMode"] = inclusionMode;
    root["deviceCount"] = connectedDevicesMap.size();

    if (inclusionMode) {
        root["inclusionTimeRemaining"] = 0;
    }

    std::string jsonStr;
    serializeJson(doc, jsonStr);

    auto msg = StatusMessage(jsonStr);
    device->getCaptivePortal()->sendToClients(msg);
}

// Handler: Set inclusion mode
void handleSetInclusionMode(void* client, const std::vector<byte>& data)
{
    if (!device)
        return;

    // Convert to string like chat does
    std::string command(data.begin(), data.end());

    bool enable = (command == "enable");

    // Call proper functions that update app state correctly
    if (enable) {
        webStartInclusionMode();
    } else {
        webStopInclusionMode();
    }

    // Send updated status
    handleGetStatus(client, data);
}

// Handler: Get device list
void handleGetDevices(void* client, const std::vector<byte>& data)
{
    if (!device)
        return;

    JsonDocument doc;
    JsonArray devices = doc["devices"].to<JsonArray>();

    for (const auto& pair : connectedDevicesMap) {
        JsonObject dev = devices.add<JsonObject>();
        dev["id"] = pair.second.id;
        dev["name"] = pair.second.name;
        dev["lastSeen"] = pair.second.lastSeen;
        dev["rssi"] = pair.second.rssi;
    }

    std::string jsonStr;
    serializeJson(doc, jsonStr);

    auto msg = DeviceListMessage(jsonStr);
    device->getCaptivePortal()->sendToClients(msg);
}

// Helper function to send inclusion events
void sendInclusionEvent(const std::string& event, const std::string& deviceId)
{
    if (!device || !device->getCaptivePortal())
        return;

    JsonDocument doc;
    doc["event"] = event;
    doc["deviceId"] = deviceId;
    doc["timestamp"] = millis();

    std::string jsonStr;
    serializeJson(doc, jsonStr);

    auto msg = InclusionEventMessage(jsonStr);
    device->getCaptivePortal()->sendToClients(msg);
}

// HTML for the admin panel
const char* ADMIN_PANEL_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>MiniHub Admin Panel</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
            margin: 0 0 10px 0;
            color: #333;
        }
        .hub-id {
            color: #666;
            font-size: 14px;
            margin-bottom: 30px;
        }
        .section {
            margin-bottom: 30px;
            padding: 20px;
            background: #f9f9f9;
            border-radius: 4px;
        }
        .section h2 {
            margin: 0 0 15px 0;
            font-size: 18px;
            color: #444;
        }
        .inclusion-control {
            display: flex;
            align-items: center;
            gap: 20px;
        }
        .toggle-button {
            padding: 10px 20px;
            font-size: 16px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        .toggle-button.enable {
            background: #4CAF50;
            color: white;
        }
        .toggle-button.enable:hover {
            background: #45a049;
        }
        .toggle-button.disable {
            background: #f44336;
            color: white;
        }
        .toggle-button.disable:hover {
            background: #da190b;
        }
        .status-indicator {
            padding: 5px 10px;
            border-radius: 4px;
            font-size: 14px;
            font-weight: bold;
        }
        .status-active {
            background: #c8e6c9;
            color: #2e7d32;
        }
        .status-inactive {
            background: #e0e0e0;
            color: #757575;
        }
        .countdown {
            font-size: 14px;
            color: #666;
            margin-left: 10px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
        }
        th {
            background: #e0e0e0;
            padding: 10px;
            text-align: left;
            font-weight: 600;
        }
        td {
            padding: 10px;
            border-bottom: 1px solid #e0e0e0;
        }
        .device-id {
            font-family: monospace;
            font-size: 14px;
        }
        .rssi {
            font-weight: 500;
        }
        .rssi.good { color: #4CAF50; }
        .rssi.fair { color: #FF9800; }
        .rssi.poor { color: #f44336; }
        .activity-log {
            max-height: 200px;
            overflow-y: auto;
            background: white;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 10px;
            font-family: monospace;
            font-size: 12px;
        }
        .log-entry {
            margin: 2px 0;
            padding: 2px 0;
        }
        .log-time {
            color: #666;
            margin-right: 10px;
        }
        .log-info { color: #2196F3; }
        .log-success { color: #4CAF50; }
        .log-error { color: #f44336; }
        .no-devices {
            text-align: center;
            color: #999;
            padding: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>MiniHub Admin Panel</h1>
        <div class="hub-id">Hub ID: <span id="hubId">Loading...</span></div>

        <div class="section">
            <h2>Inclusion Control</h2>
            <div class="inclusion-control">
                <button id="toggleInclusion" class="toggle-button enable" onclick="toggleInclusionMode()">
                    Enable Inclusion Mode
                </button>
                <span id="inclusionStatus" class="status-indicator status-inactive">Inactive</span>
                <span id="countdown" class="countdown" style="display: none;"></span>
            </div>
        </div>

        <div class="section">
            <h2>Connected Devices</h2>
            <div id="deviceTableContainer">
                <table id="deviceTable" style="display: none;">
                    <thead>
                        <tr>
                            <th>Device ID</th>
                            <th>Name</th>
                            <th>Last Seen</th>
                            <th>Signal (RSSI)</th>
                        </tr>
                    </thead>
                    <tbody id="deviceList"></tbody>
                </table>
                <div id="noDevices" class="no-devices">No devices connected</div>
            </div>
        </div>

        <div class="section">
            <h2>Activity Log</h2>
            <div id="activityLog" class="activity-log"></div>
        </div>
    </div>

    <script>
        let ws = null;
        let inclusionActive = false;
        let countdownInterval = null;
        let devices = new Map();

        // Format time for log entries
        function formatTime(timestamp) {
            const date = new Date(timestamp);
            return date.toLocaleTimeString();
        }

        // Add log entry
        function addLogEntry(message, type = 'info') {
            const log = document.getElementById('activityLog');
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            entry.innerHTML = `<span class="log-time">${formatTime(Date.now())}</span><span class="log-${type}">${message}</span>`;
            log.appendChild(entry);
            log.scrollTop = log.scrollHeight;
        }

        // Update device table
        function updateDeviceTable() {
            const table = document.getElementById('deviceTable');
            const noDevices = document.getElementById('noDevices');
            const tbody = document.getElementById('deviceList');

            if (devices.size === 0) {
                table.style.display = 'none';
                noDevices.style.display = 'block';
            } else {
                table.style.display = 'table';
                noDevices.style.display = 'none';

                tbody.innerHTML = '';
                devices.forEach((device, id) => {
                    const row = tbody.insertRow();
                    row.innerHTML = `
                        <td class="device-id">${id}</td>
                        <td>${device.name || 'Unknown'}</td>
                        <td>${formatTime(device.lastSeen)}</td>
                        <td class="rssi ${getRssiClass(device.rssi)}">${device.rssi} dBm</td>
                    `;
                });
            }
        }

        // Get RSSI signal strength class
        function getRssiClass(rssi) {
            if (rssi > -60) return 'good';
            if (rssi > -80) return 'fair';
            return 'poor';
        }

        // Toggle inclusion mode
        function toggleInclusionMode() {
            console.log('Toggling inclusion mode to', !inclusionActive);
            if (ws && ws.readyState === WebSocket.OPEN) {
                console.log('Sending inclusion mode toggle with data:', !inclusionActive);
                ws.send(JSON.stringify({
                    type: 'set_inclusion_mode',
                    data: !inclusionActive ? 'enable' : 'disable'
                }));
            }
        }

        // Update inclusion status UI
        function updateInclusionStatus(active, timeRemaining = 0) {
            inclusionActive = active;
            const button = document.getElementById('toggleInclusion');
            const status = document.getElementById('inclusionStatus');
            const countdown = document.getElementById('countdown');

            if (active) {
                button.textContent = 'Disable Inclusion Mode';
                button.className = 'toggle-button disable';
                status.textContent = 'Active';
                status.className = 'status-indicator status-active';

                countdown.style.display = 'none';
            } else {
                button.textContent = 'Enable Inclusion Mode';
                button.className = 'toggle-button enable';
                status.textContent = 'Inactive';
                status.className = 'status-indicator status-inactive';
                countdown.style.display = 'none';

                if (countdownInterval) {
                    clearInterval(countdownInterval);
                    countdownInterval = null;
                }
            }
        }

        // Connect WebSocket like the working chat example
        function connectWebSocket() {
            ws = new WebSocket('ws://' + location.hostname + '/ws');

            ws.onopen = () => {
                addLogEntry('Connected to hub', 'success');
                // Request initial status
                ws.send(JSON.stringify({ type: 'get_status', data: {} }));
                ws.send(JSON.stringify({ type: 'get_devices', data: 'request' }));
            };

            ws.onmessage = (event) => {
                const msg = JSON.parse(event.data);

                switch(msg.type) {
                    case 'status_update':
                        const statusData = JSON.parse(msg.data);
                        updateInclusionStatus(statusData.inclusionMode, statusData.inclusionTimeRemaining);
                        document.getElementById('hubId').textContent = statusData.hubId || 'Unknown';
                        break;

                    case 'device_list':
                        const deviceData = JSON.parse(msg.data);
                        devices.clear();
                        deviceData.devices.forEach(device => {
                            devices.set(device.id, device);
                        });
                        updateDeviceTable();
                        break;

                    case 'inclusion_event':
                        const eventData = JSON.parse(msg.data);
                        addLogEntry(`Inclusion: ${eventData.event} from device ${eventData.deviceId}`,
                                   eventData.event.includes('success') ? 'success' : 'info');
                        break;

                    case 'device_added':
                        const addedData = JSON.parse(msg.data);
                        devices.set(addedData.id, addedData);
                        updateDeviceTable();
                        addLogEntry(`Device ${addedData.id} added to network`, 'success');
                        break;

                    case 'log_entry':
                        const logData = JSON.parse(msg.data);
                        addLogEntry(logData.message, logData.level);
                        break;
                }
            };

            ws.onclose = () => {
                addLogEntry('Disconnected from hub', 'error');
                updateInclusionStatus(false);
                setTimeout(connectWebSocket, 2000);
            };

            ws.onerror = () => {
                addLogEntry('Connection error', 'error');
            };
        }

        // Initialize
        connectWebSocket();
        addLogEntry('Admin panel initialized', 'info');
    </script>
</body>
</html>
)=====";

// Configure captive portal with admin panel
CaptivePortalParams portalParams{"MiniHub Admin",
                                 ADMIN_PANEL_HTML,
                                 80,
                                 53,
                                 {{"get_status", handleGetStatus},
                                  {"set_inclusion_mode", handleSetInclusionMode},
                                  {"get_devices", handleGetDevices}}};
