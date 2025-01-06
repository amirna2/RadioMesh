#pragma once

#include <algorithm>
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <common/utils/Utils.h>
#include <hardware/inc/storage/eeprom/EEPROMStorage.h>

class DeviceStorage
{
private:
    IByteStorage* storage;

    // Storage keys
    const std::string STATE_KEY = "is"; // inclusion state
    const std::string CTR_KEY = "mc";   // message counter
    const std::string SKEY = "sk";      // session key
    const std::string PRIV_KEY = "pk";  // device private key
    const std::string HUB_KEY = "hk";   // hub public key

public:
    explicit DeviceStorage(IByteStorage* storage) : storage(storage)
    {
    }

    int persistState(DeviceInclusionState state)
    {
        std::vector<byte> stateData = {static_cast<byte>(state)};
        return storage->writeAndCommit(STATE_KEY, stateData);
    }

    int loadState(DeviceInclusionState& state)
    {
        std::vector<byte> stateData;
        int rc = storage->read(STATE_KEY, stateData);
        if (rc == RM_E_NONE && !stateData.empty()) {
            state = static_cast<DeviceInclusionState>(stateData[0]);
        } else if (rc == RM_E_STORAGE_KEY_NOT_FOUND) {
            state = DeviceInclusionState::NOT_INCLUDED;
            rc = RM_E_NONE;
        }
        return rc;
    }

    int persistMessageCounter(uint32_t counter)
    {
        return storage->writeAndCommit(CTR_KEY, RadioMeshUtils::numberToBytes(counter));
    }

    int loadMessageCounter(uint32_t& counter)
    {
        std::vector<byte> counterData;
        int rc = storage->read(CTR_KEY, counterData);
        if (rc == RM_E_NONE && counterData.size() == sizeof(uint32_t)) {
            counter = RadioMeshUtils::bytesToNumber<uint32_t>(counterData);
        }
        return rc;
    }

    int persistSessionKey(const std::vector<byte>& key)
    {
        return storage->writeAndCommit(SKEY, key);
    }

    int loadSessionKey(std::vector<byte>& key)
    {
        return storage->read(SKEY, key);
    }

    int persistPrivateKey(const std::vector<byte>& key)
    {
        return storage->writeAndCommit(PRIV_KEY, key);
    }

    int loadPrivateKey(std::vector<byte>& key)
    {
        return storage->read(PRIV_KEY, key);
    }

    int persistHubKey(const std::vector<byte>& key)
    {
        return storage->writeAndCommit(HUB_KEY, key);
    }

    int loadHubKey(std::vector<byte>& key)
    {
        return storage->read(HUB_KEY, key);
    }
};
