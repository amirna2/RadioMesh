#include <core/protocol/inc/crypto/cmac/AesCmac.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <common/inc/Logger.h>
#include <AES.h>
#include <algorithm>

const uint8_t AesCmac::AES_BLOCK_SIZE;
const uint8_t AesCmac::CMAC_OUTPUT_SIZE;
const uint8_t AesCmac::CMAC_MIC_SIZE;

std::vector<byte> AesCmac::computeCMAC(const std::vector<byte>& key, 
                                      const std::vector<byte>& data)
{
    if (key.empty() || (key.size() != 16 && key.size() != 24 && key.size() != 32)) {
        logerr_ln("Invalid AES key size for CMAC: %d", key.size());
        return std::vector<byte>();
    }

    // Generate subkeys K1 and K2
    std::vector<byte> k1, k2;
    generateSubkeys(key, k1, k2);

    // Split data into blocks
    std::vector<std::vector<byte>> blocks;
    for (size_t i = 0; i < data.size(); i += AES_BLOCK_SIZE) {
        size_t remaining = data.size() - i;
        size_t blockSize = (remaining < static_cast<size_t>(AES_BLOCK_SIZE)) ? remaining : static_cast<size_t>(AES_BLOCK_SIZE);
        blocks.emplace_back(data.begin() + i, data.begin() + i + blockSize);
    }

    // Handle empty data case
    if (blocks.empty()) {
        blocks.emplace_back(AES_BLOCK_SIZE, 0x80); // Single block with padding
        for (int i = 1; i < AES_BLOCK_SIZE; i++) {
            blocks[0][i] = 0x00;
        }
    }

    // Process all blocks except the last
    std::vector<byte> y(AES_BLOCK_SIZE, 0);
    for (size_t i = 0; i < blocks.size() - 1; i++) {
        y = xorVectors(y, blocks[i]);
        y = aesEncryptBlock(key, y);
    }

    // Process the last block
    std::vector<byte>& lastBlock = blocks.back();
    if (lastBlock.size() == AES_BLOCK_SIZE) {
        // Complete block - use K1
        lastBlock = xorVectors(lastBlock, k1);
    } else {
        // Incomplete block - pad and use K2
        lastBlock = padData(lastBlock, AES_BLOCK_SIZE);
        lastBlock = xorVectors(lastBlock, k2);
    }

    y = xorVectors(y, lastBlock);
    return aesEncryptBlock(key, y);
}

std::vector<byte> AesCmac::computeMIC(const std::vector<byte>& key, 
                                     const std::vector<byte>& data)
{
    std::vector<byte> fullCmac = computeCMAC(key, data);
    if (fullCmac.size() < CMAC_MIC_SIZE) {
        logerr_ln("CMAC output too short for MIC truncation");
        return std::vector<byte>();
    }

    // Truncate to first 4 bytes for MIC
    return std::vector<byte>(fullCmac.begin(), fullCmac.begin() + CMAC_MIC_SIZE);
}

bool AesCmac::verifyMIC(const std::vector<byte>& key,
                       const std::vector<byte>& data,
                       const std::vector<byte>& receivedMic)
{
    if (receivedMic.size() != CMAC_MIC_SIZE) {
        logerr_ln("Invalid MIC size: %d (expected %d)", receivedMic.size(), CMAC_MIC_SIZE);
        return false;
    }

    std::vector<byte> computedMic = computeMIC(key, data);
    if (computedMic.size() != CMAC_MIC_SIZE) {
        logerr_ln("Failed to compute MIC for verification");
        return false;
    }

    // Constant-time comparison to prevent timing attacks
    bool isValid = true;
    for (size_t i = 0; i < CMAC_MIC_SIZE; i++) {
        isValid &= (computedMic[i] == receivedMic[i]);
    }

    return isValid;
}

void AesCmac::generateSubkeys(const std::vector<byte>& key, 
                             std::vector<byte>& k1, 
                             std::vector<byte>& k2)
{
    // Encrypt zero block to get L
    std::vector<byte> zeroBlock(AES_BLOCK_SIZE, 0);
    std::vector<byte> L = aesEncryptBlock(key, zeroBlock);

    // Generate K1
    k1 = leftShift(L);
    if (L[0] & 0x80) {
        // MSB of L is 1, XOR with Rb
        k1[AES_BLOCK_SIZE - 1] ^= 0x87;
    }

    // Generate K2
    k2 = leftShift(k1);
    if (k1[0] & 0x80) {
        // MSB of K1 is 1, XOR with Rb
        k2[AES_BLOCK_SIZE - 1] ^= 0x87;
    }
}

std::vector<byte> AesCmac::leftShift(const std::vector<byte>& input)
{
    std::vector<byte> result(input.size(), 0);
    
    for (size_t i = 0; i < input.size(); i++) {
        result[i] = (input[i] << 1);
        if (i + 1 < input.size() && (input[i + 1] & 0x80)) {
            result[i] |= 0x01;
        }
    }
    
    return result;
}

std::vector<byte> AesCmac::xorVectors(const std::vector<byte>& a, 
                                     const std::vector<byte>& b)
{
    if (a.size() != b.size()) {
        logerr_ln("Vector size mismatch in XOR: %d vs %d", a.size(), b.size());
        return std::vector<byte>();
    }

    std::vector<byte> result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

std::vector<byte> AesCmac::padData(const std::vector<byte>& data, size_t blockSize)
{
    std::vector<byte> padded = data;
    
    // Add 0x80 byte
    padded.push_back(0x80);
    
    // Add 0x00 bytes to reach block size
    while (padded.size() < blockSize) {
        padded.push_back(0x00);
    }
    
    return padded;
}

std::vector<byte> AesCmac::aesEncryptBlock(const std::vector<byte>& key, 
                                          const std::vector<byte>& block)
{
    if (block.size() != AES_BLOCK_SIZE) {
        logerr_ln("Invalid block size for AES encryption: %d", block.size());
        return std::vector<byte>();
    }

    // Use the Crypto library's AES implementation
    AES256 aes;
    
    // Set the key based on key size
    if (key.size() == 16) {
        AES128 aes128;
        aes128.setKey(key.data(), key.size());
        
        std::vector<byte> result(AES_BLOCK_SIZE);
        aes128.encryptBlock(result.data(), block.data());
        return result;
    } else if (key.size() == 32) {
        aes.setKey(key.data(), key.size());
        
        std::vector<byte> result(AES_BLOCK_SIZE);
        aes.encryptBlock(result.data(), block.data());
        return result;
    } else {
        logerr_ln("Unsupported AES key size: %d", key.size());
        return std::vector<byte>();
    }
}