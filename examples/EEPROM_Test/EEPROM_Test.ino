#include <Arduino.h>
#include <RadioMesh.h>

IDevice *device = nullptr;
IByteStorage *storage = nullptr;
DeviceBuilder builder;

#define DEVICE_ID {0x01, 0x02, 0x03, 0x04}

void setup() {

   ByteStorageParams params;
    params.size = 1024;

    device = builder.start()
                   .withStorage(params)
                   .build("test-device", DEVICE_ID, MeshDeviceType::STANDARD);

    if (device == nullptr) {
        Serial.println("ERROR: device is null");
        return;
    }

    storage = device->getByteStorage();
    if (storage == nullptr) {
        loginfo_ln("ERROR: storage is null");
        return;
    }

    // Initialize storage
    Serial.println("Initializing storage...");
    int rc = storage->begin();
    loginfo("Storage init result: %d\n", rc);
    if (rc != RM_E_NONE) {
        loginfo_ln("Failed to initialize storage");
        return;
    }
    std::vector<byte> data1 = {1, 2};
    std::vector<byte> data2 = {3, 4};
    std::vector<byte> readData;

    loginfo_ln("Writing key1...");
    rc = storage->write("key1", data1);
    loginfo_ln("Write key1 result: %d\n", rc);

    loginfo_ln("Writing key2...");
    rc = storage->write("key2", data2);
    loginfo_ln("Write key2 result: %d\n", rc);

    loginfo_ln("Reading key1...");
    rc = storage->read("key1", readData);
    loginfo_ln("Read key1 result: %d\n", rc);

    if (rc == RM_E_NONE && readData.size() > 0) {
        loginfo_ln("key1 data size: %d\n", readData.size());
        loginfo_ln("key1 data: %d,%d\n", readData[0], readData[1]);
    } else {
        loginfo_ln("Failed to read key1 or data empty");
    }

    // Clear vector before next read
    readData.clear();

    loginfo_ln("Reading key2...");
    rc = storage->read("key2", readData);
    loginfo_ln("Read key2 result: %d\n", rc);

    if (rc == RM_E_NONE && readData.size() > 0) {
        loginfo_ln("key2 data size: %d\n", readData.size());
        loginfo_ln("key2 data: %d,%d\n", readData[0], readData[1]);
    } else {
        loginfo_ln("Failed to read key2 or data empty");
    }

    loginfo_ln("Test complete");
}

void loop() {
    delay(1000);
}
