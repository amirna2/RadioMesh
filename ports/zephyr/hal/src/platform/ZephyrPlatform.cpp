#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>

#include <common/inc/platform/RadioMeshPlatform.h>
#include <radio/ZephyrLoraRadio.h>
#include <storage/ZephyrNVSStorage.h>

namespace RadioMeshPlatform
{
uint32_t millis()
{
    return k_uptime_get_32();
}

void delay(uint32_t ms)
{
    k_msleep(ms);
}

uint32_t random(uint32_t max)
{
    if (max == 0) {
        return 0;
    }
    uint32_t value;
    sys_rand_get(&value, sizeof(value));
    return value % max;
}

void randomBytes(uint8_t* buf, size_t len)
{
    sys_rand_get(buf, len);
}

uint64_t chipId()
{
    // hwinfo yields the factory-programmed base MAC on the ESP32-S3: stable
    // across boots and unique per device, the same efuse the Arduino port
    // reads via ESP.getEfuseMac(). The id only seeds a local device key and
    // never crosses the wire, so the two ports' byte ordering need not match.
    uint8_t id[8] = {0};
    ssize_t length = hwinfo_get_device_id(id, sizeof(id));
    if (length <= 0) {
        return 12345; // Fallback, mirroring the Arduino non-ESP32 path
    }

    uint64_t value = 0;
    for (ssize_t i = 0; i < length; i++) {
        value = (value << 8) | id[i];
    }
    return value;
}

void consoleInit()
{
    // The Zephyr console (UART or USB-CDC, per board config) is brought up by
    // the kernel before main(); nothing to initialize here.
}

size_t logWrite(const uint8_t* data, size_t len)
{
    printk("%.*s", static_cast<int>(len), reinterpret_cast<const char*>(data));
    return len;
}

IRadio* loraRadio()
{
    return ZephyrLoraRadio::getInstance();
}

IByteStorage* byteStorage()
{
    return ZephyrNVSStorage::getInstance();
}
} // namespace RadioMeshPlatform
