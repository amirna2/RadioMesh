#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <storage/ZephyrNVSStorage.h>

namespace
{
// The reserved RadioMesh keys (see DeviceStorage.h) get fixed NVS ids; any
// other key hashes (FNV-1a) into [DYNAMIC_ID_BASE, 0xFFFE]. Hash collisions
// are possible in the dynamic range, but the core persists only the reserved
// keys today.
constexpr uint16_t DYNAMIC_ID_BASE = 0x0100;
constexpr uint16_t DYNAMIC_ID_MAX = 0xFFFE;

struct ReservedKey
{
    const char* key;
    uint16_t id;
};

constexpr ReservedKey RESERVED_KEYS[] = {
    {"is", 1}, // inclusion state
    {"mc", 2}, // message counter
    {"nk", 3}, // network key
    {"pk", 4}, // device private key
    {"hk", 5}, // hub public key
};
} // namespace

ZephyrNVSStorage* ZephyrNVSStorage::getInstance()
{
    // Static-storage singleton: same contract as the Arduino port's lazy new,
    // without requiring heap allocation in the port layer.
    static ZephyrNVSStorage instance;
    return &instance;
}

uint16_t ZephyrNVSStorage::keyToId(const std::string& key)
{
    for (const auto& reserved : RESERVED_KEYS) {
        if (key == reserved.key) {
            return reserved.id;
        }
    }

    // FNV-1a 32-bit, folded to 16 bits and offset into the dynamic range.
    uint32_t hash = 2166136261u;
    for (char c : key) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    uint16_t folded = static_cast<uint16_t>((hash ^ (hash >> 16)) & 0xFFFF);
    return DYNAMIC_ID_BASE + (folded % (DYNAMIC_ID_MAX - DYNAMIC_ID_BASE + 1));
}

int ZephyrNVSStorage::begin()
{
    if (initialized) {
        return RM_E_NONE;
    }

    fs.flash_device = PARTITION_DEVICE(storage_partition);
    if (!device_is_ready(fs.flash_device)) {
        logerr_ln("ERROR storage flash device not ready");
        return RM_E_STORAGE_SETUP;
    }
    fs.offset = PARTITION_OFFSET(storage_partition);

    struct flash_pages_info pageInfo;
    int rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &pageInfo);
    if (rc < 0) {
        logerr_ln("ERROR flash page info failed, code %d", rc);
        return RM_E_STORAGE_SETUP;
    }
    fs.sector_size = pageInfo.size;
    fs.sector_count = PARTITION_SIZE(storage_partition) / pageInfo.size;

    rc = nvs_mount(&fs);
    if (rc < 0) {
        logerr_ln("ERROR nvs_mount failed, code %d", rc);
        return RM_E_STORAGE_SETUP;
    }

    initialized = true;
    loginfo_ln("NVS storage mounted (%u sectors of %u bytes)",
               static_cast<unsigned>(fs.sector_count), static_cast<unsigned>(fs.sector_size));
    return RM_E_NONE;
}

int ZephyrNVSStorage::end()
{
    // NVS keeps no open handles; forgetting the mount is all there is to do.
    initialized = false;
    return RM_E_NONE;
}

int ZephyrNVSStorage::read(const std::string& key, std::vector<byte>& data)
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    uint16_t id = keyToId(key);

    // A zero-length read returns the stored entry size.
    ssize_t length = nvs_read(&fs, id, nullptr, 0);
    if (length == -ENOENT) {
        return RM_E_STORAGE_KEY_NOT_FOUND;
    }
    if (length < 0) {
        logerr_ln("ERROR nvs_read(%s) failed, code %d", key.c_str(), static_cast<int>(length));
        return RM_E_STORAGE_READ_FAILED;
    }

    data.resize(length);
    if (length == 0) {
        return RM_E_NONE;
    }

    ssize_t rc = nvs_read(&fs, id, data.data(), data.size());
    if (rc < 0) {
        logerr_ln("ERROR nvs_read(%s) failed, code %d", key.c_str(), static_cast<int>(rc));
        return RM_E_STORAGE_READ_FAILED;
    }
    return RM_E_NONE;
}

int ZephyrNVSStorage::write(const std::string& key, const std::vector<byte>& data)
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    ssize_t rc = nvs_write(&fs, keyToId(key), data.data(), data.size());
    if (rc == -ENOSPC) {
        logerr_ln("ERROR nvs_write(%s): storage full", key.c_str());
        return RM_E_STORAGE_NOT_ENOUGH_SPACE;
    }
    if (rc < 0) {
        logerr_ln("ERROR nvs_write(%s) failed, code %d", key.c_str(), static_cast<int>(rc));
        return RM_E_STORAGE_WRITE_FAILED;
    }
    // rc == 0 means identical data was already stored — success either way.
    return RM_E_NONE;
}

int ZephyrNVSStorage::writeAndCommit(const std::string& key, const std::vector<byte>& data)
{
    // NVS writes are atomic and immediately persistent (see commit()).
    return write(key, data);
}

int ZephyrNVSStorage::commit()
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }
    // No-op: every nvs_write lands in flash before returning, so there is no
    // staged state to flush.
    return RM_E_NONE;
}

int ZephyrNVSStorage::remove(const std::string& key)
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    int rc = nvs_delete(&fs, keyToId(key));
    if (rc == -ENOENT) {
        return RM_E_STORAGE_KEY_NOT_FOUND;
    }
    if (rc < 0) {
        logerr_ln("ERROR nvs_delete(%s) failed, code %d", key.c_str(), rc);
        return RM_E_STORAGE_WRITE_FAILED;
    }
    return RM_E_NONE;
}

bool ZephyrNVSStorage::exists(const std::string& key)
{
    if (!initialized) {
        return false;
    }
    return nvs_read(&fs, keyToId(key), nullptr, 0) >= 0;
}

int ZephyrNVSStorage::clear()
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    int rc = nvs_clear(&fs);
    if (rc < 0) {
        logerr_ln("ERROR nvs_clear failed, code %d", rc);
        return RM_E_STORAGE_WRITE_FAILED;
    }

    // nvs_clear invalidates the mount; remount so storage stays usable.
    initialized = false;
    return begin();
}

size_t ZephyrNVSStorage::available()
{
    if (!initialized) {
        return 0;
    }

    ssize_t freeSpace = nvs_calc_free_space(&fs);
    return freeSpace < 0 ? 0 : static_cast<size_t>(freeSpace);
}

bool ZephyrNVSStorage::isFull()
{
    return available() == 0;
}

int ZephyrNVSStorage::defragment()
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }
    // No-op: NVS garbage-collects sectors automatically during writes.
    return RM_E_NONE;
}
