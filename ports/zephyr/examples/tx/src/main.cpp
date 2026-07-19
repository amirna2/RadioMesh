/*
 * RadioMesh Zephyr example — LoRa TX node.
 *
 * Builds a RadioMeshPacket with the co-compiled, platform-neutral RadioMesh core
 * (RadioMeshPacket + CRC32) and transmits it over LoRa on a fixed cadence. Pair
 * with the `rx` example flashed to a second XIAO ESP32-S3 + Wio-SX1262 board.
 *
 * Scope: raw point-to-point LoRa only — no routing, relay, crypto, or inclusion.
 * The PHY is configured through rm_lora_configure() (see common/lora_phy.h) so
 * this node stays wire-compatible with Arduino RadioMesh nodes (D5).
 */
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <vector>

#include <common/utils/RadioMeshCrc32.h>
#include <core/protocol/inc/packet/Packet.h>

#include "lora_phy.h"

LOG_MODULE_REGISTER(rm_tx, LOG_LEVEL_INF);

#define LORA_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(LORA_NODE, okay),
             "lora0 alias not defined / not okay in the devicetree overlay");

#define TX_PERIOD K_SECONDS(3)

static uint32_t payload_crc(const std::vector<byte>& data)
{
    RadioMeshUtils::CRC32 crc;
    crc.update(data.data(), data.size());
    return crc.finalize();
}

int main(void)
{
    const struct device* lora = DEVICE_DT_GET(LORA_NODE);
    if (!device_is_ready(lora)) {
        LOG_ERR("LoRa device %s not ready", lora->name);
        return -ENODEV;
    }
    if (rm_lora_configure(lora, true) < 0) {
        LOG_ERR("LoRa TX config failed");
        return -EIO;
    }
    LOG_INF("RadioMesh TX node ready");

    uint32_t fcounter = 0;
    while (true) {
        RadioMeshPacket pkt;
        pkt.topic = RM_APP_TOPIC;
        pkt.sourceDevId = {0x00, 0x00, 0x00, 0x01};
        pkt.destDevId = {0xFF, 0xFF, 0xFF, 0xFF};
        pkt.packetId = {(byte)(fcounter >> 24), (byte)(fcounter >> 16), (byte)(fcounter >> 8),
                        (byte)fcounter};
        pkt.fcounter = fcounter;
        pkt.packetData = {'R', 'M', (byte)(fcounter & 0xFF)};
        pkt.packetCrc = payload_crc(pkt.packetData); // "Data CRC" field = payload integrity

        std::vector<byte> buf = pkt.toByteBuffer();
        int ret = lora_send(lora, buf.data(), buf.size());
        LOG_INF("TX fcounter=%u len=%u ret=%d", fcounter, (unsigned)buf.size(), ret);

        fcounter++;
        k_sleep(TX_PERIOD);
    }

    return 0;
}
