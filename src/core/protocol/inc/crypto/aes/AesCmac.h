#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <common/inc/Definitions.h>

/**
 * @class AesCmac
 * @brief Implementation of AES-CMAC (RFC 4493) for message authentication
 *
 * This class provides AES-CMAC computation for generating Message Integrity Codes (MICs)
 * in the RadioMesh protocol. It uses the shared network key to authenticate packets.
 */
class AesCmac
{
public:
    static constexpr size_t AES_BLOCK_SIZE = 16;
    static constexpr size_t AES_KEY_SIZE = 32;  // 256-bit key
    static constexpr size_t CMAC_SIZE = 16;     // Full CMAC output
    static constexpr size_t MIC_SIZE = 4;       // Truncated MIC size

    /**
     * @brief Get the singleton instance
     * @return Pointer to the AesCmac instance
     */
    static AesCmac* getInstance()
    {
        if (!instance) {
            instance = new AesCmac();
        }
        return instance;
    }

    virtual ~AesCmac() = default;

    /**
     * @brief Set the key for CMAC computation
     * @param key The AES key (must be 32 bytes)
     * @return RM_E_NONE on success, error code otherwise
     */
    int setKey(const std::vector<byte>& key);

    /**
     * @brief Compute CMAC for given data
     * @param data Input data to authenticate
     * @param cmac Output buffer for full CMAC (16 bytes)
     * @return RM_E_NONE on success, error code otherwise
     */
    int computeCmac(const std::vector<byte>& data, std::array<byte, CMAC_SIZE>& cmac);

    /**
     * @brief Compute truncated MIC for packet authentication
     * @param data Input data (header + encrypted payload)
     * @param mic Output buffer for truncated MIC (4 bytes)
     * @return RM_E_NONE on success, error code otherwise
     */
    int computeMic(const std::vector<byte>& data, std::array<byte, MIC_SIZE>& mic);

    /**
     * @brief Verify MIC for received packet
     * @param data Input data (header + encrypted payload)
     * @param receivedMic The MIC to verify (4 bytes)
     * @return true if MIC is valid, false otherwise
     */
    bool verifyMic(const std::vector<byte>& data, const std::array<byte, MIC_SIZE>& receivedMic);

private:
    AesCmac() = default;
    AesCmac(const AesCmac&) = delete;
    void operator=(const AesCmac&) = delete;

    static AesCmac* instance;

    // Key storage
    std::vector<byte> aesKey;
    bool keySet = false;

    // Subkey generation (K1, K2) per RFC 4493
    std::array<byte, AES_BLOCK_SIZE> subkeyK1;
    std::array<byte, AES_BLOCK_SIZE> subkeyK2;

    /**
     * @brief Generate subkeys K1 and K2 from main key
     * @return RM_E_NONE on success, error code otherwise
     */
    int generateSubkeys();

    /**
     * @brief Left shift a block by one bit
     * @param input Input block
     * @param output Output block (can be same as input)
     */
    void leftShiftBlock(const byte* input, byte* output);

    /**
     * @brief XOR two blocks
     * @param a First block
     * @param b Second block
     * @param output Result block (can be same as a or b)
     */
    void xorBlock(const byte* a, const byte* b, byte* output);

    /**
     * @brief Apply CMAC padding to last block
     * @param lastBlock The last block to pad
     * @param lastBlockSize Size of data in last block
     * @param padded Output padded block
     * @param useK1 true to XOR with K1, false to pad and XOR with K2
     */
    void padLastBlock(const byte* lastBlock, size_t lastBlockSize, byte* padded, bool useK1);
};