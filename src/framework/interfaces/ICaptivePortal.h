#pragma once

#include <common/inc/Definitions.h>
#include <functional>
#include <string>
#include <vector>

using PortalDataCallback =
    std::function<void(const std::string& type, const std::vector<byte>& data)>;

/**
 * @class ICaptivePortal
 * @brief Interface for captive portal functionality
 */
class ICaptivePortal
{
public:
   virtual ~ICaptivePortal() = default;

   /**
    * @brief Set up the portal with configuration parameters
    * @return RM_E_NONE if successful, error code otherwise
    */
   virtual int setup() = 0;

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
    * @brief Register callback for data received from portal clients
    * @param callback Function to handle received data
    * @return RM_E_NONE if successful, error code otherwise
    */
   virtual int setDataCallback(PortalDataCallback callback) = 0;

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
