#pragma once

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <framework/interfaces/ICaptivePortal.h>

class AsyncCaptivePortal : public ICaptivePortal
{
public:
   static AsyncCaptivePortal* getInstance()
   {
      if (!instance) {
         instance = new AsyncCaptivePortal();
      }
      return instance;
   }

   int setup() override;
   int start() override;
   int stop() override;
   int sendToClients(const std::string& type, const std::vector<byte>& data) override;

   int setDataCallback(PortalDataCallback callback) override;
   bool isRunning() override;
   size_t getClientCount() override;

   int setParams(const CaptivePortalParams& params);

private:
   AsyncCaptivePortal() = default;
   static AsyncCaptivePortal* instance;

   std::unique_ptr<DNSServer> dnsServer;
   std::unique_ptr<AsyncWebServer> webServer;
   std::unique_ptr<AsyncWebSocket> webSocket;
   CaptivePortalParams portalParams;
   PortalDataCallback dataCallback;
   bool running = false;

   void handleWebSocketEvent(AwsEventType type, AsyncWebSocketClient* client, uint8_t* data,
                             size_t len);
   std::string injectWebSocketCode(const std::string& html);
};