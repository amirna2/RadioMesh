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
class AesCrypto: public IAesCrypto
{
public:

   static AesCrypto *getInstance()
   {
      if (!instance) {
         instance = new AesCrypto();
      }
      return instance;
   }

   virtual ~AesCrypto() = default;

   // IAesCrypto interface
   virtual void setKey(const std::vector<byte>& key) override;
   virtual const std::vector<byte> getKey() override;
   virtual const std::vector<byte> generateKey(size_t keySize) override;
   virtual void setIV(const std::vector<byte>& iv) override;
   virtual const std::vector<byte> getIV() override;
   virtual std::vector<byte> encrypt(const std::vector<byte>& clearData) override;
   virtual std::vector<byte> decrypt(const std::vector<byte>& encryptedData) override;

   // AesCrypto specific
   int setParams(const std::vector<byte>& key, const std::vector<byte>& iv);
private:
   AesCrypto() {}
   AesCrypto(const AesCrypto &) = delete;
   void operator=(const AesCrypto &) = delete;
   static AesCrypto *instance;

   std::vector<byte> key;
   std::vector<byte> iv;
};
