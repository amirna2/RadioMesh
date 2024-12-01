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

int EEPROMStorage::initializeStorageHeader()
{
   // Initialize with known good values
   StorageHeader header;
   header.magic = STORAGE_MAGIC;
   header.version = STORAGE_VERSION;
   header.numEntries = 0;

   // Write header and verify
   for (size_t i = 0; i < sizeof(StorageHeader); i++) {
      EEPROM.write(i, reinterpret_cast<const uint8_t*>(&header)[i]);
   }
   EEPROM.commit();

   // Verify header was written correctly
   StorageHeader verify;
   for (size_t i = 0; i < sizeof(StorageHeader); i++) {
      reinterpret_cast<uint8_t*>(&verify)[i] = EEPROM.read(i);
   }

   if (verify.magic != STORAGE_MAGIC || verify.version != STORAGE_VERSION) {
      return RM_E_STORAGE_WRITE_FAILED;
   }
   return RM_E_NONE;
}

int EEPROMStorage::readStorageHeader(StorageHeader& header)
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    // Read header from start of EEPROM
    for (size_t i = 0; i < sizeof(StorageHeader); i++) {
        reinterpret_cast<uint8_t*>(&header)[i] = EEPROM.read(i);
    }
    return RM_E_NONE;
}

int EEPROMStorage::writeStorageHeader(const StorageHeader& header)
{
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

bool EEPROMStorage::isStorageValid()
{
    StorageHeader header;
    if (readStorageHeader(header) != RM_E_NONE) {
        return false;
    }
    return header.magic == STORAGE_MAGIC && header.version == STORAGE_VERSION;
}


int EEPROMStorage::begin()
{
   if (!EEPROM.begin(storageParams.size)) {
      logerr_ln("Failed to initialize EEPROM");
      return RM_E_STORAGE_SETUP;
   }

   initialized = true;

   // Check if storage is already initialized
   if (!isStorageValid()) {
      logdbg_ln("Initializing storage header");
      return initializeStorageHeader();
   } else {
      logdbg_ln("Storage header already initialized");
   }

   return RM_E_NONE;
}

int EEPROMStorage::findEntry(const std::string& key, size_t& offset)
{
   StorageHeader header;
   if (readStorageHeader(header) != RM_E_NONE) {
      logerr_ln("Failed to read storage header");
      return RM_E_STORAGE_NOT_INIT;
   }

   size_t currentOffset = sizeof(StorageHeader);
   for (size_t i = 0; i < header.numEntries; i++) {
      EntryHeader entryHeader;
      // Read entry header
      for (size_t j = 0; j < sizeof(EntryHeader); j++) {
         reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(currentOffset + j);
      }

      currentOffset += sizeof(EntryHeader);

      // Read and compare key
      std::string storedKey;
      storedKey.resize(entryHeader.keyLength);
      for (size_t j = 0; j < entryHeader.keyLength; j++) {
         storedKey[j] = EEPROM.read(currentOffset + j);
      }

      if (storedKey == key && (entryHeader.flags & ENTRY_VALID_FLAG)) {
         offset = currentOffset;
         logdbg_ln("Found key: %s at offset: %d", key.c_str(), offset);
         return RM_E_NONE;
      }

      // Skip to next entry
      currentOffset += entryHeader.keyLength + entryHeader.dataLength;
   }
   logdbg_ln("Key: %s not found", key.c_str());
   return RM_E_STORAGE_KEY_NOT_FOUND;
}

int EEPROMStorage::writeEntry(size_t offset, const std::string& key, const std::vector<byte>& data)
{
   EntryHeader entryHeader;
   entryHeader.keyLength = key.length();
   entryHeader.dataLength = data.size();
   entryHeader.flags = ENTRY_VALID_FLAG;

   // Write entry header
   size_t currentOffset = offset;
   for (size_t i = 0; i < sizeof(EntryHeader); i++) {
      EEPROM.write(currentOffset + i, reinterpret_cast<const uint8_t*>(&entryHeader)[i]);
   }
   currentOffset += sizeof(EntryHeader);

   // Write key
   for (size_t i = 0; i < key.length(); i++) {
      EEPROM.write(currentOffset + i, key[i]);
   }
   currentOffset += key.length();

   // Write data
   for (size_t i = 0; i < data.size(); i++) {
      EEPROM.write(currentOffset + i, data[i]);
   }

   EEPROM.commit();
   return RM_E_NONE;
}

int EEPROMStorage::write(const std::string& key, const std::vector<byte>& data)
{
   logdbg_ln("Writing key: %s", key.c_str());
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }

   if (key.empty()) {
      logerr_ln("Invalid parameter: key is empty");
      return RM_E_INVALID_PARAM;
   }
   if (data.empty()) {
      logerr_ln("Invalid parameter: data is empty");
      return RM_E_INVALID_PARAM;
   }
   if (key.length() > 255) {
      logerr_ln("Invalid parameter: key is too long");
      return RM_E_INVALID_LENGTH;
   }

   logdbg_ln("Writing key: %s", key.c_str());
   logdbg_ln("Data size: %d", data.size());
   logdbg_ln("Available space: %d", available());

   // check if enough space is available
   if (available() < key.length() + data.size() + sizeof(EntryHeader)) {
      logerr_ln("Not enough space left");
      return RM_E_STORAGE_NOT_ENOUGH_SPACE;
   }

   // Read storage header for entry count
   StorageHeader storageHeader;
   if (readStorageHeader(storageHeader) != RM_E_NONE) {
      return RM_E_STORAGE_READ_FAILED;
   }
   logdbg_ln("Current entries: %d", storageHeader.numEntries);

   // Calculate offset for new entry - skip past existing entries
   size_t addr = sizeof(StorageHeader);
   for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
      EntryHeader entryHeader;
      // Read existing entry header
      for (size_t j = 0; j < sizeof(EntryHeader); j++) {
         reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(addr + j);
      }
      // Skip this entry (header + key + data)
      addr += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
   }

   logdbg_ln("Writing at offset: %d", addr);

   // Write new entry header
   EntryHeader newEntry;
   newEntry.keyLength = key.length();
   newEntry.dataLength = data.size();
   newEntry.flags = ENTRY_VALID_FLAG;

   for (size_t i = 0; i < sizeof(EntryHeader); i++) {
      EEPROM.write(addr + i, reinterpret_cast<uint8_t*>(&newEntry)[i]);
   }
   addr += sizeof(EntryHeader);

   // Write key
   for (size_t i = 0; i < key.length(); i++) {
      EEPROM.write(addr + i, key[i]);
   }
   addr += key.length();

   // Write data
   for (size_t i = 0; i < data.size(); i++) {
      EEPROM.write(addr + i, data[i]);
   }

   // Update storage header with new entry count
   storageHeader.numEntries++;
   writeStorageHeader(storageHeader);

   EEPROM.commit();

   logdbg_ln("Write successful for key: %s", key.c_str());

   return RM_E_NONE;
}

bool EEPROMStorage::exists(const std::string& key)
{

   logdbg_ln("Checking key: %s exists", key.c_str());
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return false;
   }

   if (key.empty()) {
      logerr_ln("Invalid parameter: key is empty");
      return RM_E_INVALID_PARAM;
   }

   size_t addr = sizeof(StorageHeader);  // Skip header

   // Read entry header
   EntryHeader entryHeader;
   for (size_t i = 0; i < sizeof(EntryHeader); i++) {
      reinterpret_cast<uint8_t*>(&entryHeader)[i] = EEPROM.read(addr + i);
   }
   addr += sizeof(EntryHeader);

   logdbg_ln("Number of entries: %d", getEntryCount());

   // Read key and compare
   std::string storedKey;
   storedKey.resize(entryHeader.keyLength);
   for (size_t i = 0; i < entryHeader.keyLength; i++) {
      storedKey[i] = EEPROM.read(addr + i);
   }

   if ((entryHeader.flags & ENTRY_VALID_FLAG) && (storedKey == key)) {
      logdbg_ln("Key found: %s", key.c_str());
      return true;
   }

   logdbg_ln("Key not found: %s", key.c_str());
   return false;
}

int EEPROMStorage::read(const std::string& key, std::vector<byte>& data)
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }

   if (key.empty()) {
      logerr_ln("Invalid parameter: key is empty");
      return RM_E_INVALID_PARAM;
   }

   // Read storage header for entry count
   StorageHeader storageHeader;
   if (readStorageHeader(storageHeader) != RM_E_NONE) {
      logerr_ln("Failed to read storage header");
      return RM_E_STORAGE_READ_FAILED;
   }

   // Search through all entries
   size_t addr = sizeof(StorageHeader);
   for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
      // Read entry header
      EntryHeader entryHeader;
      for (size_t j = 0; j < sizeof(EntryHeader); j++) {
         reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(addr + j);
      }
      addr += sizeof(EntryHeader);

      // Read key and compare
      std::string storedKey;
      storedKey.resize(entryHeader.keyLength);
      for (size_t j = 0; j < entryHeader.keyLength; j++) {
         storedKey[j] = EEPROM.read(addr + j);
      }

      if (storedKey == key && (entryHeader.flags & ENTRY_VALID_FLAG)) {
         // Found matching key - read data
         addr += entryHeader.keyLength;  // Skip to data
         data.resize(entryHeader.dataLength);
         for (size_t j = 0; j < entryHeader.dataLength; j++) {
               data[j] = EEPROM.read(addr + j);
         }
         logdbg_ln("Read successful for key: %s", key.c_str());
         return RM_E_NONE;
      }

      // Skip to next entry
      addr += entryHeader.keyLength + entryHeader.dataLength;
   }

   logerr_ln("Key not found: %s", key.c_str());
   return RM_E_STORAGE_KEY_NOT_FOUND;
}

int EEPROMStorage::remove(const std::string& key)
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }

   if (key.empty()) {
      logerr_ln("Invalid parameter: key is empty");
      return RM_E_INVALID_PARAM;
   }

   size_t addr = sizeof(StorageHeader);

   // Read entry header
   EntryHeader entryHeader;
   for (size_t i = 0; i < sizeof(EntryHeader); i++) {
      reinterpret_cast<uint8_t*>(&entryHeader)[i] = EEPROM.read(addr + i);
   }
   addr += sizeof(EntryHeader);

   // Read and verify key
   std::string storedKey;
   storedKey.resize(entryHeader.keyLength);
   for (size_t i = 0; i < entryHeader.keyLength; i++) {
      storedKey[i] = EEPROM.read(addr + i);
   }

   if (storedKey != key || !(entryHeader.flags & ENTRY_VALID_FLAG)) {
      logerr_ln("Key not found: %s", key.c_str());
      return RM_E_STORAGE_KEY_NOT_FOUND;
   }

   // Found the key - mark entry as invalid by clearing valid flag
   addr = sizeof(StorageHeader);  // Go back to entry header
   entryHeader.flags &= ~ENTRY_VALID_FLAG;  // Clear valid flag

   // Write updated header back
   for (size_t i = 0; i < sizeof(EntryHeader); i++) {
         EEPROM.write(addr + i, reinterpret_cast<uint8_t*>(&entryHeader)[i]);
   }
   EEPROM.commit();
   logdbg_ln("Key removed: %s", key.c_str());
   return RM_E_NONE;
}

int EEPROMStorage::clear()
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }
   // Clear all EEPROM space
   for (size_t i = 0; i < storageParams.size; i++) {
      EEPROM.write(i, 0xFF);
   }
   EEPROM.commit();

   // Reinitialize storage header after clear
   return initializeStorageHeader();
}

int EEPROMStorage::end()
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }
   initialized = false;
   EEPROM.end();
   return RM_E_NONE;
}

size_t EEPROMStorage::available()
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return 0;
   }
   return storageParams.size;
}

bool EEPROMStorage::isFull()
{
   return !initialized || available() == 0;
}

int EEPROMStorage::getEntryCount()
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }

   StorageHeader header;
   if (readStorageHeader(header) != RM_E_NONE) {
      logerr_ln("Failed to read storage header");
      return RM_E_STORAGE_READ_FAILED;
   }

   return header.numEntries;
}
