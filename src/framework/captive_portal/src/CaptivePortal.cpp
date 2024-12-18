#include <framework/captive_portal/inc/CaptivePortal.h>

AsyncCaptivePortal* AsyncCaptivePortal::instance = nullptr;

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

int AsyncCaptivePortal::setup()
{
   if (isRunning()) {
      logerr_ln("Portal already running");
      return RM_E_INVALID_STATE;
   }

   portalParams = CaptivePortalParams();
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
      std::string html = injectWebSocketCode(portalParams.indexHtml);
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

int AsyncCaptivePortal::setDataCallback(PortalDataCallback callback)
{
   dataCallback = callback;
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

void AsyncCaptivePortal::handleWebSocketEvent(AwsEventType type, AsyncWebSocketClient* client,
                                              uint8_t* data, size_t len)
{
   switch (type) {
   case WS_EVT_CONNECT:
      loginfo_ln("Client #%u connected", client->id());
      break;

   case WS_EVT_DISCONNECT:
      loginfo_ln("Client #%u disconnected", client->id());
      break;

   case WS_EVT_DATA:
      if (dataCallback && len > 0) {
         // Parse JSON message
         std::string msg(reinterpret_cast<char*>(data), len);
         // Basic JSON parsing -  might need to use a proper JSON library
         size_t typeStart = msg.find("\"type\":\"") + 8;
         size_t typeEnd = msg.find("\"", typeStart);
         size_t dataStart = msg.find("\"data\":\"") + 8;
         size_t dataEnd = msg.find("\"}", dataStart);

         if (typeStart != std::string::npos && dataStart != std::string::npos) {
            std::string type = msg.substr(typeStart, typeEnd - typeStart);
            std::string dataStr = msg.substr(dataStart, dataEnd - dataStart);
            std::vector<byte> dataVec(dataStr.begin(), dataStr.end());

            dataCallback(type, dataVec);
         }
      }
      break;
   }
}

std::string AsyncCaptivePortal::injectWebSocketCode(const std::string& html)
{
   std::string wsCode = "<script>var ws = new WebSocket('ws://' + window.location.hostname + ':";
   wsCode += std::to_string(portalParams.webPort);
   wsCode += "/ws');";
   wsCode += "ws.onmessage = function(event) { var msg = JSON.parse(event.data);";
   wsCode += "if (msg.type && msg.data) { window.dispatchEvent(new CustomEvent(msg.type, { detail: "
             "msg.data })); } };";
   wsCode += "</script>";

   size_t pos = html.find("</body>");
   if (pos != std::string::npos) {
      return html.substr(0, pos) + wsCode + html.substr(pos);
   }

   return html + wsCode;
}
