#pragma once

#include <string>
#include <vector>
#include <array>
#include <common/inc/Options.h>
#include <framework/interfaces/IRadio.h>
#include <framework/interfaces/IAesCrypto.h>
#include <framework/interfaces/IDisplay.h>
#include <framework/interfaces/IWifiConnector.h>
#include <framework/interfaces/IWifiAccessPoint.h>


/**
 * @class IDevice
 * @brief This class is an interface for a device.
 *
 * It provides the structure for getting various components of the device, sending data, enabling relay,
 * servicing the interrupt flags and more.
 * Use this inteterface to create specialized devices part of the RadioMesh network.
 */
class IDevice
{
public:
   virtual ~IDevice() {}
   /**
    * @brief Get the radio object interface associated with the device.
    * @return IRadio* pointer to the radio object implementing the interface.
   */
   virtual IRadio *getRadio() = 0;

   /**
    * @brief Get the crypto object interface associated with the device.
    * @return IAesCrypto* pointer to the crypto object implementing the interface.
   */
   virtual IAesCrypto *getCrypto() = 0;

   /**
    * @brief Get the display object interface associated with the device.
    * @return IDisplay* pointer to the display object implementing the interface.
   */
   virtual IDisplay *getDisplay() = 0;

   /**
    * @brief Get the wifi connector object interface associated with the device.
    * @return IWifiConnector* pointer to the wifi connector object implementing the interface.
   */
   virtual IWifiConnector *getWifiConnector() = 0;

   /**
    * @brief Get the wifi access point object interface associated with the device.
    * @return IWifiAccessPoint* pointer to the wifi access point object implementing the interface.
   */
   virtual IWifiAccessPoint *getWifiAccessPoint() = 0;

   /**
    * @brief Send data to the mesh network.
    *
    * A device can send data only if it is included in the network or it is sending one of the inclusion messages:
    *
    * - ReservedTopic::INCLUSION_OPEN,
    * - ReservedTopic::INCLUSION_REQUEST
    * - ReservedTopic::INCLUSION_RESPONSE
    * - ReservedTopic::INCLUSION_CONFIRM
    *
    * @param topic Topic to send the data to
    * @param data Data to send
    * @param target Target device to send the data to (default is broadcast to all devices)
    * @return RM_E_NONE if the data was sent successfully, an error code otherwise.
   */
   virtual int sendData(const uint8_t topic, const std::vector<byte> data, std::array<byte, RM_ID_LENGTH> target = BROADCAST_ADDR) = 0;

   /**
    * @brief Allow the device to relay packets.
    * @param enabled true to enable the relay, false to disable it.
   */
   virtual void enableRelay(bool enabled) = 0;

   /**
    * @brief Check if the device is allowed to relay packets.
    * @return true if the device is allowed to relay packets, false otherwise.
   */
   virtual bool isRelayEnabled() = 0;

   /**
    * @brief Get the device name.
    * @return std::string containing the device name.
   */
   virtual std::string getDeviceName() = 0;

   /**
    * @brief Get the device ID.
    * @return An array containing the device ID.
   */
   virtual std::array<byte, RM_ID_LENGTH> getDeviceId() = 0;

   /**
    * @brief Service the interrupt flags of the radio and handles any received data.
    * @return RM_E_NONE if the device ran successfully, an error code otherwise.
   */
   virtual int run() = 0;

   /**
    * @brief Set the device type.
    * @param type DeviceType enum value representing the device type.
   */
   virtual void setDeviceType(MeshDeviceType type) = 0;

   /**
    * @brief Enable or disable inclusion mode.
    *
    * HUB only feature. When enabled, the hub will accept new devices to join the network.
    * @param enable true to enable inclusion mode, false to disable it.
    * @return RM_E_NONE if the inclusion mode was successfully enabled or disabled, an error code otherwise.
    */
   virtual int enableInclusionMode(bool enable) = 0;

   /**
    * @brief Send inclusion open broadcast message.
    *
    * Hub only feature. Sends an empty broadcast message to notify devices that hub accepts new devices
    * Hub must be in inclusion mode.
    *
    * @return RM_E_NONE on success
    * @return RM_E_INVALID_STATE if not in inclusion mode
    * @return RM_E_INVALID_DEVICE_TYPE if not a hub
    */
   virtual int sendInclusionOpen() = 0;

   /**
    * @brief Send inclusion request message
    *
    * Device must not be included to send this request.
    * Message contains:
    * - Device ID (already in packet header)
    * - Device's public key
    * - Initial message counter value
    *
    * @param publicKey Device's public key
    * @param initialCounter Initial message counter value
    * @return RM_E_NONE on success
    * @return RM_E_INVALID_STATE if device already included
    * @return RM_E_INVALID_DEVICE_TYPE if device is a HUB
    */
   virtual int sendInclusionRequest(const std::vector<byte>& publicKey, uint32_t initialCounter) = 0;

};
