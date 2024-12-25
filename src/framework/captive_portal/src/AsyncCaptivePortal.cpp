#include <common/inc/Definitions.h>
#include <framework/captive_portal/inc/AsyncCaptivePortal.h>

AsyncCaptivePortal* AsyncCaptivePortal::instance = nullptr;

#ifdef RM_NO_WIFI
int AsyncCaptivePortal::setParams(const CaptivePortalParams& params)
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncCaptivePortal::start()
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncCaptivePortal::stop()
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncCaptivePortal::sendToClients(const std::string& type, const std::vector<byte>& data)
{
    return RM_E_NOT_SUPPORTED;
}

int AsyncCaptivePortal::sendToClients(const std::string& type, const std::string& data)
{
    return RM_E_NOT_SUPPORTED;
}

bool AsyncCaptivePortal::isRunning()
{
    return false;
}

size_t AsyncCaptivePortal::getClientCount()
{
    return 0;
}
#else
int AsyncCaptivePortal::setParams(const CaptivePortalParams& params)
{
    // Check for valid parameters
    if (params.webPort == 0 || params.dnsPort == 0 || params.indexHtml.empty()) {
        logerr_ln("Invalid parameters");
        return RM_E_INVALID_PARAM;
    }

    portalParams = params;
    return RM_E_NONE;
}

int AsyncCaptivePortal::start()
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

    // Handle captive portal detection
    webServer->onNotFound([](AsyncWebServerRequest* request) { request->redirect("/"); });

    // Start servers
    dnsServer->start(portalParams.dnsPort, "*", WiFi.softAPIP());
    webServer->begin();

    running = true;
    loginfo_ln("Captive portal started");
    return RM_E_NONE;
}

int AsyncCaptivePortal::stop()
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
    loginfo_ln("Captive portal stopped");
    return RM_E_NONE;
}

int AsyncCaptivePortal::sendToClients(const std::string& type, const std::vector<byte>& data)
{
    if (!isRunning() || !webSocket) {
        return RM_E_INVALID_STATE;
    }

    // Create JSON message
    std::string msg = "{\"type\":\"" + type + "\",\"data\":\"";
    msg += std::string(data.begin(), data.end());
    msg += "\"}";

    webSocket->textAll(msg.c_str());
    return RM_E_NONE;
}

int AsyncCaptivePortal::sendToClients(const std::string& type, const std::string& data)
{
    if (!isRunning() || !webSocket) {
        return RM_E_INVALID_STATE;
    }
    std::string msg = "{\"type\":\"" + type + "\",\"data\":\"" + data + "\"}";
    webSocket->textAll(msg.c_str());
    return RM_E_NONE;
}

bool AsyncCaptivePortal::isRunning()
{
    return running;
}

size_t AsyncCaptivePortal::getClientCount()
{
    return webSocket ? webSocket->count() : 0;
}

void AsyncCaptivePortal::handleClientMessage(AsyncWebSocketClient* client, uint8_t* data,
                                             size_t len)
{
    if (!client || !client->canSend() || !data) {
        return;
    }
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

    for (const auto& handler : portalParams.eventHandlers) {
        if (handler.event == eventType) {
            handler.callback(static_cast<void*>(client), dataStr);
            break;
        }
    }
}

void AsyncCaptivePortal::handleWebSocketEvent(AwsEventType type, AsyncWebSocketClient* client,
                                              uint8_t* data, size_t len)
{
    switch (type) {
    case WS_EVT_CONNECT:
        loginfo_ln("Client #%u connected", client->id());
        client->keepAlivePeriod(10);
        break;

    case WS_EVT_DISCONNECT:
        loginfo_ln("Client #%u disconnected", client->id());
        // TODO: Handle client disconnect event
        break;

    case WS_EVT_DATA:
        if (len > 0 && data != nullptr) {
            handleClientMessage(client, data, len);
        }
        break;
    case WS_EVT_PONG:
        loginfo_ln("Client #%u pong", client->id());
        break;
    case WS_EVT_ERROR:
        if (len >= 2) {
            uint16_t errorCode = (uint16_t)(data[0] << 8) + data[1];
            const char* errorMsg = (len > 2) ? (char*)(data + 2) : "";
            logerr_ln("Client #%u websocket error code=%u, msg=%s", client->id(), errorCode,
                      errorMsg);
            client->close();
        }
        break;

    default:
        break;
    }
}

std::string AsyncCaptivePortal::injectWebSocketCode(const std::string& html)
{
    std::string wsCode = R"(
      <script>
         let wsRetryCount = 0;
         const MAX_RETRIES = 3;

         function connectWebSocket() {
            if (window.captivePortalWs && window.captivePortalWs.readyState !== WebSocket.CLOSED) {
               window.captivePortalWs.close();
            }

            const ws = new WebSocket('ws://' + window.location.hostname + ':' + )" +
                         std::to_string(portalParams.webPort) + R"( + '/ws');

            ws.onopen = function() {
               console.log('WebSocket connected');
               wsRetryCount = 0;
            };

            ws.onclose = function(event) {
               console.log('WebSocket disconnected', event.code, event.reason);

               if (wsRetryCount < MAX_RETRIES) {
                  wsRetryCount++;
                  setTimeout(connectWebSocket, 2000 * wsRetryCount);
               } else {
                  console.log('Max WebSocket reconnection attempts reached');
               }
            };

            ws.onerror = function(err) {
               console.log('WebSocket error occurred');
            };

            ws.onmessage = function(event) {
               if (!event || !event.data) return;

               var msg;
               try {
                  msg = JSON.parse(event.data);
               } catch(e) {
                  console.log('Invalid message format');
                  return;
               }

               if (msg && msg.type && msg.data) {
                  window.dispatchEvent(new CustomEvent(msg.type, {
                     detail: msg.data
                  }));
               }
            };

            window.captivePortalWs = ws;
         }

         window.addEventListener('load', function() {
            connectWebSocket();
         });

         window.addEventListener('beforeunload', function() {
            if (window.captivePortalWs) {
               window.captivePortalWs.close(1000, 'Page closing');
            }
         });

         document.addEventListener('visibilitychange', function() {
            if (document.hidden) {
               if (window.captivePortalWs) {
                  window.captivePortalWs.close(1000, 'Page hidden');
               }
            } else {
               connectWebSocket();
            }
         });
      </script>
   )";

    size_t pos = html.find("</body>");
    if (pos != std::string::npos) {
        return html.substr(0, pos) + wsCode + html.substr(pos);
    }
    return html + wsCode;
}
#endif // RM_NO_WIFI
