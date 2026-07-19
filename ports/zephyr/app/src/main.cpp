/*
 * RadioMesh Zephyr port — Milestone 1 tracer bullet.
 *
 * Symmetric firmware: flash the SAME image to BOTH XIAO ESP32-S3 + Wio SX1262
 * nodes. Each node periodically transmits a RadioMeshPacket built with the
 * co-compiled RadioMesh core, and otherwise listens. On reception it parses the
 * incoming bytes via the RadioMeshPacket buffer constructor, re-computes the
 * payload CRC32, and logs the decoded header fields.
 *
 * M1 success = the platform-neutral M0 core (RadioMeshPacket + CRC32) drives a
 * real Zephyr LoRa link end to end. No routing, relay, crypto, or inclusion.
 *
 * PHY config MUST match the Arduino RadioMesh XIAO preset for D5 interop:
 *   915 MHz, BW 125 kHz, SF8, CR 4/7, preamble 8, TX power 20 dBm,
 *   private sync word => SX126x public_network = false.
 * CR 4/7 and preamble 8 are RadioLib 7.1.0 begin() defaults (never set
 * explicitly on the Arduino side); pinned here so both stacks demodulate.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <vector>

#include <common/utils/RadioMeshCrc32.h>
#include <core/protocol/inc/packet/Packet.h>

LOG_MODULE_REGISTER(rm_m1, LOG_LEVEL_INF);

#define LORA_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(LORA_NODE, okay),
             "lora0 alias not defined / not okay in the devicetree overlay");

#define APP_TOPIC 0x10
#define RX_WINDOW K_SECONDS(5)
#define MAX_LORA_LEN 255

static uint32_t payload_crc(const std::vector<byte>& data)
{
    RadioMeshUtils::CRC32 crc;
    crc.update(data.data(), data.size());
    return crc.finalize();
}

static void set_direction(const struct device* dev, bool tx)
{
    struct lora_modem_config cfg = {};
    cfg.frequency = 915000000;
    cfg.bandwidth = BW_125_KHZ;
    cfg.datarate = SF_8;
    cfg.coding_rate = CR_4_7;
    cfg.preamble_len = 8;
    cfg.tx_power = 20;
    cfg.tx = tx;
    cfg.iq_inverted = false;
    cfg.public_network = false; // private sync word (matches Arduino privateNetwork=true)

    int ret = lora_config(dev, &cfg);
    if (ret < 0) {
        LOG_ERR("lora_config(tx=%d) failed: %d", (int)tx, ret);
    }
}

int main(void)
{
    const struct device* lora = DEVICE_DT_GET(LORA_NODE);

    if (!device_is_ready(lora)) {
        LOG_ERR("LoRa device not ready");
        return 0;
    }
    LOG_INF("RadioMesh M1 bring-up: LoRa device ready");

    uint32_t fcounter = 0;

    while (true) {
        // ---- Build + transmit a RadioMeshPacket via the co-compiled core ----
        RadioMeshPacket pkt;
        pkt.topic = APP_TOPIC;
        pkt.sourceDevId = {0x00, 0x00, 0x00, 0x01};
        pkt.destDevId = {0xFF, 0xFF, 0xFF, 0xFF};
        pkt.packetId = {(byte)(fcounter >> 24), (byte)(fcounter >> 16), (byte)(fcounter >> 8),
                        (byte)fcounter};
        pkt.fcounter = fcounter;
        pkt.packetData = {'R', 'M', (byte)(fcounter & 0xFF)};
        pkt.packetCrc = payload_crc(pkt.packetData); // "Data CRC" field = payload integrity

        std::vector<byte> buf = pkt.toByteBuffer();

        set_direction(lora, true);
        int ret = lora_send(lora, buf.data(), buf.size());
        LOG_INF("TX fcounter=%u len=%u ret=%d", fcounter, (unsigned)buf.size(), ret);
        fcounter++;

        // ---- Listen for a packet from the peer node ----
        set_direction(lora, false);

        uint8_t rx[MAX_LORA_LEN];
        int16_t rssi = 0;
        int8_t snr = 0;
        int len = lora_recv(lora, rx, sizeof(rx), RX_WINDOW, &rssi, &snr);
        if (len <= 0) {
            continue; // timeout / no packet this window
        }

        std::vector<byte> rbuf(rx, rx + len);
        RadioMeshPacket rpkt(rbuf);
        bool crc_ok = (payload_crc(rpkt.packetData) == rpkt.packetCrc);

        LOG_INF("RX len=%d rssi=%d snr=%d | topic=0x%02x fcounter=%u dataLen=%u crc=%s", len, rssi,
                (int)snr, rpkt.topic, rpkt.fcounter, (unsigned)rpkt.packetData.size(),
                crc_ok ? "OK" : "BAD");
    }

    return 0;
}
