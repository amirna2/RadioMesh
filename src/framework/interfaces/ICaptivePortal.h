#pragma once

#include <common/inc/Definitions.h>
#include <functional>
#include <string>
#include <vector>

/**
 * @class ICaptivePortal
 * @brief Interface for captive portal functionality
 */
class ICaptivePortal
{
public:
    virtual ~ICaptivePortal() = default;

    /**
     * @brief Start the portal services
     * @return RM_E_NONE if successful, error code otherwise
     */
    virtual int start() = 0;

    /**
     * @brief Stop the portal services
     * @return RM_E_NONE if successful, error code otherwise
     */
    virtual int stop() = 0;

    /**
     * @brief Send a message to a specific client
     * @param clientId The ID of the client to send the message to
     * @param message The message to send
     * @return RM_E_NONE if successful, error code otherwise
     */
    virtual int sendToClient(uint32_t clientId, const PortalMessage& message) = 0;

    /**
     * @brief Send a message to all connected clients
     * @param message The message to send
     * @return RM_E_NONE if successful, error code otherwise
     */
    virtual int sendToClients(const PortalMessage& message) = 0;

    /**
     * @brief Check if portal is running
     * @return true if running, false otherwise
     */
    virtual bool isRunning() = 0;

    /**
     * @brief Get number of connected clients
     * @return Number of active connections
     */
    virtual size_t getClientCount() = 0;
};
