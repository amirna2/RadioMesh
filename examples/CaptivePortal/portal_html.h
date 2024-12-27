#pragma once

const char* SIMPLE_PORTAL_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>WebSocket Test</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; }
        #status { padding: 10px; margin: 10px 0; }
        #messages { border: 1px solid #ddd; padding: 10px; height: 200px; overflow-y: auto; }
        input { margin: 10px 0; padding: 5px; }
        .connected { color: green; }
        .disconnected { color: red; }
        .error { color: orangered; }
    </style>
</head>
<body>
    <div id="status" class="disconnected">Disconnected</div>
    <div id="messages"></div>
    <input type="text" id="input" placeholder="Type message...">
    <button onclick="sendMessage()">Send</button>

    <script>
        const status = document.getElementById('status');
        const messages = document.getElementById('messages');
        const input = document.getElementById('input');

        let ws = null;
        let wsRetryCount = 0;
        const MAX_RETRIES = 3;
        const PING_INTERVAL = 30000; // 30 seconds
        let pingInterval = null;

        function updateStatus(text, className) {
            status.textContent = text;
            status.className = className;
        }

        function connectWebSocket() {
            if (ws) {
                clearPingInterval();
                ws.close();
            }

            ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            window.captivePortalWs = ws;

            ws.onopen = () => {
                wsRetryCount = 0;
                setupPingInterval();
                updateStatus('Connected', 'connected');
            };

            ws.onclose = (event) => {
                clearPingInterval();
                updateStatus('Disconnected', 'disconnected');

                if (event.code !== 1000 && wsRetryCount < MAX_RETRIES) {
                    wsRetryCount++;
                    setTimeout(connectWebSocket, 2000 * wsRetryCount);
                }
            };

            ws.onerror = () => {
                clearPingInterval();
                updateStatus('Connection Error', 'error');
            };

            ws.onmessage = (event) => {
                try {
                    const msg = JSON.parse(event.data);
                    if (msg && msg.type && msg.data) {
                        const msgDiv = document.createElement('div');
                        msgDiv.textContent = `Received: ${msg.data}`;
                        messages.appendChild(msgDiv);
                        messages.scrollTop = messages.scrollHeight;
                    }
                } catch(e) {
                    console.error('Invalid message format:', e);
                }
            };
        }

        function setupPingInterval() {
            clearPingInterval();
            pingInterval = setInterval(() => {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({type: 'ping', data: ''}));
                }
            }, PING_INTERVAL);
        }

        function clearPingInterval() {
            if (pingInterval) {
                clearInterval(pingInterval);
                pingInterval = null;
            }
        }

        function sendMessage() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                const message = input.value.trim();
                if (message) {
                    ws.send(JSON.stringify({type: 'message', data: message}));
                    const msgDiv = document.createElement('div');
                    msgDiv.textContent = `You: ${message}`;
                    messages.appendChild(msgDiv);
                    messages.scrollTop = messages.scrollHeight;
                    input.value = '';
                }
            }
        }

        input.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') sendMessage();
        });

        // Clean up on page unload
        window.addEventListener('beforeunload', () => {
            clearPingInterval();
            if (ws) {
                ws.close(1000, "Page closing");
            }
        });

        // Initialize connection
        connectWebSocket();
    </script>
</body>
</html>
)=====";

const char* PORTAL_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>RadioMesh Hub Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        /* Base styles */
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            max-width: 800px;
            margin: 0 auto;
            background-color: #f5f5f5;
        }

        h2, h3 {
            color: #333;
            border-bottom: 2px solid #eee;
            padding-bottom: 10px;
        }

        /* Tab styles */
        .tab {
            overflow: hidden;
            border: 1px solid #ccc;
            background-color: #344955;
            border-radius: 4px 4px 0 0;
        }

        .tab button {
            background-color: inherit;
            float: left;
            border: none;
            outline: none;
            cursor: pointer;
            padding: 14px 16px;
            transition: 0.3s;
            font-size: 16px;
            color: #fff;
        }

        .tab button:hover {
            background-color: #4a6572;
        }

        .tab button.active {
            background-color: #232f34;
        }

        /* Content area styles */
        .tabcontent {
            display: none;
            padding: 20px;
            border: 1px solid #ccc;
            border-top: none;
            background-color: white;
            border-radius: 0 0 4px 4px;
            animation: fadeEffect 0.5s;
        }

        @keyframes fadeEffect {
            from {opacity: 0;}
            to {opacity: 1;}
        }

        /* Form styles */
        .form-group {
            margin: 15px 0;
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: bold;
        }

        input {
            width: 100%;
            padding: 8px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }

        input[type="file"] {
            border: none;
            padding: 0;
        }

        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            transition: background-color 0.3s;
        }

        button:hover {
            background-color: #45a049;
        }

        button:disabled {
            background-color: #cccccc;
            cursor: not-allowed;
        }

        /* Status indicator styles */
        .status {
            margin: 20px 0;
            padding: 15px;
            border-radius: 4px;
            background: #f8f9fa;
            border-left: 4px solid #4CAF50;
        }

        .status.error {
            background: #ffebee;
            color: #c62828;
            border-left-color: #c62828;
        }

        .status.success {
            background: #e8f5e9;
            color: #2e7d32;
            border-left-color: #2e7d32;
        }

        .status.warning {
            background: #fff3e0;
            color: #ef6c00;
            border-left-color: #ef6c00;
        }

        /* Connection indicator */
        #connection-indicator {
            position: fixed;
            top: 10px;
            right: 10px;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            z-index: 1000;
            transition: all 0.3s ease;
        }

        .connected {
            background-color: #4CAF50;
            color: white;
        }

        .disconnected {
            background-color: #f44336;
            color: white;
        }

        .connecting {
            background-color: #ff9800;
            color: white;
        }

        /* Progress bar */
        .progress-bar {
            width: 100%;
            height: 20px;
            background-color: #f0f0f0;
            border-radius: 10px;
            overflow: hidden;
            margin: 10px 0;
        }

        .progress-bar-fill {
            height: 100%;
            background-color: #4CAF50;
            width: 0%;
            transition: width 0.3s ease;
        }
    </style>
</head>
<body>
    <div id="connection-indicator"></div>
    <h2>RadioMesh Hub Setup</h2>

    <div class="tab">
        <button class="tablinks" onclick="openTab(event, 'Wifi')">WiFi Setup</button>
        <button class="tablinks" onclick="openTab(event, 'Provision')">Device Provisioning</button>
        <button class="tablinks" onclick="openTab(event, 'Firmware')">Firmware Update</button>
    </div>

    <div id="Wifi" class="tabcontent">
        <h3>WiFi Configuration</h3>
        <form id="wifiForm" onsubmit="return handleWifiSubmit(event)">
            <div class="form-group">
                <label for="wifi_ssid">SSID:</label>
                <input type="text" id="wifi_ssid" required>
            </div>
            <div class="form-group">
                <label for="wifi_password">Password:</label>
                <input type="password" id="wifi_password" required>
            </div>
            <button type="submit" id="wifiSubmitBtn">Save WiFi Config</button>
        </form>
    </div>

    <div id="Provision" class="tabcontent">
        <h3>Device Provisioning</h3>
        <form id="provisionForm" onsubmit="return handleProvisionSubmit(event)">
            <div class="form-group">
                <label for="device_id">Device ID:</label>
                <input type="text" id="device_id" required pattern="[0-9a-fA-F]{8}"
                       title="Please enter an 8-character hex ID">
            </div>
            <div class="form-group">
                <label for="network_key">Network Key:</label>
                <input type="password" id="network_key" required minlength="16"
                       title="Network key must be at least 16 characters">
            </div>
            <button type="submit" id="provisionSubmitBtn">Provision Device</button>
        </form>
    </div>

    <div id="Firmware" class="tabcontent">
        <h3>Firmware Update</h3>
        <form id="firmwareForm" onsubmit="return handleFirmwareSubmit(event)">
            <div class="form-group">
                <label for="firmware_file">Firmware File:</label>
                <input type="file" id="firmware_file" accept=".bin" required>
            </div>
            <div class="progress-bar" style="display: none;">
                <div class="progress-bar-fill"></div>
            </div>
            <button type="submit" id="firmwareSubmitBtn">Upload Firmware</button>
        </form>
    </div>

    <div id="status" class="status">Ready to connect...</div>

    <script>
        // WebSocket Connection Management
        let ws = null;
        let reconnectAttempts = 0;
        const MAX_RECONNECT_ATTEMPTS = 3;
        const INITIAL_RECONNECT_DELAY = 1000;
        const MAX_RECONNECT_DELAY = 16000;
        let reconnectTimeout = null;

        function updateConnectionStatus(status, message) {
            const indicator = document.getElementById('connection-indicator');
            indicator.className = status;
            indicator.textContent = message;
        }

        function updateStatus(message, isError = false, isWarning = false) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + (isError ? 'error' : (isWarning ? 'warning' : 'success'));
        }

        function connectWebSocket() {
            if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
                updateConnectionStatus('disconnected', 'Connection Failed');
                updateStatus('Maximum reconnection attempts reached. Please reload the page.', true);
                return;
            }

            if (ws) {
                ws.close();
            }

            updateConnectionStatus('connecting', 'Connecting...');

            // Use the current window location to build WebSocket URL
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.hostname}:${window.location.port}/ws`;

            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                console.log('WebSocket connected');
                reconnectAttempts = 0;
                updateConnectionStatus('connected', 'Connected');
                updateStatus('Connected to device');
                enableForms(true);
            };

            ws.onclose = (event) => {
                console.log('WebSocket disconnected', event.code, event.reason);
                updateConnectionStatus('disconnected', 'Disconnected');
                updateStatus('Connection lost. Attempting to reconnect...', false, true);
                enableForms(false);

                const delay = Math.min(INITIAL_RECONNECT_DELAY * Math.pow(2, reconnectAttempts), MAX_RECONNECT_DELAY);
                reconnectAttempts++;

                if (reconnectTimeout) {
                    clearTimeout(reconnectTimeout);
                }
                reconnectTimeout = setTimeout(connectWebSocket, delay);
            };

            ws.onerror = (error) => {
                console.error('WebSocket error:', error);
                updateStatus('Connection error occurred', true);
            };

            ws.onmessage = (event) => {
                handleWebSocketMessage(event.data);
            };
        }

        function enableForms(enabled) {
            document.querySelectorAll('form button[type="submit"]').forEach(button => {
                button.disabled = !enabled;
            });
        }

        function handleWebSocketMessage(data) {
            try {
                const message = JSON.parse(data);
                if (message.type === 'status') {
                    updateStatus(message.data);
                }
                // Handle other message types as needed
            } catch (e) {
                console.error('Error parsing message:', e);
                updateStatus('Error processing server message', true);
            }
        }

        function sendMessage(type, data) {
            if (!ws || ws.readyState !== WebSocket.OPEN) {
                updateStatus('Not connected to server', true);
                return false;
            }

            try {
                ws.send(JSON.stringify({ type, data }));
                return true;
            } catch (e) {
                console.error('Send error:', e);
                updateStatus('Error sending message', true);
                return false;
            }
        }

        // Form Handlers
        function handleWifiSubmit(event) {
            event.preventDefault();
            const data = {
                ssid: document.getElementById('wifi_ssid').value,
                password: document.getElementById('wifi_password').value
            };
            if (sendMessage('wifi_config', data)) {
                updateStatus('WiFi configuration sent');
            }
            return false;
        }

        function handleProvisionSubmit(event) {
            event.preventDefault();
            const data = {
                device_id: document.getElementById('device_id').value,
                network_key: document.getElementById('network_key').value
            };
            if (sendMessage('provision', data)) {
                updateStatus('Provisioning request sent');
            }
            return false;
        }

        function handleFirmwareSubmit(event) {
            event.preventDefault();
            const fileInput = document.getElementById('firmware_file');
            const file = fileInput.files[0];
            if (!file) {
                updateStatus('Please select a firmware file', true);
                return false;
            }

            const reader = new FileReader();
            const progressBar = document.querySelector('.progress-bar');
            const progressFill = document.querySelector('.progress-bar-fill');

            reader.onload = function(e) {
                const data = {
                    filename: file.name,
                    content: e.target.result
                };
                if (sendMessage('firmware', data)) {
                    updateStatus('Firmware upload started');
                }
            };

            reader.onprogress = function(e) {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    progressBar.style.display = 'block';
                    progressFill.style.width = percentComplete + '%';
                }
            };

            reader.onerror = function() {
                updateStatus('Error reading firmware file', true);
            };

            reader.readAsDataURL(file);
            return false;
        }

        // Tab Management
        function openTab(evt, tabName) {
            const tabcontent = document.getElementsByClassName("tabcontent");
            for (let i = 0; i < tabcontent.length; i++) {
                tabcontent[i].style.display = "none";
            }

            const tablinks = document.getElementsByClassName("tablinks");
            for (let i = 0; i < tablinks.length; i++) {
                tablinks[i].className = tablinks[i].className.replace(" active", "");
            }

            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }

        // Page Visibility Handler
        document.addEventListener('visibilitychange', function() {
            if (document.hidden) {
                if (ws) {
                    ws.close(1000, "Page hidden");
                }
                if (reconnectTimeout) {
                    clearTimeout(reconnectTimeout);
                }
            } else if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                connectWebSocket();
            }
        });

        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            // Open first tab by default
            document.querySelector('.tablinks').click();
            // Connect WebSocket
            connectWebSocket();
        });

        // Handle page unload
        window.addEventListener('beforeunload', function() {
            if (ws) {
                ws.close(1000, "Page closing");
            }
        });
    </script>
</body>
</html>
)=====";
