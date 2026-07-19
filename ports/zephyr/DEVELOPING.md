# Developing the RadioMesh Zephyr port

This directory builds the RadioMesh examples for **Zephyr RTOS** on the
Seeed **XIAO ESP32-S3 + Wio-SX1262** kit. The `Makefile` here is the single entry
point for everything: building, flashing, and talking to the boards.

Everything below works on **macOS** and **Linux**. Where the two differ (serial
device names, package managers, permissions) both are shown.

---

## At a glance

Run all commands from `ports/zephyr/`.

| Command | What it does | Needs the Zephyr toolchain? |
| --- | --- | --- |
| `make setup` | Verify the workspace and (re-)apply the `gpio_esp32` patch | yes |
| `make build ROLE=tx\|rx` | Build an example into an out-of-tree build dir | yes |
| `make flash ROLE=tx\|rx [PORT=…]` | Flash a board (auto-detects a single board) | yes |
| `make run ROLE=tx\|rx [PORT=…]` | Build + flash + monitor | yes |
| `make boards` | List connected boards **and the role each is running** | **no** |
| `make monitor [PORT=…]` | Reconnecting serial console (Ctrl-C to stop) | **no** |
| `make clean [ROLE=tx\|rx]` | Remove build dir(s) | no |

The **serial helpers** (`boards`, `monitor`, and the underlying `tools/rmon.py`)
have **zero dependencies** — they run under any system `python3`, no venv, no
`pip install`. Only building and flashing need the Zephyr toolchain.

---

## Prerequisites

|  | macOS | Linux (Debian/Ubuntu) |
| --- | --- | --- |
| Python | ≥ 3.10 (`brew install python`) — the system Python may be too old | ≥ 3.10 (`sudo apt install python3 python3-venv python3-pip`) |
| Host tools | `brew install cmake ninja dtc gperf ccache` | `sudo apt install cmake ninja-build gperf ccache dtc` |
| Serial access | works out of the box | add yourself to the `dialout` group (see Troubleshooting) |
| Zephyr SDK | installed by `west sdk install` (below) | same |

---

## First-time setup (once per machine)

RadioMesh is its own west manifest repo. Pick a **workspace directory** — it will
hold `RadioMesh/` next to a freshly cloned `zephyr/` and the modules. From the
**parent** of this repo:

```bash
python3 -m venv .venv-zephyr
source .venv-zephyr/bin/activate          # (bash/zsh)
pip install west

west init -l RadioMesh                     # RadioMesh = this repo's directory name
west update                                # clones Zephyr + modules beside RadioMesh/
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
west packages pip --install                # module deps, incl. esptool (ESP32 imager)
west sdk install -t xtensa-espressif_esp32s3_zephyr-elf
```

Then, from `RadioMesh/ports/zephyr/`:

```bash
make setup
```

`make setup` verifies the workspace and applies the carried `gpio_esp32` patch
(Zephyr 4.4.0 rejects the SX1262's GPIO≥32 control lines without it — see
[`README.md`](README.md)). **Re-run `make setup` after every `west update`**, which
restores the pristine Zephyr tree and drops the patch.

> **Don't want to `source` the venv every time?** Pass `VENV=/path/to/.venv-zephyr`
> to any `make` command that needs the toolchain, e.g.
> `make build ROLE=tx VENV=~/dev/.venv-zephyr`. The serial helpers never need it.

---

## Everyday workflow

```bash
make boards                 # what's connected and which role each board runs
make build ROLE=tx          # build the tx (or rx) example
make flash ROLE=tx          # flash it (auto-detects the port if one board)
make monitor                # watch the serial console (Ctrl-C to stop)
make run   ROLE=rx          # build + flash + monitor, in one command
make clean                  # remove both build dirs
```

**Port selection.** With a single board attached, `flash`/`monitor` auto-detect
it. With two or more, they **refuse to guess** and ask you to pass `PORT=`:

```bash
make boards                              # copy the port you want
make flash ROLE=tx PORT=/dev/cu.usbmodem1401      # macOS
make flash ROLE=tx PORT=/dev/ttyACM0              # Linux
```

---

## Bring up two nodes (the M1 tracer bullet)

With two boards, one becomes the transmitter and one the receiver:

```bash
make boards                                   # note each board's port + role

# Board A — transmitter
make run ROLE=tx PORT=<port-A>                # once you see "TX ... ret=0", Ctrl-C the monitor

# Board B — receiver
make run ROLE=rx PORT=<port-B>
```

On the **receiver** you should see, on the transmitter's ~3 s cadence:

```
rm_rx: RX len=38 rssi=-49 snr=12 | topic=0x10 fcounter=N dataLen=3 crc=OK
```

That is the whole point of M1: the platform-neutral RadioMesh core, co-compiled
from `src/`, serialized a packet on A, sent it over LoRa, and B received, parsed,
and CRC-checked it. An idle receiver prints a `listening... (received=N)` heartbeat
every 10 s so you can always tell it is alive.

---

## The serial/board tool (`tools/rmon.py`)

`make boards` and `make monitor` wrap `tools/rmon.py`, a dependency-free serial
helper (Python standard library only). You can call it directly:

```bash
python3 tools/rmon.py list        # connected board ports, one per line
python3 tools/rmon.py boards      # each port + the firmware role it's running
python3 tools/rmon.py monitor     # reconnecting console (survives reboot/replug)
```

It knows the role by reading the Zephyr log-module tag in the console stream
(`rm_tx` → tx, `rm_rx` → rx, `rm_m1` → the legacy symmetric app). The monitor
reconnects across the board's post-flash reset, so you never have to restart it.

Serial device names differ by OS — the tool handles both: `/dev/cu.usbmodem*` on
macOS, `/dev/ttyACM*` on Linux.

---

## Troubleshooting

| Symptom | Cause | Fix |
| --- | --- | --- |
| `make boards` prints nothing / "no boards found" | Board not enumerated, or (Linux) wrong glob | Replug; confirm it appears as `/dev/cu.usbmodem*` (macOS) or `/dev/ttyACM*` (Linux) |
| `Permission denied` opening the port (Linux) | User not in the `dialout` group | `sudo usermod -aG dialout $USER`, then log out/in |
| `make build` → `west: command not found` | Zephyr venv not active | `source .venv-zephyr/bin/activate` or pass `VENV=…` |
| `make boards` shows `unknown (no role output…)` | A receiver with no peer is quiet, or the app didn't boot | It heartbeats every 10 s; if still silent, replug to reboot, or check the firmware booted |
| First flash over factory firmware (Meshtastic) fails to connect | Board isn't in ROM download mode | Hold **BOOT**, replug USB, keep holding ~2 s, then `make flash`. Once our Zephyr image is on, `make flash` resets over JTAG automatically |
| Every TX returns `-116` (Tx timeout) after a good build | Wio-SX1262 shield not fully seated (DIO1 floating) | Reseat the SX1262 board onto the XIAO |
| No boot banner in `make monitor` | The USB-Serial-JTAG console discards TX until the host attaches (~1.5 s) | Expected; the periodic TX/RX lines still appear. `make monitor` reconnects automatically |
| After `west update`, build fails on a GPIO pin | The `gpio_esp32` patch was dropped by the update | Re-run `make setup` (it re-applies the patch) |

---

## Editor / IntelliSense

Each build writes a `compile_commands.json` into its build dir
(`<workspace>/.build-<role>/`). Point clangd or the VS Code C/C++ extension at that
file for accurate IntelliSense over the Zephyr headers — **do not** hardcode Zephyr
include paths. Until your editor uses that compile database, Zephyr macros such as
`LOG_MODULE_REGISTER` show as unresolved in the editor; that is an editor-config
artifact, not a build warning (`make build` compiles cleanly).
