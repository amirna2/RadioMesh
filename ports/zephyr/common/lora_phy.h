/*
 * RadioMesh Zephyr examples — shared LoRa PHY setup.
 *
 * The PHY parameters below are the RadioMesh interop CONTRACT: they match the
 * Arduino XIAO_ESP32S3_WIO_SX1262 preset. Every example configures the radio
 * through rm_lora_configure() so that a Zephyr node and an Arduino node — and a
 * `tx` example and an `rx` example — land on the same channel and modulation.
 * Changing any value here silently breaks interop (D5).
 *
 *   915 MHz, BW 125 kHz, SF8, CR 4/7, preamble 8, TX 20 dBm, private sync word.
 *
 * CR 4/7 and preamble 8 are RadioLib 7.1.0 begin() defaults — never set
 * explicitly on the Arduino side, so they are pinned here to match.
 */
#ifndef RADIOMESH_ZEPHYR_LORA_PHY_H
#define RADIOMESH_ZEPHYR_LORA_PHY_H

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

/* Application message topic for the tracer-bullet examples (reserved range). */
#define RM_APP_TOPIC 0x10

/*
 * Configure the radio for transmit (tx=true) or receive (tx=false). Both roles
 * share every other parameter, which is the whole point of centralizing it here.
 * Returns the underlying lora_config() result (0 on success, <0 on error).
 */
static inline int rm_lora_configure(const struct device* dev, bool tx)
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
	cfg.public_network = false; /* private sync word (Arduino privateNetwork=true) */
	return lora_config(dev, &cfg);
}

#endif /* RADIOMESH_ZEPHYR_LORA_PHY_H */
