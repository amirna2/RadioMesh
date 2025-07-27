#pragma once

#include <common/inc/Definitions.h>
#include <vector>

/**
 * @class AesCmac
 * @brief AES-CMAC implementation according to RFC 4493
 *
 * Provides Message Authentication Code (MAC) computation using AES in CMAC mode.
 * Supports 128-bit output with optional truncation to 32 bits for RadioMesh MIC.
 */
class AesCmac
{
public:
    static const uint8_t AES_BLOCK_SIZE = 16;
    static const uint8_t CMAC_OUTPUT_SIZE = 16;
    static const uint8_t CMAC_MIC_SIZE = 4;

    /**
     * @brief Compute AES-CMAC for given data
     * @param key AES key (16, 24, or 32 bytes)
     * @param data Input data to authenticate
     * @return Full 128-bit CMAC output
     */
    static std::vector<byte> computeCMAC(const std::vector<byte>& key, 
                                        const std::vector<byte>& data);

    /**
     * @brief Compute truncated MIC for RadioMesh packets
     * @param key AES key (16, 24, or 32 bytes)
     * @param data Input data to authenticate
     * @return 32-bit (4-byte) truncated MIC
     */
    static std::vector<byte> computeMIC(const std::vector<byte>& key, 
                                       const std::vector<byte>& data);

    /**
     * @brief Verify MIC against expected value
     * @param key AES key used for computation
     * @param data Original data
     * @param receivedMic MIC to verify
     * @return true if MIC is valid, false otherwise
     */
    static bool verifyMIC(const std::vector<byte>& key,
                         const std::vector<byte>& data,
                         const std::vector<byte>& receivedMic);

private:
    /**
     * @brief Generate subkeys K1 and K2 for CMAC
     * @param key AES encryption key
     * @param k1 Output subkey K1
     * @param k2 Output subkey K2
     */
    static void generateSubkeys(const std::vector<byte>& key, 
                               std::vector<byte>& k1, 
                               std::vector<byte>& k2);

    /**
     * @brief Left shift operation for subkey generation
     * @param input Input block
     * @return Left-shifted block
     */
    static std::vector<byte> leftShift(const std::vector<byte>& input);

    /**
     * @brief XOR two byte vectors
     * @param a First vector
     * @param b Second vector
     * @return XOR result
     */
    static std::vector<byte> xorVectors(const std::vector<byte>& a, 
                                       const std::vector<byte>& b);

    /**
     * @brief Apply PKCS#7 padding to data
     * @param data Input data
     * @param blockSize Target block size
     * @return Padded data
     */
    static std::vector<byte> padData(const std::vector<byte>& data, size_t blockSize);

    /**
     * @brief Encrypt single AES block
     * @param key AES key
     * @param block 16-byte block to encrypt
     * @return Encrypted block
     */
    static std::vector<byte> aesEncryptBlock(const std::vector<byte>& key, 
                                            const std::vector<byte>& block);
};