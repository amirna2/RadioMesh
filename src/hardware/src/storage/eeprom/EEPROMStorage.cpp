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

int EEPROMStorage::setParams(const ByteStorageParams& params)
{
   if (params.size == 0 || params.size > EEPROM_STORAGE_MAX_SIZE) {
      return RM_E_STORAGE_INVALID_SIZE;
   }
   storageParams = params;
   return RM_E_NONE;
}

void EEPROMStorage::initializeStorageHeader()
{
   // Initialize with known good values
   StorageHeader header;
   header.magic = STORAGE_MAGIC;
   header.version = STORAGE_VERSION;
   header.numEntries = 0;

   // Write header
   for (size_t i = 0; i < sizeof(StorageHeader); i++) {
      EEPROM.write(i, reinterpret_cast<const uint8_t*>(&header)[i]);
   }
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

    // Write header to start of EEPROM without committing
    for (size_t i = 0; i < sizeof(StorageHeader); i++) {
        EEPROM.write(i, reinterpret_cast<const uint8_t*>(&header)[i]);
    }
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
   if (initialized) {
      logerr_ln("Storage already initialized");
      return RM_E_STORAGE_SETUP;
   }
   if (storageParams.size == 0) {
      logerr_ln("Invalid storage parameters");
      return RM_E_INVALID_PARAM;
   }

#ifdef ESP32
   if (!EEPROM.begin(storageParams.size)) {
      logerr_ln("Failed to initialize EEPROM");
      return RM_E_STORAGE_SETUP;
   }
#else
   EEPROM.begin(storageParams.size);
#endif

   initialized = true;

   // initialize storage header if not already done
   if (!isStorageValid()) {
      logdbg_ln("Initializing storage header");
      initializeStorageHeader();
      if (!commit()) {
         logerr_ln("Failed to commit storage header");
         return RM_E_STORAGE_SETUP;
      }
   } else {
      logdbg_ln("Storage header already initialized");
   }

   return RM_E_NONE;
}

int EEPROMStorage::end()
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }

   EEPROM.end();
   initialized = false;
   return RM_E_NONE;
}


#if 0

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

   logdbg_ln("Write successful for key: %s", key.c_str());

   return RM_E_NONE;
}
#endif

int EEPROMStorage::write(const std::string& key, const std::vector<byte>& data) {
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
    logdbg_ln("Space needed: %d", key.length() + data.size() + sizeof(EntryHeader));

    // Check space availability

    if (available() < key.length() + data.size() + sizeof(EntryHeader)) {
        logerr_ln("Not enough space left");
        return RM_E_STORAGE_NOT_ENOUGH_SPACE;
    }

    StorageHeader storageHeader;
    if (readStorageHeader(storageHeader) != RM_E_NONE) {
        return RM_E_STORAGE_READ_FAILED;
    }

    // Search for existing key
    size_t addr = sizeof(StorageHeader);
    size_t writeAddr = addr;
    bool foundExisting = false;

    for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
        EntryHeader entryHeader;
        size_t currentEntryStart = addr;

        // Read entry header
        for (size_t j = 0; j < sizeof(EntryHeader); j++) {
            reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(addr + j);
        }

        // Read key if entry is valid
        if (entryHeader.flags & ENTRY_VALID_FLAG) {
            std::string storedKey;
            storedKey.resize(entryHeader.keyLength);
            for (size_t j = 0; j < entryHeader.keyLength; j++) {
                storedKey[j] = EEPROM.read(addr + sizeof(EntryHeader) + j);
            }

            if (storedKey == key) {
                // Invalidate existing entry
                entryHeader.flags &= ~ENTRY_VALID_FLAG;
                for (size_t j = 0; j < sizeof(EntryHeader); j++) {
                    EEPROM.write(currentEntryStart + j,
                        reinterpret_cast<uint8_t*>(&entryHeader)[j]);
                }
                foundExisting = true;
                storageHeader.numEntries--;
                writeStorageHeader(storageHeader);
                break;
            }
        }

        writeAddr = addr + sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
        addr = writeAddr;
    }

    // Write new entry
    EntryHeader newEntry;
    newEntry.keyLength = key.length();
    newEntry.dataLength = data.size();
    newEntry.flags = ENTRY_VALID_FLAG;

    // Write entry header
    for (size_t i = 0; i < sizeof(EntryHeader); i++) {
        EEPROM.write(writeAddr + i, reinterpret_cast<uint8_t*>(&newEntry)[i]);
    }
    writeAddr += sizeof(EntryHeader);

    // Write key
    for (size_t i = 0; i < key.length(); i++) {
        EEPROM.write(writeAddr + i, key[i]);
    }
    writeAddr += key.length();

    // Write data
    for (size_t i = 0; i < data.size(); i++) {
        EEPROM.write(writeAddr + i, data[i]);
    }

    // Update storage header
    storageHeader.numEntries++;
    writeStorageHeader(storageHeader);

    logdbg_ln("Write complete for key: %s", key.c_str());
    return RM_E_NONE;
}

int EEPROMStorage::writeAndCommit(const std::string& key, const std::vector<byte>& data)
{
   int rc = write(key, data);
   if (rc != RM_E_NONE) {
      logerr_ln("Failed to write data");
      return rc;
   }
   rc = commit();
   if (rc != RM_E_NONE) {
      logerr_ln("Failed to commit data");
      return rc;
   }
   return RM_E_NONE;
}

bool EEPROMStorage::exists(const std::string& key)
{
   logdbg_ln("Checking if key: %s exists", key.c_str());
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return false;
   }

   if (key.empty()) {
      logerr_ln("Invalid parameter: key is empty");
      return false;
   }

   // Read storage header for entry count
   StorageHeader storageHeader;
   if (readStorageHeader(storageHeader) != RM_E_NONE) {
      return false;
   }

   logdbg_ln("Number of entries: %d", storageHeader.numEntries);

   // Search through all entries
   size_t addr = sizeof(StorageHeader);
   for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
      // Read entry header
      EntryHeader entryHeader;
      for (size_t j = 0; j < sizeof(EntryHeader); j++) {
         reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(addr + j);
      }

      // Only check valid entries
      if (entryHeader.flags & ENTRY_VALID_FLAG) {
         // Read key and compare
         std::string storedKey;
         storedKey.resize(entryHeader.keyLength);
         for (size_t j = 0; j < entryHeader.keyLength; j++) {
            storedKey[j] = EEPROM.read(addr + sizeof(EntryHeader) + j);
         }

         if (storedKey == key) {
            logdbg_ln("Key found: %s", key.c_str());
            return true;
         }
      }

      // Move to next entry
      addr += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
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

   StorageHeader storageHeader;
   if (readStorageHeader(storageHeader) != RM_E_NONE) {
      return RM_E_STORAGE_READ_FAILED;
   }

   size_t addr = sizeof(StorageHeader);
   for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
      // Read entry header
      EntryHeader entryHeader;
      for (size_t j = 0; j < sizeof(EntryHeader); j++) {
         reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(addr + j);
      }

      if (entryHeader.flags & ENTRY_VALID_FLAG) {
         // Read key and compare
         std::string storedKey;
         storedKey.resize(entryHeader.keyLength);
         for (size_t j = 0; j < entryHeader.keyLength; j++) {
            storedKey[j] = EEPROM.read(addr + sizeof(EntryHeader) + j);
         }

         if (storedKey == key) {
            // Found it - invalidate entry
            entryHeader.flags &= ~ENTRY_VALID_FLAG;
            // Write back updated header
            for (size_t j = 0; j < sizeof(EntryHeader); j++) {
               EEPROM.write(addr + j, reinterpret_cast<uint8_t*>(&entryHeader)[j]);
            }
            logdbg_ln("Key removed: %s", key.c_str());
            return RM_E_NONE;
         }
      }

      // Move to next entry
      addr += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
   }

   logerr_ln("Key not found: %s", key.c_str());
   return RM_E_STORAGE_KEY_NOT_FOUND;
}

int EEPROMStorage::defragment()
{
    if (!initialized) {
        return RM_E_STORAGE_NOT_INIT;
    }

    StorageHeader storageHeader;
    if (readStorageHeader(storageHeader) != RM_E_NONE) {
        return RM_E_STORAGE_READ_FAILED;
    }

    // Capture initial state
    size_t initialEntries = storageHeader.numEntries;
    size_t initialSpace = available();

    size_t readAddr = sizeof(StorageHeader);
    size_t writeAddr = sizeof(StorageHeader);
    uint16_t validEntries = 0;

    // Iterate through all entries
    for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
        EntryHeader entryHeader;

        // Read entry header
        for (size_t j = 0; j < sizeof(EntryHeader); j++) {
            reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(readAddr + j);
        }

        if (entryHeader.flags & ENTRY_VALID_FLAG) {
            if (readAddr != writeAddr) {
                // Move entry header
                for (size_t j = 0; j < sizeof(EntryHeader); j++) {
                    EEPROM.write(writeAddr + j, reinterpret_cast<uint8_t*>(&entryHeader)[j]);
                }

                // Move key and data
                size_t totalSize = entryHeader.keyLength + entryHeader.dataLength;
                for (size_t j = 0; j < totalSize; j++) {
                    EEPROM.write(writeAddr + sizeof(EntryHeader) + j,
                               EEPROM.read(readAddr + sizeof(EntryHeader) + j));
                }
            }
            writeAddr += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
            validEntries++;
        }
        readAddr += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
    }

    // Update storage header
    storageHeader.numEntries = validEntries;
    writeStorageHeader(storageHeader);

    // Log defragmentation stats
    size_t finalSpace = available();
    size_t reclaimedSpace = finalSpace - initialSpace;
    size_t removedEntries = initialEntries - validEntries;

    loginfo_ln("Defrag Stats:");
    loginfo_ln("- Entries: %d -> %d (removed %d)",
               initialEntries, validEntries, removedEntries);
    loginfo_ln("- Available Space: %d -> %d bytes (reclaimed %d)",
               initialSpace, finalSpace, reclaimedSpace);
    loginfo_ln("- Fragmentation: %.1f%%",
               (100.0f * reclaimedSpace) / storageParams.size);

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
   // Reinitialize storage header after clear and commit
   initializeStorageHeader();
   return commit();
}

size_t EEPROMStorage::available() {
    if (!initialized) {
        logerr_ln("Storage not initialized");
        return 0;
    }

    StorageHeader storageHeader;
    if (readStorageHeader(storageHeader) != RM_E_NONE) {
        return 0;
    }

    // Calculate used space including invalid entries
    size_t usedSpace = sizeof(StorageHeader);
    size_t addr = sizeof(StorageHeader);

    for (uint16_t i = 0; i < storageHeader.numEntries; i++) {
        EntryHeader entryHeader;
        for (size_t j = 0; j < sizeof(EntryHeader); j++) {
            reinterpret_cast<uint8_t*>(&entryHeader)[j] = EEPROM.read(addr + j);
        }
        // Count space regardless of valid flag
        usedSpace += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
        addr += sizeof(EntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
    }

    return storageParams.size - usedSpace;
}

bool EEPROMStorage::isFull() {
    if (!initialized) {
        return true;
    }
    return available() == 0;
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

int EEPROMStorage::commit()
{
   if (!initialized) {
      logerr_ln("Storage not initialized");
      return RM_E_STORAGE_NOT_INIT;
   }
   if (!EEPROM.commit()) {
      logerr_ln("Failed to commit EEPROM");
      return RM_E_STORAGE_WRITE_FAILED;
   }
   return RM_E_NONE;
}
