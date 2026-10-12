/*
 * RadioMesh Zephyr example — STANDARD device.
 *
 * Builds a STANDARD node through the same DeviceBuilder chain as the Arduino
 * examples/DeviceInclusion/Standard/StandardDevice.ino, sets up the radio and
 * runs the device loop. Current scope (V0): prove the full library builds and
 * boots on the XIAO ESP32-S3 + Wio-SX1262 — builder, NVS-backed storage, radio
 * setup and the run loop. Telemetry and the cluster roles come later.
 */
#include <zephyr/kernel.h>

#include <RadioMesh.h>

// Fixed test identity; the cluster examples assign one per role.
static const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x5A, 0x00, 0x00, 0x01};
static const std::string DEVICE_NAME = "ZephyrStandard";

static const uint32_t STATUS_INTERVAL_MS = 10000;

static void onPacketReceived(const RadioMeshPacket* packet, int err)
{
    if (err != RM_E_NONE || packet == nullptr) {
        logerr_ln("RX Failed [%d]", err);
        return;
    }
    loginfo_ln("Received packet, topic: 0x%02X", packet->topic);
}

int main()
{
    IDevice* device = DeviceBuilder()
                          .start()
                          .withLoraRadio(LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262)
                          .withRxPacketCallback(onPacketReceived)
                          .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::STANDARD);
    if (device == nullptr) {
        logerr_ln("Failed to create device");
        return 0;
    }

    IRadio* radio = device->getRadio();
    if (radio == nullptr) {
        logerr_ln("Failed to get radio instance");
        return 0;
    }

    int rc = radio->setup();
    if (rc != RM_E_NONE) {
        logerr_ln("Radio setup failed: %d", rc);
        return 0;
    }
    loginfo_ln("Standard device initialized successfully");

    uint32_t lastStatus = k_uptime_get_32();
    while (true) {
        device->run();

        if (k_uptime_get_32() - lastStatus >= STATUS_INTERVAL_MS) {
            loginfo_ln("Status: uptime %u s, included: %s", k_uptime_get_32() / 1000,
                       device->isIncluded() ? "yes" : "no");
            lastStatus = k_uptime_get_32();
        }

        k_msleep(10);
    }
    return 0;
}
