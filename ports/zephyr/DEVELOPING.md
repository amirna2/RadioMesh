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
| `make build ROLE=standard\|relay\|hub` | build an example | yes |
| `make flash ROLE=<role> [PORT=] [ERASE=1]` | flash a board | yes |
| `make run ROLE=<role> [PORT=] [ERASE=1]` | build + flash + monitor | yes |
| `make test SUITE=<name> [PORT=] [TIMEOUT=]` | run one `test/` suite on a board | yes |
| `make boards` | list boards and their role | no |
| `make monitor [PORT=]` | serial console (Ctrl-C to quit) | no |
| `make clean [ROLE=]` | remove build outputs | no |

`flash` and `monitor` auto-detect a single board; with two, pass `PORT=`
(`/dev/cu.usbmodem*` on macOS, `/dev/ttyACM*` on Linux). A port can be open in
one program at a time: close other serial monitors before flashing.

`ERASE=1` wipes the whole flash first, clearing NVS (device keys, network key,
inclusion state) so the board starts as a fresh node.

## Cluster bring-up

    make boards                                     # note each port
    make flash ROLE=hub PORT=<port-A> ERASE=1       # hub
    make flash ROLE=standard PORT=<port-B> ERASE=1  # leaf
    make flash ROLE=relay PORT=<port-C> ERASE=1     # optional relay
    make monitor PORT=<port-A>                      # watch the hub

The hub opens an inclusion window 5 s after boot and again 30 s after each one
closes; each window includes one node. Once included, the leaf sends a reading
every 10 s and the hub logs:

    Received application message from 5A000001, topic: 0x10, value: 24

Nodes join only in the hub's direct range (relays don't forward inclusion
messages). A relay then re-broadcasts telemetry; when the hub also hears the
leaf directly, it drops the relayed copy (`Packet already seen`).

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
| Flash fails with `Resource busy` | another program (e.g. a serial monitor) has the port open; close it |
| `ERASE=1` flash fails with `Device not configured` | the blank board re-enumerated between erase and write; re-run `make flash` |

## Editor

Each build writes `compile_commands.json` in `<workspace>/.build-<role>/`. Point
clangd or the C/C++ extension at it for Zephyr-aware IntelliSense.
