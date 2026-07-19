/*
 * RadioMesh Zephyr example — LoRa RX node.
 *
 * Continuously receives LoRa frames, parses each with the co-compiled
 * RadioMeshPacket buffer constructor, re-computes the payload CRC32, and logs
 * the decoded header fields. Pair with the `tx` example flashed to a second
 * XIAO ESP32-S3 + Wio-SX1262 board.
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

LOG_MODULE_REGISTER(rm_rx, LOG_LEVEL_INF);

#define LORA_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(LORA_NODE, okay),
             "lora0 alias not defined / not okay in the devicetree overlay");

#define MAX_LORA_LEN 255
#define RX_TIMEOUT K_SECONDS(10) // wake periodically to emit a liveness heartbeat

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
    if (rm_lora_configure(lora, false) < 0) {
        LOG_ERR("LoRa RX config failed");
        return -EIO;
    }
    LOG_INF("RadioMesh RX node ready; listening...");

    uint8_t rx[MAX_LORA_LEN];
    uint32_t received = 0;
    while (true) {
        int16_t rssi = 0;
        int8_t snr = 0;
        int len = lora_recv(lora, rx, sizeof(rx), RX_TIMEOUT, &rssi, &snr);
        if (len <= 0) {
            LOG_INF("listening... (received=%u)", received); // heartbeat on idle timeout
            continue;
        }

        std::vector<byte> rbuf(rx, rx + len);
        RadioMeshPacket rpkt(rbuf);
        bool crc_ok = (payload_crc(rpkt.packetData) == rpkt.packetCrc);
        received++;

        LOG_INF("RX len=%d rssi=%d snr=%d | topic=0x%02x fcounter=%u dataLen=%u crc=%s", len, rssi,
                (int)snr, rpkt.topic, rpkt.fcounter, (unsigned)rpkt.packetData.size(),
                crc_ok ? "OK" : "BAD");
    }

    return 0;
}
