#pragma once

#include <string>
#include <vector>

#include <common/inc/Options.h>

/**
 * @class ICrypto
 * @brief This class is an interface for cryptographic operations.
 *
 * It provides the structure for setting, getting, and generating keys, as well as encrypting and decrypting data.
 */
class ICrypto
{
public:
   virtual ~ICrypto() = default;

   /**
    * @brief Set the key for the crypto object.
    * @param key The key to set.
   */
   virtual void setKey(const std::vector<byte>& key) = 0;

   /**
    * @brief Get the key for the crypto object.
    * @return std::vector<byte> containing the key.
   */
   virtual const std::vector<byte> getKey() = 0;

   /**
    * @brief Generate a key of the specified size.
    * @param keySize The size of the key to generate.
    * @return std::vector<byte> containing the generated key.
   */
   virtual const std::vector<byte> generateKey(size_t keySize) = 0;

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
