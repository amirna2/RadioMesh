#pragma once

#include <framework/interfaces/IStorage.h>

class EEPROMStorage : public IStorage
{
public:
    /**
     * @brief Get the instance of the EEPROMStorage.
     * @returns A pointer to the instance of the EEPROMStorage.
    **/
    static EEPROMStorage *getInstance()
    {
        if (!instance) {
            instance = new EEPROMStorage();
        }
        return instance;
    }

    virtual ~EEPROMStorage() = default;

    // IStorage interface
    int read(const std::string& key, std::vector<byte>& data) override;
    int write(const std::string& key, const std::vector<byte>& data) override;
    int remove(const std::string& key) override;
    bool exists(const std::string& key) override;

    int begin() override;
    int end() override;
    int clear() override;
    size_t available() override;
    bool isFull() override;

    // EEPROMStorage specific methods
    int setParams(const StorageParams& params);

private:
    static EEPROMStorage *instance;
    EEPROMStorage();
    EEPROMStorage(const EEPROMStorage &) = delete;
    void operator=(const EEPROMStorage &) = delete;
    bool initialized = false;
    StorageParams storageParams;

    static constexpr uint32_t STORAGE_MAGIC = 0x524D5354;  // "RMST" in hex
    static constexpr uint16_t STORAGE_VERSION = 1;

    // Storage header - starts at EEPROM address 0
    struct StorageHeader
    {
        uint32_t magic;      // Magic number to validate EEPROM contents
        uint16_t version;    // Storage format version
        uint16_t numEntries; // Current number of stored key-value pairs
    };

    // Entry header - precedes each key-value pair
    struct EntryHeader
    {
        uint16_t keyLength;   // Length of the key string
        uint16_t dataLength;  // Length of data
        uint8_t flags;        // Bit flags (bit 0: valid/deleted)
    };

    // Add helper method declarations
    int initializeStorageHeader();
    int readStorageHeader(StorageHeader& header);
    int writeStorageHeader(const StorageHeader& header);
    bool isStorageValid();
};
