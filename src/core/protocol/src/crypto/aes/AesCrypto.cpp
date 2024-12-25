
#include <AES.h>
#include <CTR.h>
#include <Crypto.h>
#include <string>
#include <vector>

#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>

CTR<AES256> ctraes256;

AesCrypto* AesCrypto::instance = nullptr;

int AesCrypto::setParams(const SecurityParams& params)
{
    if (params.key.size() != AES_KEY_SIZE) {
        logerr_ln("ERROR: Invalid key size");
        return RM_E_INVALID_PARAM;
    }
    if (params.iv.size() != AES_IV_SIZE) {
        logerr_ln("ERROR: Invalid IV size");
        return RM_E_INVALID_PARAM;
    }

    this->securityParams.key.assign(params.key.begin(), params.key.end());
    this->securityParams.iv.assign(params.iv.begin(), params.iv.end());

    return RM_E_NONE;
}

int AesCrypto::resetSecurityParams(const SecurityParams& params)
{
    return setParams(params);
}

std::vector<byte> AesCrypto::encrypt(const std::vector<byte>& clearData)
{
    size_t size = clearData.size();
    size_t len;
    size_t posn;

    std::vector<byte> encryptedData(size);
    ctraes256.clear();
    ctraes256.setKey(this->securityParams.key.data(), AES_KEY_SIZE);
    ctraes256.setIV(this->securityParams.iv.data(), AES_IV_SIZE);
    ctraes256.setCounterSize(4);

    size_t blockSize = AES_BLOCK_SIZE;

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
    ctraes256.setKey(this->securityParams.key.data(), AES_KEY_SIZE);
    ctraes256.setIV(this->securityParams.iv.data(), AES_IV_SIZE);
    ctraes256.setCounterSize(AES_COUNTER_SIZE);

    size_t blockSize = AES_BLOCK_SIZE;

    for (posn = 0; posn < size; posn += blockSize) {
        len = size - posn;
        if (len > blockSize) {
            len = blockSize;
        }
        ctraes256.decrypt(decryptedData.data() + posn, encryptedData.data() + posn, len);
    }

    return decryptedData;
}
