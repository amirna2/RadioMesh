#include <Arduino.h>

#include <common/inc/platform/RadioMeshPlatform.h>
#include <hardware/inc/radio/LoraRadio.h>
#include <hardware/inc/storage/eeprom/EEPROMStorage.h>

#ifdef ESP32
#include <esp_system.h>
#endif

// Analog-noise entropy source (moved verbatim from RadioMeshUtils::simpleRNG
// in common/utils/Utils.cpp - platform code has no place in the portable core).
static uint8_t simpleRNG(uint16_t size)
{
    uint8_t val;
#if defined(ESP32)
    uint8_t analogPin = A0;
#else
    uint8_t analogPin = GPIO0;
#endif
    val = 0;
    while (size) {
        for (unsigned i = 0; i < 8; ++i) {
            int init = analogRead(analogPin);
            // Instead of waiting for change, just sample twice
            delayMicroseconds(1); // Tiny delay between reads
            int second = analogRead(analogPin);

            // Use difference between readings
            int diff = abs(second - init);
            val = (val << 1) | (diff & 0x01);
        }
        val++;
        --size;
    }

    // If we got 0, just use millis as fallback
    if (val == 0) {
        val = (::millis() & 0xFF) + 1;
    }

    return val;
}

// Seed the Arduino RNG once, lazily, from analog entropy (the core previously
// reseeded per generated ID; once per boot yields the same per-device
// uniqueness without repeated analog sampling).
static void seedOnce()
{
    static bool seeded = false;
    if (!seeded) {
        randomSeed(simpleRNG(4));
        seeded = true;
    }
}

namespace RadioMeshPlatform
{
uint32_t millis()
{
    return ::millis();
}

void delay(uint32_t ms)
{
    ::delay(ms);
}

uint32_t random(uint32_t max)
{
    if (max == 0) {
        return 0;
    }
    seedOnce();
    return static_cast<uint32_t>(::random(max));
}

void randomBytes(uint8_t* buf, size_t len)
{
    seedOnce();
    for (size_t i = 0; i < len; i++) {
        buf[i] = static_cast<uint8_t>(::random(256));
    }
}

uint64_t chipId()
{
#ifdef ESP32
    return ESP.getEfuseMac();
#else
    return 12345; // Fallback for non-ESP32 platforms
#endif
}

void consoleInit()
{
    // Moved verbatim from DeviceBuilder::start():
    // This gives us 10 seconds to do a hard reset if the board is in a bad
    // state after power cycle
    while (!Serial && ::millis() < 10000)
        ;
    Serial.begin(115200);
}

size_t logWrite(const uint8_t* data, size_t len)
{
    return Serial.write(data, len);
}

IRadio* loraRadio()
{
    return LoraRadio::getInstance();
}

IByteStorage* byteStorage()
{
    // Storage sizing is port business: the core receives a ready-to-begin()
    // storage and never references EEPROM_STORAGE_MAX_SIZE.
    EEPROMStorage* storage = EEPROMStorage::getInstance();
    storage->setParams(ByteStorageParams(EEPROM_STORAGE_MAX_SIZE));
    return storage;
}
} // namespace RadioMeshPlatform
