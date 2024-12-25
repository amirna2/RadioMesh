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
     * @brief Send data to connected portal clients
     * @param type Message type identifier
     * @param data Binary data to send
     * @return RM_E_NONE if successful, error code otherwise
     */
    virtual int sendToClients(const std::string& type, const std::vector<byte>& data) = 0;

    /**
     * @brief Check if portal is running
     * @return true if running, false otherwise
     */
    virtual int sendToClients(const std::string& type, const std::string& data) = 0;

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
