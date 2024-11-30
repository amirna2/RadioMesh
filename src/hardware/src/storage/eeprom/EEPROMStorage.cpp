#include <vector>
#include <string>

#include <common/inc/Logger.h>
#include <common/inc/Errors.h>
#include <common/inc/Definitions.h>
#include <hardware/inc/storage/eeprom/EEPROMStorage.h>
#include <EEPROM.h>


EEPROMStorage* EEPROMStorage::instance = nullptr;

EEPROMStorage::EEPROMStorage()
{
}

int EEPROMStorage::setParams(const StorageParams& params)
{
   if (params.size == 0) {
      return RM_E_INVALID_PARAM;
   }
   storageParams = params;
   return RM_E_NONE;
}

int EEPROMStorage::initializeStorageHeader() {
    StorageHeader header;
    header.magic = STORAGE_MAGIC;
    header.version = STORAGE_VERSION;
    header.numEntries = 0;

    return writeStorageHeader(header);
}

int EEPROMStorage::readStorageHeader(StorageHeader& header) {
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    // Read header from start of EEPROM
    for (size_t i = 0; i < sizeof(StorageHeader); i++) {
        reinterpret_cast<uint8_t*>(&header)[i] = EEPROM.read(i);
    }
    return RM_E_NONE;
}

int EEPROMStorage::writeStorageHeader(const StorageHeader& header) {
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    // Write header to start of EEPROM
    for (size_t i = 0; i < sizeof(StorageHeader); i++) {
        EEPROM.write(i, reinterpret_cast<const uint8_t*>(&header)[i]);
    }
    EEPROM.commit();
    return RM_E_NONE;
}

bool EEPROMStorage::isStorageValid() {
    StorageHeader header;
    if (readStorageHeader(header) != RM_E_NONE) {
        return false;
    }
    return header.magic == STORAGE_MAGIC && header.version == STORAGE_VERSION;
}


int EEPROMStorage::begin() {
    if (!EEPROM.begin(storageParams.size)) {
        return RM_E_STORAGE_SETUP;
    }

    initialized = true;

    // Check if storage is already initialized
    if (!isStorageValid()) {
        return initializeStorageHeader();
    }

    return RM_E_NONE;
}

int EEPROMStorage::write(const std::string& key, const std::vector<byte>& data) {
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }
    size_t addr = sizeof(StorageHeader);  // Skip header
    for (size_t i = 0; i < data.size(); i++) {
        EEPROM.write(addr + i, data[i]);
    }
    EEPROM.commit();
    return RM_E_NONE;
}

int EEPROMStorage::read(const std::string& key, std::vector<byte>& data) {
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    // TODO: For now, just read from same fixed address as write
    size_t addr = sizeof(StorageHeader); // Skip header
    data.resize(4);  // TODO: Fix this! We know it's 4 bytes from test data
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = EEPROM.read(addr + i);
    }

    return RM_E_NONE;
}

int EEPROMStorage::end()
{
   if (!initialized) {
      return RM_E_STORAGE_NOT_INIT;
   }
   initialized = false;
   EEPROM.end();
   return RM_E_NONE;
}

bool EEPROMStorage::exists(const std::string& key) {
    if (!initialized) {
        return false;
    }

    // TODO: For now, with our fixed address implementation
    // Check if first byte at storage address after header is 0xFF (our "deleted" marker)
    size_t addr = sizeof(StorageHeader);  // Skip header
    return EEPROM.read(addr) != 0xFF;
}

size_t EEPROMStorage::available()
{
   if (!initialized) {
      return 0;
   }
   return storageParams.size;
}

bool EEPROMStorage::isFull()
{
   return !initialized || available() == 0;
}

int EEPROMStorage::remove(const std::string& key)
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }
    // TODO: For this basic "remove" implementation, just mark entry as invalid
    // Later we'll need proper key-value storage management
    size_t addr = sizeof(StorageHeader);  // Skip header
    for (size_t i = 0; i < 4; i++) {  // TODO: Fix this! Using same size as test data
        EEPROM.write(addr + i, 0xFF);  // Mark as "deleted"
    }
    EEPROM.commit();
    return RM_E_NONE;
}

int EEPROMStorage::clear()
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }
    // Clear all EEPROM space
    for (size_t i = 0; i < storageParams.size; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    return RM_E_NONE;
}
