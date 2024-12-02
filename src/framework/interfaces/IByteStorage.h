#pragma once

#include <vector>
#include <common/inc/Options.h>

/**
 * @class IByteStorage
 * @brief This class is an interface for a byte storage.
 *
 * It provides the structure for reading, writing, and managing byte storage.
 * Use this interface to storage data on the device in storage such as EEPROM.
 */
class IByteStorage {
public:
    virtual ~IByteStorage() = default;

    /**
     * @brief Read data from storage.
     * @param key The key to read data from.
     * @param data The data read from storage.
     * @returns RM_E_NONE if the data was successfully read, an error code otherwise.
     */
    virtual int read(const std::string& key, std::vector<byte>& data) = 0;

    /**
     * @brief Write data to storage.
     * @attention The write operation is not persisted until commit() is called.
     * @param key The key to write data to.
     * @param data The data to write to storage.
     * @returns RM_E_NONE if the data was successfully written, an error code otherwise.
     */
    virtual int write(const std::string& key, const std::vector<byte>& data) = 0;

    /**
     * @brief Write data to storage and commit the write operation.
     * @param key The key to write data to.
     * @param data The data to write to storage.
     * @returns RM_E_NONE if the data was successfully written and committed, an error code otherwise.
     */
    virtual int writeAndCommit(const std::string& key, const std::vector<byte>& data) = 0;

    /**
     * @brief Commit the write operation.
     * @returns RM_E_NONE if the write operation was successfully committed, an error code otherwise.
     */
    virtual int commit() = 0;
    /**
     * @brief Remove data from storage.
     * @param key The key to remove data from.
     * @returns RM_E_NONE if the data was successfully removed, an error code otherwise.
     */
    virtual int remove(const std::string& key) = 0;
    virtual bool exists(const std::string& key) = 0;

    // Storage management
    virtual int begin() = 0;
    virtual int end() = 0;
    virtual int clear() = 0;
    virtual size_t available() = 0;
    virtual bool isFull() = 0;
};
