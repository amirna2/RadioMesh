#pragma once

#include <string>
#include <vector>

#include <common/inc/Definitions.h>
#include <framework/interfaces/IAesCrypto.h>

/***
 * @class AesCrypto class
 * @brief This class provides AES encryption and decryption functionality.
 *
 */
class AesCrypto : public IAesCrypto
{
public:
   static const uint8_t AES_KEY_SIZE = 32;
   static const uint8_t AES_IV_SIZE = 16;

   static AesCrypto* getInstance()
   {
      if (!instance) {
         instance = new AesCrypto();
      }
      return instance;
   }

   virtual ~AesCrypto() = default;

   // IAesCrypto interface
   virtual int resetSecurityParams(const SecurityParams& params) override;
   virtual std::vector<byte> encrypt(const std::vector<byte>& clearData) override;
   virtual std::vector<byte> decrypt(const std::vector<byte>& encryptedData) override;

   int setParams(const SecurityParams& params);

private:
   AesCrypto()
   {
   }
   AesCrypto(const AesCrypto&) = delete;
   void operator=(const AesCrypto&) = delete;
   static AesCrypto* instance;
   SecurityParams securityParams;

   const uint8_t AES_BLOCK_SIZE = 16;
   const uint8_t AES_COUNTER_SIZE = 4;
};
