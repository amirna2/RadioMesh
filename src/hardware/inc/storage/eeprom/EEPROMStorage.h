#pragma once

#include <framework/interfaces/IByteStorage.h>

/*
EEPROM Memory Layout
--------------------------------------------------
Offset  | Size    | Description
--------------------------------------------------
0x0000  |         | StorageHeader
        | 4 bytes | - magic (0x524D5354)
        | 2 bytes | - version
        | 2 bytes | - numEntries
--------------------------------------------------
0x0008  |         | First Entry
        |         | EntryHeader
        | 2 bytes | - keyLength
        | 2 bytes | - dataLength
        | 1 byte  | - flags
        | N bytes | Key data (length=keyLength)
        | M bytes | Value data (length=dataLength)
--------------------------------------------------
...     |         | Next Entry
        |         | (Same structure repeats)
--------------------------------------------------
*/

/*
EXAMPLE EEPROM CONTENTS: (1 entry) - "test" -> [1,2,3,4]

Address | Content             | Description
--------+---------------------+-------------
0x0000  | 54 53 4D 52         | Magic "RMST"
0x0004  | 01 00               | Version 1
0x0006  | 01 00               | 1 entry
--------+---------------------+-------------
0x0008  | 04 00               | keyLength=4
0x000A  | 04 00               | dataLength=4
0x000C  | 01                  | flags (valid)
0x000D  | 74 65 73 74         | "test"
0x0011  | 01 02 03 04         | [1,2,3,4]
--------+---------------------+-------------
*/

/// @brief EEPROM Maximum size
const uint32_t EEPROM_STORAGE_MAX_SIZE = 1024;

class EEPROMStorage : public IByteStorage
{
public:
    /**
     * @brief Get the instance of the EEPROMStorage.
     * @returns A pointer to the instance of the EEPROMStorage.
     **/
    static EEPROMStorage* getInstance()
    {
        if (!instance) {
            instance = new EEPROMStorage();
        }
        return instance;
    }

    virtual ~EEPROMStorage() = default;

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
    // EEPROMStorage specific methods
    int setParams(const ByteStorageParams& params);
    int getEntryCount();

private:
    static EEPROMStorage* instance;
    EEPROMStorage();
    EEPROMStorage(const EEPROMStorage&) = delete;
    void operator=(const EEPROMStorage&) = delete;
    bool initialized = false;
    ByteStorageParams storageParams;

    static constexpr uint32_t STORAGE_MAGIC = 0x524D5354;
    static constexpr uint16_t STORAGE_VERSION = 1;
    static constexpr uint8_t ENTRY_VALID_FLAG = 0x01;

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
        uint16_t keyLength;  // Length of the key string
        uint16_t dataLength; // Length of data
        uint8_t flags;       // Bit flags (bit 0: valid/deleted)
    };

    // Add helper method declarations
    void initializeStorageHeader();
    int readStorageHeader(StorageHeader& header);
    int writeStorageHeader(const StorageHeader& header);
    bool isStorageValid();

    // list of reserved keys (used internally)
    const std::vector<std::string> reservedKeys = {"is", "mc", "sk"};
};
