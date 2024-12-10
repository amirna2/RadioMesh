#pragma once

#include <memory>
#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <framework/interfaces/IRadio.h>

#include <RadioLib.h>

#define RX_TX_STATE 0
#define RX_STATE 1
#define TX_STATE 2

/**
 * @class LoraRadio
 *
 * @brief This class implements the IRadio interface for a LoRa radio.
 */
class LoraRadio : public IRadio
{
public:
   /**
    * @brief Get the instance of the LoraRadio.
    * @returns A pointer to the instance of the LoraRadio.
    */
   static LoraRadio* getInstance()
   {
      if (!instance) {
         instance = new LoraRadio();
      }
      return instance;
   }

   // IRadio interface
   virtual ~LoraRadio()
   {
   }

   virtual int setup(const LoraRadioParams& params) override;
   virtual int setup() override;
   virtual int standBy() override;
   virtual int sleep() override;
   virtual int getRSSI() override;
   virtual float getSNR() override;

   // LoraRadio specific methods

   /**
    * @brief interrupt callabarck for the radio.
    *
    */
   static void onInterrupt();

   /**
    * @brief Indicates if the radio has been setup.
    *
    * @return true if the radio has been setup, false otherwise
    */
   bool isRadioSetup()
   {
      return isSetup;
   }

   /**
    * @brief Get the radio parameters.
    *
    * @return LoraRadioParams object containing the radio parameters
    */
   LoraRadioParams getParams()
   {
      return radioParams;
   }

   /**
    * @brief Set the radio parameters.
    *
    * @param params LoraRadioParams object containing the radio parameters
    * @return RM_E_NONE if the parameters were successfully set, an error code otherwise.
    */
   int setParams(LoraRadioParams params);

   /**
    * @brief Send a packet of data.
    *
    * @param data vector of bytes containing the data to send
    * @return RM_E_NONE if the packet was successfully sent, an error code otherwise.
    */
   int sendPacket(std::vector<byte>& data);

   /**
    * @brief switch the radio to receive mode.
    *
    * @return RM_E_NONE if the radio was successfully switched to receive mode, an error code
    * otherwise.
    */
   int startReceive();

   /**
    * @brief Interrupt driven method to start transmitting a packet.
    *
    * @param data the byte array containing the data to transmit
    * @param length the length of the data array
    * @return RM_E_NONE if the packet was successfully transmitted, an error code otherwise.
    */
   int startTransmitPacket(byte* data, int length);

   /**
    * @brief Read the received data from the radio.
    *
    * @param packetData vector of bytes to store the received data
    * @return WAR_ERR_NONE if the data was successfully read, an error code otherwise.
    */
   int readReceivedData(std::vector<byte>* packetData);

   /**
    * @brief Check if the radio is in receive mode.
    * @return true if the radio is in receive mode, false otherwise.
    */
   bool checkAndClearRxFlag();

   /**
    * @brief Check if the radio is in transmit mode.
    * @return true if the radio is in transmit mode, false otherwise.
    */
   bool checkAndClearTxFlag();

   /**
    * @brief Get the radio state error.
    * @return the radio state error.
    */
   int getRadioStateError();

private:
   LoraRadio()
   {
   }
   LoraRadio(const LoraRadio&) = delete;      // Prevent copy
   void operator=(const LoraRadio&) = delete; // Prevent assignment

   static LoraRadio* instance;

   volatile bool rxDone = false;
   volatile bool txDone = false;
   volatile bool isSetup = false;
   volatile int16_t radioStateError = RM_E_NONE;

   void resetRadioState(int flag = RX_TX_STATE)
   {
      if (flag == RX_TX_STATE) {
         rxDone = false;
         txDone = false;
      } else if (flag == RX_STATE) {
         rxDone = false;
      } else if (flag == TX_STATE) {
         txDone = false;
      }
      radioStateError = RM_E_NONE;
   }

   LoraRadioParams radioParams;
   std::unique_ptr<SX1262> radio;

   int checkLoraParameters(LoraRadioParams params);
   int switchToReceiveMode();
   int createModule(const LoraRadioParams& params);
};
