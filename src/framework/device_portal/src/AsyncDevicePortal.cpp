#include <common/inc/Definitions.h>
#include <framework/device_portal/inc/AsyncDevicePortal.h>

AsyncDevicePortal* AsyncDevicePortal::instance = nullptr;

#ifdef RM_NO_WIFI
int AsyncDevicePortal::setParams(const DevicePortalParams& params)
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncDevicePortal::start()
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncDevicePortal::stop()
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncDevicePortal::sendToClients(const PortalMessage& message)
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncDevicePortal::sendToClient(uint32_t clientId, const PortalMessage& message)
{
    return RM_E_NOT_SUPPORTED;
}

bool AsyncDevicePortal::isRunning()
{
    return false;
}

size_t AsyncDevicePortal::getClientCount()
{
    return 0;
}
#else
int AsyncDevicePortal::setParams(const DevicePortalParams& params)
{
    // Check for valid parameters
    if (params.webPort == 0 || params.dnsPort == 0 || params.indexHtml.empty()) {
        logerr_ln("Invalid parameters");
        return RM_E_INVALID_PARAM;
    }

    portalParams = params;
    return RM_E_NONE;
}

int AsyncDevicePortal::start()
{
    if (isRunning()) {
        return RM_E_NONE;
    }

    // Initialize servers
    dnsServer.reset(new DNSServer());
    webServer.reset(new AsyncWebServer(portalParams.webPort));
    webSocket.reset(new AsyncWebSocket("/ws"));

    // Setup WebSocket handler
    webSocket->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                              AwsEventType type, void* arg, uint8_t* data,
                              size_t len) { handleWebSocketEvent(type, client, data, len); });
    // Add WebSocket handler
    webServer->addHandler(webSocket.get());

    // Serve main page
    webServer->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        logdbg_ln("Serving main page");
        std::string html = injectWebSocketCode(portalParams.indexHtml);
        logdbg_ln("Portal params HTML length: %d", portalParams.indexHtml.length());
        logtrace_ln("Portal params HTML content: %s", portalParams.indexHtml.c_str());

        request->send(200, "text/html", html.c_str());
    });

    // Handle device portal detection
    webServer->onNotFound([](AsyncWebServerRequest* request) { request->redirect("/"); });

    // Start servers
    dnsServer->start(portalParams.dnsPort, "*", WiFi.softAPIP());
    webServer->begin();

    running = true;
    loginfo_ln("Device portal started");
    return RM_E_NONE;
}

int AsyncDevicePortal::stop()
{
    if (!isRunning()) {
        return RM_E_INVALID_STATE;
    }

    webServer->end();
    dnsServer->stop();
    webSocket->closeAll();

    webServer.reset();
    dnsServer.reset();
    webSocket.reset();

    running = false;
    loginfo_ln("Device portal stopped");
    return RM_E_NONE;
}

int AsyncDevicePortal::sendToClient(uint32_t clientId, const PortalMessage& message)
{
    if (!isRunning() || !webSocket) {
        return RM_E_INVALID_STATE;
    }

    if (clientInfo.find(clientId) == clientInfo.end()) {
        logerr_ln("Client #%u not found", clientId);
        return RM_E_INVALID_PARAM;
    }

    std::string msg =
        "{\"type\":\"" + message.getType() + "\",\"data\":\"" + message.serialize() + "\"}";
    webSocket->textAll(msg.c_str());

    if (!webSocket->text(clientId, msg.c_str())) {
        logerr_ln("Failed to send message to client #%u", clientId);
        return RM_E_UNKNOWN; // TODO: Better error code
    }

    return RM_E_NONE;
}

int AsyncDevicePortal::sendToClients(const PortalMessage& message)
{
    if (!isRunning() || !webSocket)
        return RM_E_INVALID_STATE;

    std::string msg =
        "{\"type\":\"" + message.getType() + "\",\"data\":\"" + message.serialize() + "\"}";
    webSocket->textAll(msg.c_str());
    return RM_E_NONE;
}

bool AsyncDevicePortal::isRunning()
{
    return running;
}

size_t AsyncDevicePortal::getClientCount()
{
    return webSocket ? webSocket->count() : 0;
}

void AsyncDevicePortal::handleClientMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len)
{
    if (!client || !client->canSend()) {
        logerr_ln("Invalid client or client cannot send");
        return;
    }

    if (!data || len == 0) {
        logerr_ln("Invalid data");
        return;
    }

    // TODO: This  currently assumes data is a JSON string
    // But we should handle binary data as well

    // Convert data to string safely
    std::string msg;
    msg.reserve(len);
    msg.assign((char*)data, len);

    size_t typeStart = msg.find("\"type\":\"");
    if (typeStart == std::string::npos)
        return;

    size_t typeEnd = msg.find("\"", typeStart + 8);
    if (typeEnd == std::string::npos)
        return;

    size_t dataStart = msg.find("\"data\":\"");
    if (dataStart == std::string::npos)
        return;

    size_t dataEnd = msg.find("\"}", dataStart + 8);
    if (dataEnd == std::string::npos)
        return;

    std::string eventType = msg.substr(typeStart + 8, typeEnd - (typeStart + 8));
    std::string dataStr = msg.substr(dataStart + 8, dataEnd - (dataStart + 8));

    std::vector<byte> dataVec(dataStr.begin(), dataStr.end());

    for (const auto& handler : portalParams.eventHandlers) {
        if (handler.event == eventType) {
            handler.callback(static_cast<void*>(client), dataVec);
            break;
        }
    }
}

void AsyncDevicePortal::handleWebSocketEvent(AwsEventType type, AsyncWebSocketClient* client,
                                             uint8_t* data, size_t len)
{
    if (!client)
        return;

    uint32_t clientId = client->id();

    switch (type) {
    case WS_EVT_CONNECT:
        clientInfo[clientId] = {.id = clientId};
        client->keepAlivePeriod(20); // Enable built-in keep alive
        loginfo_ln("Client #%u connected", clientId);
        break;

    case WS_EVT_DISCONNECT:
        clientInfo.erase(clientId);
        loginfo_ln("Client #%u disconnected", clientId);
        break;

    case WS_EVT_DATA:
        handleClientMessage(client, data, len);
        break;
    case WS_EVT_ERROR:
        logerr_ln("Client #%u websocket error", clientId);
        break;
    case WS_EVT_PONG:
        loginfo_ln("Client #%u pong received", clientId);
        if (clientInfo.find(clientId) != clientInfo.end()) {
            clientInfo[clientId].lastPong = millis();
        }
        break;
    default:
        break;
    }
}

std::string AsyncDevicePortal::injectWebSocketCode(const std::string& html)
{
    char formattedBuffer[4096];
    std::string wsCode = R"(
    <script>
        let wsRetryCount = 0;
        const MAX_RETRIES = 3;
        let reconnectTimeout = null;

        function connectWebSocket() {
            if (reconnectTimeout) {
                clearTimeout(reconnectTimeout);
                reconnectTimeout = null;
            }

            const ws = new WebSocket('ws://' + window.location.hostname + ':%d/ws');

            ws.onopen = () => window.dispatchEvent(new CustomEvent('WebSocket.open'));
            ws.onclose = (event) => {
                window.dispatchEvent(new CustomEvent('WebSocket.close'));
                if (event.code !== 1000 && wsRetryCount < MAX_RETRIES) {
                    wsRetryCount++;
                    reconnectTimeout = setTimeout(connectWebSocket, 2000 * wsRetryCount);
                }
            };
            ws.onerror = () => window.dispatchEvent(new CustomEvent('WebSocket.error'));
            ws.onmessage = (event) => {
                try {
                    const msg = JSON.parse(event.data);
                    if (msg && msg.type && msg.data) {
                        window.dispatchEvent(new CustomEvent(msg.type, {detail: msg.data}));
                    }
                } catch(e) {
                    console.error('Invalid message format:', e);
                }
            };
            window.devicePortalWs = ws;
        }

        window.addEventListener('load', connectWebSocket);
    </script>)";

    snprintf(formattedBuffer, sizeof(formattedBuffer), wsCode.c_str(), portalParams.webPort);

    size_t pos = html.find("</body>");
    if (pos != std::string::npos) {
        return html.substr(0, pos) + formattedBuffer + html.substr(pos);
    }
    return html + wsCode;
}
#endif // RM_NO_WIFI
