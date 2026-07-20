#pragma once

#include <string>
#include <vector>

#include <framework/interfaces/IByteStorage.h>

#include <zephyr/kvss/nvs.h>

/**
 * @class ZephyrNVSStorage
 *
 * @brief This class implements the IByteStorage interface over Zephyr NVS.
 *
 * Data lives in the devicetree `storage_partition` (mounted in begin()).
 * String keys map to 16-bit NVS ids: the reserved RadioMesh keys get fixed
 * ids, any other key hashes into a dynamic id range.
 *
 * NVS writes are atomic and immediately persistent, so write() persists
 * directly and commit() is a no-op; NVS also garbage-collects sectors on its
 * own, making defragment() a no-op as well.
 */
class ZephyrNVSStorage : public IByteStorage
{
public:
    /**
     * @brief Get the instance of the ZephyrNVSStorage.
     * @returns A pointer to the instance of the ZephyrNVSStorage.
     */
    static ZephyrNVSStorage* getInstance();

    virtual ~ZephyrNVSStorage() = default;

    // IByteStorage interface
    int read(const std::string& key, std::vector<byte>& data) override;
    int write(const std::string& key, const std::vector<byte>& data) override;
    int remove(const std::string& key) override;
    bool exists(const std::string& key) override;
    int writeAndCommit(const std::string& key, const std::vector<byte>& data) override;
    int commit() override;

    int begin() override;
    int end() override;
    int clear() override;
    size_t available() override;
    bool isFull() override;
    int defragment() override;

private:
    ZephyrNVSStorage() = default;
    ZephyrNVSStorage(const ZephyrNVSStorage&) = delete; // Prevent copy
    void operator=(const ZephyrNVSStorage&) = delete;   // Prevent assignment

    uint16_t keyToId(const std::string& key);

    struct nvs_fs fs = {};
    bool initialized = false;
};
