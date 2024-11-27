
#include <vector>
#include <string>
#include <AES.h>
#include <CTR.h>
#include <Crypto.h>

#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <common/inc/Errors.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>

CTR<AES256> ctraes256;

AesCrypto* AesCrypto::instance = nullptr;


int AesCrypto::setParams(const std::vector<byte>& key, const std::vector<byte>& iv)
{
   if (key.size() != 32) {
      logerr_ln("ERROR: Invalid key size");
      return RM_E_INVALID_PARAM;
   }
   if (iv.size() != 16) {
      logerr_ln("ERROR: Invalid IV size");
      return RM_E_INVALID_PARAM;
   }

   this->key.assign(key.begin(), key.end());
   this->iv.assign(iv.begin(), iv.end());

   return RM_E_NONE;
}
void AesCrypto::setKey(const std::vector<byte>& key)
{
   this->key.assign(key.begin(), key.end());
}

const std::vector<byte> AesCrypto::getKey()
{
   return key;
}

void AesCrypto::setIV(const std::vector<byte>& iv)
{
   this->iv.assign(iv.begin(), iv.end());
}

const std::vector<byte> AesCrypto::getIV()
{
   return std::vector<byte>(iv.begin(), iv.end());
}

const std::vector<byte> AesCrypto::generateKey(size_t keySize)
{
   // TODO: This is a dummy implementation. Replace with actual key generation
   key = std::vector<byte>(keySize);
   return std::vector<byte>(key.begin(), key.end());
}

std::vector<byte> AesCrypto::encrypt(const std::vector<byte>& clearData)
{
   size_t size = clearData.size();
   size_t len;
   size_t posn;

   std::vector<byte> encryptedData(size);
   ctraes256.clear();
   ctraes256.setKey(this->key.data(), 32);
   ctraes256.setIV(this->iv.data(), 16);
   ctraes256.setCounterSize(4);

   size_t blockSize = 16;

   for (posn = 0; posn < size; posn += blockSize) {
     len = size - posn;
     if (len > blockSize) {
       len = blockSize;
     }
     ctraes256.encrypt(encryptedData.data() + posn, clearData.data() + posn, len);
   }

   return encryptedData;
}

std::vector<byte> AesCrypto::decrypt(const std::vector<byte>& encryptedData)
{
   size_t size = encryptedData.size();
   size_t len;
   size_t posn;

   std::vector<byte> decryptedData(size);
   ctraes256.clear();
   ctraes256.setKey(this->key.data(), 32);
   ctraes256.setIV(this->iv.data(), 16);
   ctraes256.setCounterSize(4);

   size_t blockSize = 16;

   for (posn = 0; posn < size; posn += blockSize) {
     len = size - posn;
     if (len > blockSize) {
       len = blockSize;
     }
     ctraes256.decrypt(decryptedData.data() + posn, encryptedData.data() + posn, len);
   }

   return decryptedData;
}


