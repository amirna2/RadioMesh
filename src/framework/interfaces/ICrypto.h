#pragma once

#include <string>
#include <vector>

#include <common/inc/Options.h>

/**
 * @class ICrypto
 * @brief This class is an interface for cryptographic operations.
 *
 * It provides the structure for setting, getting, and generating keys, as well as encrypting and
 * decrypting data.
 */
class ICrypto
{
public:
    virtual ~ICrypto() = default;

    /**
     * @brief Reset security parameters.
     * @param params New
     * @warning This executes only when the device is instructed to reset the security parameters
     * from the hub.
     * @return RM_E_NONE if the security parameters were successfully reset, an error code
     * otherwise.
     */
    virtual int resetSecurityParams(const SecurityParams& params) = 0;
    /**
     * @brief Encrypt the provided data.
     * @param clearData The data to encrypt.
     * @return std::vector<byte> containing the encrypted data.
     */
    virtual std::vector<byte> encrypt(const std::vector<byte>& clearData) = 0;

    /**
     * @brief Decrypt the provided data.
     * @param encryptedData The data to decrypt.
     * @return std::vector<byte> containing the decrypted data.
     */
    virtual std::vector<byte> decrypt(const std::vector<byte>& encryptedData) = 0;
};
