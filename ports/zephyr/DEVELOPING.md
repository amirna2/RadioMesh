# Developing the RadioMesh Zephyr port

Build, flash, and monitor the LoRa examples for the XIAO ESP32-S3 + Wio-SX1262
kit. Run every command from `ports/zephyr/`. Works on macOS and Linux.

## Prerequisites

Python ≥ 3.10 and host build tools:

- **macOS:** `brew install python cmake ninja dtc gperf ccache`
- **Linux (Debian/Ubuntu):** `sudo apt install python3 python3-venv cmake ninja-build dtc gperf ccache`

Linux serial access: `sudo usermod -aG dialout $USER`, then log out and back in.

## First-time setup

One command:

    make bootstrap

It creates a Python venv, sets up the Zephyr workspace and SDK, and applies the
required driver patch. Re-run `make setup` after any `west update`.

## Commands

| Command | Does | Needs toolchain |
|---|---|---|
| `make bootstrap` | first-time: venv + Zephyr workspace + SDK | — |
| `make setup` | re-apply the driver patch (after `west update`) | yes |
| `make build ROLE=tx\|rx` | build an example | yes |
| `make flash ROLE=tx\|rx [PORT=]` | flash a board | yes |
| `make run ROLE=tx\|rx [PORT=]` | build + flash + monitor | yes |
| `make test SUITE=<name> [PORT=] [TIMEOUT=]` | run one `test/` suite on a board | yes |
| `make boards` | list boards and their role | no |
| `make monitor [PORT=]` | serial console (Ctrl-C to quit) | no |
| `make clean [ROLE=]` | remove build outputs | no |

`flash` and `monitor` auto-detect a single board; with two, pass `PORT=`
(`/dev/cu.usbmodem*` on macOS, `/dev/ttyACM*` on Linux).

## Two-board bring-up

    make boards                      # note each port
    make run ROLE=tx PORT=<port-A>   # transmitter (Ctrl-C after "TX ... ret=0")
    make run ROLE=rx PORT=<port-B>   # receiver

The receiver logs, about every 3 s:

    rm_rx: RX len=38 ... topic=0x10 fcounter=N dataLen=3 crc=OK

## On-board unit tests

`make test SUITE=<name>` builds one suite from the repo's `test/` directory
with the on-board runner (`tests/runner`), flashes it, and prints the Unity
output. It exits 0 only when the suite reports no failures; a failure, a reboot
mid-run, or no result within `TIMEOUT` seconds (default 120) exits non-zero.

    make test SUITE=test_Packet

The suites are the same sources the PlatformIO test environments run, with the
same Unity version. The display, WiFi and Arduino-driver suites
(`test_CustomDisplay`, `test_Wifi*`, `test_LoraRadio`, `test_EEPROMStorage`)
are Arduino-only. Suites that use storage erase the board's NVS. All suites
share one build directory; `make clean ROLE=test` removes it.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `west: command not found` | run `make bootstrap`, or activate your venv / pass `VENV=/path` |
| Linux `Permission denied` on the port | `sudo usermod -aG dialout $USER`, then re-login |
| No board found | replug; check `/dev/cu.usbmodem*` (macOS) or `/dev/ttyACM*` (Linux) |
| First flash over Meshtastic won't connect | hold BOOT, replug USB ~2 s, then `make flash` |
| Every TX returns `-116` | reseat the Wio-SX1262 shield (DIO1 floating) |
| Build fails on a GPIO pin after `west update` | `make setup` (re-applies the patch) |

## Editor

Each build writes `compile_commands.json` in `<workspace>/.build-<role>/`. Point
clangd or the C/C++ extension at it for Zephyr-aware IntelliSense.
