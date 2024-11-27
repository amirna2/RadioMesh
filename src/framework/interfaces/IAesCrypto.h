#pragma once

#include <string>
#include <vector>

#include <common/inc/Options.h>
#include <framework/interfaces/ICrypto.h>

/***
 * @class IAesCrypto class
 * @brief This class provides AES encryption and decryption functionality by extending the ICrypto interface.
 *
*/
class IAesCrypto : public ICrypto
{
public:
   virtual ~IAesCrypto() = default;

   virtual void setIV(const std::vector<byte>& iv) = 0;
   virtual const std::vector<byte> getIV() = 0;
};
