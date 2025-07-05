#pragma once
#include <Arduino.h>
#include <RadioMesh.h>

extern IDevice* device;

// Chat message implementation
class ChatMessage : public PortalMessage
{
    std::string message;

public:
    ChatMessage(const std::vector<byte>& data)
    {
        // Convert incoming bytes to string
        message = std::string(data.begin(), data.end());
    }
    std::string getType() const override
    {
        return "chat_message";
    }
    std::string serialize() const override
    {
        return message; // Raw JSON from client preserved
    }
};

void handleSendMessage(void* client, const std::vector<byte>& data)
{
    if (!device) {
        logerr_ln("Device is null");
        return;
    }

    auto msg = ChatMessage(data);
    device->getDevicePortal()->sendToClients(msg);
}

class StatusMessage : public PortalMessage
{
    std::string msg;

public:
    StatusMessage(const std::string& message) : msg(message)
    {
    }

    std::string getType() const override
    {
        return "status"; // Used by client-side to know this is a status message
    }

    std::string serialize() const override
    {
        return msg;
    }
};

void handleJoinEvent(void* client, const std::vector<byte>& data)
{
    if (!device) {
        logerr_ln("Device is null");
        return;
    }

    // Parse the incoming join message
    std::string jsonStr(data.begin(), data.end());
    loginfo_ln("Join event: %s", jsonStr.c_str());
    auto msg = StatusMessage(jsonStr);
    device->getDevicePortal()->sendToClients(msg);
}

void handleLeaveEvent(void* client, const std::vector<byte>& data)
{
    if (!device) {
        logerr_ln("Device is null");
        return;
    }

    // Parse the incoming join message
    std::string jsonStr(data.begin(), data.end());
    loginfo_ln("Leave event: %s", jsonStr.c_str());
    auto msg = StatusMessage(jsonStr);
    device->getDevicePortal()->sendToClients(msg);
}

const char* CHAT_PORTAL_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Mesh Chat</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            margin: 20px;
            background: #f5f5f5;
        }
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        #loginView, #chatView { display: none; }
        .active { display: block !important; }

        .chat-container {
            display: flex;
            gap: 20px;
            height: 600px;
        }

        /* Main chat area */
        .chat-main {
            flex: 1;
            display: flex;
            flex-direction: column;
        }

        #status {
            background: #000;
            color: #0f0;
            padding: 8px 12px;
            margin-bottom: 10px;
            font-family: monospace;
            width: 100%;
            box-sizing: border-box;
        }

        #messages {
            flex: 1;
            border: 1px solid #ddd;
            padding: 15px;
            overflow-y: auto;
            margin-bottom: 10px;
        }

        .input-area {
            display: flex;
            gap: 10px;
        }

        #msgInput {
            flex: 1;
            padding: 8px 12px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 14px;
        }
        #users {
            width: 200px;
            border: 1px solid #ddd;
            padding: 15px;
            border-radius: 4px;
        }
        .msg {
            margin: 8px 0;
            line-height: 1.4;
        }
        .sender {
            font-weight: 500;
            color: #2962ff;
        }

        button {
            padding: 8px 16px;
            background: #f5f5f5;
            border: 1px solid #ddd;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
        }
        button:hover {
            background: #e0e0e0;
        }

        #loginView {
            max-width: 400px;
            margin: 100px auto;
            text-align: center;
        }
        #loginView input {
            width: 100%;
            padding: 8px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div id="loginView">
            <h2>Enter Chat Room</h2>
            <input type="text" id="username" placeholder="Choose nickname">
            <button onclick="joinChat()">Join</button>
        </div>

        <div id="chatView">
            <div class="chat-container">
                <div class="chat-main">
                    <div id="status"></div>
                    <div id="messages"></div>
                    <div class="input-area">
                        <input type="text" id="msgInput" placeholder="Type message..."
                               onkeyup="if(event.key==='Enter')sendMessage(this)">
                        <button onclick="leaveChat()">Leave Room</button>
                    </div>
                </div>
                <div id="users"></div>
            </div>
        </div>
    </div>

    <script>
        let ws = null;
        let username = '';
        let activeUsers = new Set();
        const status = document.getElementById('status');
        const messages = document.getElementById('messages');
        const users = document.getElementById('users');

        function showView(id) {
            document.querySelectorAll('#loginView, #chatView').forEach(el => el.classList.remove('active'));
            document.getElementById(id).classList.add('active');
        }

        function updateDeviceList() {
            users.innerHTML = '<h3>Online Users</h3>' +
                Array.from(activeUsers)
                    .map(user => `<div class="device">${user}</div>`)
                    .join('');
        }

        function joinChat() {
            username = document.getElementById('username').value.trim();
            if (!username) return;
            if (!status) {
                console.error('Status element not found');
                return;
            }
            ws = new WebSocket('ws://' + location.hostname + '/ws');

            ws.onopen = () => {
                showView('chatView');
                status.innerHTML = `Welcome ${username}!`;
                ws.send(JSON.stringify({
                    type: 'join',
                    data: `${username} has joined the chat`,
                    from: username
                }));
            };

            ws.onmessage = e => {
                let msg = JSON.parse(e.data);
                if (msg.type === 'join') {
                    activeUsers.add(msg.from);
                    status.innerHTML = msg.data;
                    updateDeviceList();
                }
                else if (msg.type === 'leave') {
                    activeUsers.delete(msg.from);
                    status.innerHTML = msg.data;
                    updateDeviceList();
                }
                else if (msg.type === 'chat_message') {
                    messages.innerHTML += `
                        <div class="msg">
                            <span class="sender">${msg.from}:</span>
                            <span class="content">${msg.data}</span>
                        </div>`;
                    messages.scrollTop = messages.scrollHeight;
                }
            };

            ws.onclose = () => {
                activeUsers.clear();
                updateDeviceList();
            };
        }

        function leaveChat() {
            if (ws) {
                ws.send(JSON.stringify({
                    type: 'leave',
                    data: `${username} has left the chat`,
                    from: username
                }));
                ws.close();
            }
            activeUsers.clear();
            showView('loginView');
        }

        function sendMessage(input) {
            if (!input.value) return;
            ws.send(JSON.stringify({
                type: 'chat_message',
                data: input.value,
                from: username
            }));
            input.value = '';
        }

        showView('loginView');
    </script>
</body>
</html>
)=====";

DevicePortalParams portalParams{
    "RadioMesh Chat",
    CHAT_PORTAL_HTML,
    80,
    53,
    {{"chat_message", handleSendMessage}, {"join", handleJoinEvent}, {"leave", handleLeaveEvent}}};
