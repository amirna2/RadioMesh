#pragma once

#ifndef RM_NO_WIFI
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#endif
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <framework/interfaces/IDevicePortal.h>
#include <map>

class AsyncDevicePortal : public IDevicePortal
{
public:
    static AsyncDevicePortal* getInstance()
    {
        if (!instance) {
            instance = new AsyncDevicePortal();
        }
        return instance;
    }

    int start() override;
    int stop() override;

    int sendToClient(uint32_t clientId, const PortalMessage& message) override;
    int sendToClients(const PortalMessage& message) override;

    bool isRunning() override;
    size_t getClientCount() override;

    int setParams(const DevicePortalParams& params);

private:
    AsyncDevicePortal() = default;
    static AsyncDevicePortal* instance;
    DevicePortalParams portalParams;
    bool running = false;

#ifndef RM_NO_WIFI
    std::unique_ptr<DNSServer> dnsServer;
    std::unique_ptr<AsyncWebServer> webServer;
    std::unique_ptr<AsyncWebSocket> webSocket;
    void handleWebSocketEvent(AwsEventType type, AsyncWebSocketClient* client, uint8_t* data,
                              size_t len);
    std::string injectWebSocketCode(const std::string& html);
    void handleClientMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);

    static const uint32_t CLIENT_TIMEOUT_MS = 30000; // 30 seconds
    static const size_t MAX_CLIENTS = 4;             // Conservative limit

    struct ClientInfo
    {
        uint32_t id;
        uint32_t lastActive; // Last activity timestamp
        uint32_t lastPong;   // Last pong received timestamp
    };
    std::map<uint32_t, ClientInfo> clientInfo;

#endif
};
