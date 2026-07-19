# RadioMesh — Zephyr port (`ports/zephyr`)

Milestone 1 tracer bullet: two XIAO ESP32-S3 + Wio-SX1262 nodes running Zephyr
exchange a `RadioMeshPacket` over LoRa, using the platform-neutral RadioMesh core
co-compiled straight from `src/` (D2). This tree is invisible to PlatformIO (D3).

## Layout

```
ports/zephyr/
├── common/
│   ├── xiao_esp32s3_procpu.overlay   # SX1262 pins — single source of truth (all examples)
│   └── lora_phy.h                    # interop PHY config (freq/SF/BW/CR/...) + app topic
├── examples/
│   ├── tx/    # transmit-only node: builds a RadioMeshPacket and sends on a cadence
│   └── rx/    # receive-only node: parses each frame + re-checks the payload CRC32
└── patches/   # out-of-tree Zephyr fixes to re-apply after `west update` (see below)
```

Each example is a standalone Zephyr application (`CMakeLists.txt` + `prj.conf` +
`src/main.cpp`). Both pull the shared pin overlay via `DTC_OVERLAY_FILE` and the
shared PHY setup from `common/lora_phy.h`, so the pins and the modulation
parameters live in exactly one place. Future device-role examples (leaf, router,
hub) will slot in alongside `tx`/`rx`.

## Setup & workflow

From `ports/zephyr/`: `make bootstrap` does the one-time setup (venv + Zephyr
workspace + SDK); then `make run ROLE=tx` / `make run ROLE=rx` build, flash, and
monitor. Full guide: **[`DEVELOPING.md`](DEVELOPING.md)**.

### Why the carried `gpio_esp32` patch

Zephyr 4.4.0's `gpio_esp32` driver rejects any pin ≥ 32 (`gpio_pin_is_valid()`
uses a 32-bit `BIT()` shift), which blocks the SX1262 control lines on this kit
(CS/RST/BUSY/DIO1 are all GPIO ≥ 39). The fix (upstream on `main`, switching to
`BIT64()`) is carried in `patches/` and applied automatically by `make setup` —
re-run `make setup` after every `west update`, which restores the pristine tree.

> Verified with Zephyr 4.4.0 + SDK 1.0.1 (macOS arm64); the same flow applies on Linux.

## Build & flash

The `Makefile` here is the developer entry point. Full setup and workflow are in
[`DEVELOPING.md`](DEVELOPING.md); the short version, from `ports/zephyr/`:

```bash
make setup              # once, and after every `west update` (re-applies the gpio patch)
make run ROLE=tx        # build + flash + monitor a transmitter
make run ROLE=rx        # ...and a receiver on the second board
make boards             # list connected boards and the role each is running
```

Each role is a standalone Zephyr app; under the hood `make build ROLE=tx` runs
`west build -b xiao_esp32s3/esp32s3/procpu examples/tx`.

## Expected result (manual M1 gate)

On each node's console (`make monitor`, or a serial terminal @115200):

```
# tx node
<inf> rm_tx: RadioMesh TX node ready
<inf> rm_tx: TX fcounter=0 len=38 ret=0

# rx node
<inf> rm_rx: RadioMesh RX node ready; listening...
<inf> rm_rx: RX len=38 rssi=-44 snr=12 | topic=0x10 fcounter=0 dataLen=3 crc=OK
```

Seeing the transmitter's `RX ... crc=OK` on the receiver is M1 success: the
co-compiled M0 core serialized, transmitted, received, and parsed a packet over a
real Zephyr LoRa link.

## Notes

- **Console:** the XIAO's single USB-C is the ESP32-S3 native USB. The running
  Zephyr firmware presents a USB-Serial-JTAG console, which **discards TX until a
  host attaches** (~1.5 s after power-up) — the boot banner is often lost, but the
  periodic `TX`/`RX` log lines are not. A serial monitor that reconnects across the
  post-flash reset is the easiest way to observe.
- **First flash over factory firmware** (e.g. Meshtastic) needs manual download
  mode: hold **BOOT**, replug USB, keep holding ~2 s. Once our Zephyr image is on,
  `west flash` resets into download mode over JTAG with no button press.
- **TCXO:** `dio3-tcxo-voltage` in the overlay is required for this module —
  without it every TX returns `-116` (Tx timeout). A `-116` after a known-good
  build usually means the Wio-SX1262 shield is not fully seated (DIO1 floating).
- **PHY interop:** `common/lora_phy.h` pins the modulation to the Arduino RadioMesh
  XIAO preset (915 MHz / BW125 / SF8 / CR4-7 / preamble 8 / private sync); changing
  any value breaks Arduino<->Zephyr interop (D5).
```

