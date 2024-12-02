#include <Arduino.h>
#include <RadioMesh.h>

IDevice *device = nullptr;
IByteStorage *storage = nullptr;
DeviceBuilder builder;

#define DEVICE_ID {0x01, 0x02, 0x03, 0x04}
#define MIN_STORAGE_THRESHOLD 30  // bytes

void setup() {
    // Initialize with 128 bytes
    ByteStorageParams params;
    params.size = 128;

    device = builder.start()
                   .withStorage(params)
                   .build("test-device", DEVICE_ID, MeshDeviceType::STANDARD);

    if (device == nullptr) {
        logerr_ln("ERROR: device is null");
        return;
    }

    storage = device->getByteStorage();
    if (storage == nullptr) {
        loginfo_ln("ERROR: storage is null");
        return;
    }

    // Initialize storage
    int rc = storage->begin();
    if (rc != RM_E_NONE) {
        loginfo_ln("Failed to initialize storage");
        return;
    }

    // Check if key1 exists
    loginfo_ln("Checking if key1 exists...");
    if (storage->exists("key1")) {
        std::vector<byte> readData;
        rc = storage->read("key1", readData);
        if (rc == RM_E_NONE && readData.size() > 0) {
            loginfo_ln("key1 data found: ");
            for (byte b : readData) {
                loginfo_ln("%d ", b);
            }
        }
        rc = storage->remove("key1");
        if (rc == RM_E_NONE) {
            loginfo_ln("key1 removed");
        } else {
            loginfo_ln("Failed to remove key1");
        }
    } else {
        loginfo_ln("key1 was not found");
    }

    // Write "hello" to key1
    std::vector<byte> data = {'h', 'e', 'l', 'l', 'o'};
    loginfo_ln("Writing 'hello' to key1...");
    rc = storage->write("key1", data);
    loginfo_ln("Write result: %d", rc);

    // Commit changes
    loginfo_ln("Committing storage...");
    rc = storage->commit();
    if (rc != RM_E_NONE) {
        loginfo_ln("Failed to commit storage");
        return;
    }


    int space = storage->available();
    if (space < MIN_STORAGE_THRESHOLD) {
        logwarn_ln("Storage space low (%d bytes)..deframgenting...", space);
        rc = storage->defragment();
        if (rc != RM_E_NONE) {
            loginfo_ln("Failed to defragment storage");
            return;
        }
        loginfo_ln("Defragmentation complete");
        loginfo_ln("Storage space available: %d bytes", storage->available());
    } else {
        loginfo_ln("Storage space available: %d bytes", space);
    }

    // Close storage
    loginfo_ln("Closing storage...");
    rc = storage->end();
    if (rc != RM_E_NONE) {
        loginfo_ln("Failed to close storage");
        return;
    }

    loginfo_ln("Test complete");
}

void loop() {
    delay(1000);
}
