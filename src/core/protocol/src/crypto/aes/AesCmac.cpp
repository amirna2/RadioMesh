#include <core/protocol/inc/crypto/aes/AesCmac.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <cstring>

// Use the same crypto library as AesCrypto
#include <AES.h>
#include <Crypto.h>

AesCmac* AesCmac::instance = nullptr;

// Constants for CMAC
static const byte CONST_RB = 0x87; // R(128) = 0x87 for AES-128/256

int AesCmac::setKey(const std::vector<byte>& key)
{
    if (key.size() != AES_KEY_SIZE) {
        logerr_ln("Invalid key size for AES-CMAC: %d", key.size());
        return RM_E_INVALID_PARAM;
    }

    aesKey = key;
    keySet = true;

    // Generate subkeys K1 and K2
    return generateSubkeys();
}

int AesCmac::generateSubkeys()
{
    if (!keySet) {
        return RM_E_NOT_INITIALIZED;
    }

    // Step 1: AES encrypt zero block to get L
    std::array<byte, AES_BLOCK_SIZE> zeroBlock;
    std::array<byte, AES_BLOCK_SIZE> L;
    zeroBlock.fill(0);

    // Use AES256 in ECB mode for subkey generation
    AES256 aes256;
    aes256.setKey(aesKey.data(), AES_KEY_SIZE);
    aes256.encryptBlock(L.data(), zeroBlock.data());

    // Step 2: Generate K1 from L
    byte msb = L[0] & 0x80;
    leftShiftBlock(L.data(), subkeyK1.data());
    if (msb) {
        subkeyK1[AES_BLOCK_SIZE - 1] ^= CONST_RB;
    }

    // Step 3: Generate K2 from K1
    msb = subkeyK1[0] & 0x80;
    leftShiftBlock(subkeyK1.data(), subkeyK2.data());
    if (msb) {
        subkeyK2[AES_BLOCK_SIZE - 1] ^= CONST_RB;
    }

    logdbg_ln("Subkeys generated successfully");
    return RM_E_NONE;
}

void AesCmac::leftShiftBlock(const byte* input, byte* output)
{
    byte overflow = 0;
    for (int i = AES_BLOCK_SIZE - 1; i >= 0; i--) {
        byte current = input[i];
        output[i] = (current << 1) | overflow;
        overflow = (current & 0x80) ? 1 : 0;
    }
}

void AesCmac::xorBlock(const byte* a, const byte* b, byte* output)
{
    for (size_t i = 0; i < AES_BLOCK_SIZE; i++) {
        output[i] = a[i] ^ b[i];
    }
}

void AesCmac::padLastBlock(const byte* lastBlock, size_t lastBlockSize, byte* padded, bool useK1)
{
    memset(padded, 0, AES_BLOCK_SIZE);
    memcpy(padded, lastBlock, lastBlockSize);

    if (useK1) {
        // Complete block - XOR with K1
        xorBlock(padded, subkeyK1.data(), padded);
    } else {
        // Incomplete block - pad with 10* and XOR with K2
        padded[lastBlockSize] = 0x80;
        xorBlock(padded, subkeyK2.data(), padded);
    }
}

int AesCmac::computeCmac(const std::vector<byte>& data, std::array<byte, CMAC_SIZE>& cmac)
{
    if (!keySet) {
        logerr_ln("AES-CMAC key not set");
        return RM_E_NOT_INITIALIZED;
    }

    size_t dataLen = data.size();
    size_t numBlocks = (dataLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE;
    
    if (numBlocks == 0) {
        numBlocks = 1; // Handle empty message
    }

    // Initialize AES256 cipher
    AES256 aes256;
    aes256.setKey(aesKey.data(), AES_KEY_SIZE);

    // Initialize Y = 0
    std::array<byte, AES_BLOCK_SIZE> Y;
    Y.fill(0);

    // Process all blocks except the last
    for (size_t i = 0; i < numBlocks - 1; i++) {
        std::array<byte, AES_BLOCK_SIZE> block;
        memcpy(block.data(), data.data() + (i * AES_BLOCK_SIZE), AES_BLOCK_SIZE);
        
        // Y = Y XOR M[i]
        xorBlock(Y.data(), block.data(), Y.data());
        
        // Y = AES(K, Y)
        aes256.encryptBlock(Y.data(), Y.data());
    }

    // Process last block with padding
    size_t lastBlockStart = (numBlocks - 1) * AES_BLOCK_SIZE;
    size_t lastBlockSize = dataLen - lastBlockStart;
    
    std::array<byte, AES_BLOCK_SIZE> lastBlock;
    lastBlock.fill(0);
    
    if (lastBlockSize > 0) {
        memcpy(lastBlock.data(), data.data() + lastBlockStart, lastBlockSize);
    }
    
    // Apply padding and appropriate subkey
    std::array<byte, AES_BLOCK_SIZE> paddedLastBlock;
    bool isComplete = (lastBlockSize == AES_BLOCK_SIZE) && (dataLen > 0);
    padLastBlock(lastBlock.data(), lastBlockSize, paddedLastBlock.data(), isComplete);
    
    // Final step: Y = AES(K, Y XOR paddedLastBlock)
    xorBlock(Y.data(), paddedLastBlock.data(), Y.data());
    aes256.encryptBlock(cmac.data(), Y.data());

    return RM_E_NONE;
}

int AesCmac::computeMic(const std::vector<byte>& data, std::array<byte, MIC_SIZE>& mic)
{
    std::array<byte, CMAC_SIZE> fullCmac;
    
    int rc = computeCmac(data, fullCmac);
    if (rc != RM_E_NONE) {
        return rc;
    }

    // Truncate to 4 bytes (take first 4 bytes)
    memcpy(mic.data(), fullCmac.data(), MIC_SIZE);
    
    logdbg_ln("MIC computed: %02X%02X%02X%02X", 
              mic[0], mic[1], mic[2], mic[3]);
    
    return RM_E_NONE;
}

bool AesCmac::verifyMic(const std::vector<byte>& data, const std::array<byte, MIC_SIZE>& receivedMic)
{
    std::array<byte, MIC_SIZE> computedMic;
    
    if (computeMic(data, computedMic) != RM_E_NONE) {
        logerr_ln("Failed to compute MIC for verification");
        return false;
    }

    // Constant-time comparison
    byte diff = 0;
    for (size_t i = 0; i < MIC_SIZE; i++) {
        diff |= computedMic[i] ^ receivedMic[i];
    }
    
    bool valid = (diff == 0);
    
    if (!valid) {
        logwarn_ln("MIC verification failed - Expected: %02X%02X%02X%02X, Received: %02X%02X%02X%02X",
                   computedMic[0], computedMic[1], computedMic[2], computedMic[3],
                   receivedMic[0], receivedMic[1], receivedMic[2], receivedMic[3]);
    }
    
    return valid;
}