# RadioMesh — Zephyr port (`ports/zephyr`)

The full RadioMesh library on Zephyr, for XIAO ESP32-S3 + Wio-SX1262 nodes: the
platform-neutral core is co-compiled straight from `src/` (D2) with the Zephyr
implementations in `hal/`. The examples mirror the Arduino DeviceInclusion
examples and interoperate with Arduino nodes over the air. This tree is invisible
to PlatformIO (D3).

## Layout

```
ports/zephyr/
├── common/
│   └── xiao_esp32s3_procpu.overlay   # SX1262 pins — single source of truth (all apps)
├── examples/
│   ├── standard/  # STANDARD node (StandardDevice.ino mirror); relay.conf builds the relay
│   └── hub/       # HUB node (MiniHub.ino mirror, without the WiFi portal)
├── hal/           # Zephyr platform services, IRadio (ZephyrLoraRadio), NVS storage
├── tests/runner/  # on-board Unity runner for the repo's test/ suites
├── tools/rmon.py  # serial helper: list boards, monitor, run tests
└── patches/       # out-of-tree Zephyr fixes to re-apply after `west update` (see below)
```

Each example is a standalone Zephyr application (`CMakeLists.txt` + `prj.conf` +
`src/main.cpp`) that builds the full library (`CONFIG_RADIOMESH=y`) and pulls the
shared pin overlay via `DTC_OVERLAY_FILE`.

| Role | Builds | Device ID |
|---|---|---|
| `hub` | `examples/hub` | `5A000000` |
| `standard` (leaf) | `examples/standard` | `5A000001` |
| `relay` | `examples/standard` + `relay.conf` (relaying on) | `5A000002` |

MiniHub enables inclusion from its web portal; the Zephyr hub opens an inclusion
window 5 s after boot and again 30 s after each window closes.

## Setup & workflow

From `ports/zephyr/`: `make bootstrap` does the one-time setup (venv + Zephyr
workspace + SDK); then `make run ROLE=<role>` builds, flashes, and monitors.
Full guide: **[`DEVELOPING.md`](DEVELOPING.md)**.

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
make setup                                    # once, and after every `west update`
make flash ROLE=hub PORT=<port-A> ERASE=1     # hub, fresh NVS
make run ROLE=standard PORT=<port-B> ERASE=1  # leaf: build + flash + monitor
make boards                                   # list connected boards and their role
```

Under the hood `make build ROLE=hub` runs
`west build -b xiao_esp32s3/esp32s3/procpu examples/hub`.

## Expected result

```
# hub
Received inclusion request from device 5A000001 (total: 1)
Received inclusion confirmation from device 5A000001 - inclusion completed successfully (total: 1)
Received application message from 5A000001, topic: 0x10, value: 24

# leaf
Device state: INCLUDED - successfully joined network!
Sent sensor data: 24°C
```

The leaf joins at the hub's next inclusion window, then sends an encrypted mock
reading on topic `0x10` every 10 s. An included node rejoins from NVS after a
power cycle; flash it with `ERASE=1` to start it fresh.

## Notes

- **Console:** the XIAO's single USB-C is the ESP32-S3 native USB. The running
  Zephyr firmware presents a USB-Serial-JTAG console, which **discards TX until a
  host attaches** (~1.5 s after power-up); the examples hold their log output back
  3 s so the boot log survives. A serial monitor that reconnects across the
  post-flash reset is the easiest way to observe.
- **First flash over factory firmware** (e.g. Meshtastic) may need manual download
  mode: hold **BOOT**, replug USB, keep holding ~2 s. The board's serial port also
  changes name once the factory firmware is gone. Once our Zephyr image is on,
  `west flash` resets into download mode over JTAG with no button press.
- **TCXO:** `dio3-tcxo-voltage` in the overlay is required for this module —
  without it every TX returns `-116` (Tx timeout). A `-116` after a known-good
  build usually means the Wio-SX1262 shield is not fully seated (DIO1 floating).
- **PHY interop:** the XIAO preset (`LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262`:
  915 MHz / BW125 / SF8 / 20 dBm / private sync) plus the CR 4/7 and preamble 8
  pinned in `ZephyrLoraRadio` form the wire contract with Arduino nodes (D5);
  changing any of them breaks Arduino<->Zephyr interop.
