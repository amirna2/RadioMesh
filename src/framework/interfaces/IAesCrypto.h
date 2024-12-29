#pragma once

#include <string>
#include <vector>

#include <common/inc/Options.h>
#include <framework/interfaces/ICrypto.h>

/***
 * @class IAesCrypto class
 * @brief This class provides AES encryption and decryption functionality by extending the ICrypto
 * interface.
 *
 */
class IAesCrypto : public ICrypto
{
public:
    virtual ~IAesCrypto() = default;
};
