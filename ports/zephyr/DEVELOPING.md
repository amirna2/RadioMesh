# Developing the RadioMesh Zephyr port

The `Makefile` in this directory is the developer **and** CI entry point. Run all
commands below from `ports/zephyr/`.

## Prerequisites

- **Python ≥ 3.10** (Zephyr 4.x). Older system Pythons are too old — use a venv.
- Host tools: `cmake`, `ninja`, `dtc`, `gperf`, `ccache`
  (macOS: `brew install cmake ninja dtc gperf ccache`).
- A **Zephyr SDK** with the xtensa ESP32-S3 toolchain.

## One-time bootstrap

RadioMesh is its own west manifest repo (T2 topology). From the directory that
will become the workspace topdir — the **parent** of this repo:

```bash
python3 -m venv .venv-zephyr && source .venv-zephyr/bin/activate
pip install west
west init -l RadioMesh          # RadioMesh = this repo's directory name
west update                     # clones Zephyr + modules as siblings of RadioMesh/
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
west packages pip --install     # module deps, incl. esptool (ESP32 image tool)
west sdk install -t xtensa-espressif_esp32s3_zephyr-elf
```

Then, from `RadioMesh/ports/zephyr/`:

```bash
make setup      # verify the workspace + (re-)apply the carried gpio_esp32 patch
```

Re-run `make setup` after **every** `west update` — it re-applies the gpio_esp32
patch that a fresh checkout drops (see `README.md` for why the patch exists).

> If you would rather not `source` the venv, pass `VENV=/path/to/venv` to every
> make command, e.g. `make build ROLE=tx VENV=~/dev/.venv-zephyr`.

## Everyday workflow

```bash
make boards               # which boards are connected + the role each is running
make build ROLE=tx        # build the tx (or rx) example
make flash ROLE=tx        # flash; auto-detects the port if exactly one board
make monitor              # reconnecting serial console (Ctrl-C to stop)
make run   ROLE=rx        # build + flash + monitor in one shot
make clean [ROLE=tx]      # remove build dir(s)
```

Ports auto-detect when a single board is attached. With two boards connected,
`flash`/`monitor` **refuse to guess** — pass `PORT=/dev/cu.usbmodemXXXX`. Use
`make boards` to see each port and its role.

## Two-board bring-up (the M1 tracer bullet)

With two XIAO ESP32-S3 + Wio-SX1262 boards attached:

```bash
make boards                                  # note each board's port

# Board A — transmitter (Ctrl-C the monitor once you see TX ... ret=0)
make run ROLE=tx PORT=/dev/cu.usbmodemAAAA

# Board B — receiver
make run ROLE=rx PORT=/dev/cu.usbmodemBBBB
```

On the receiver you should see, on the transmitter's cadence (~3 s):

```
rm_rx: RX len=38 rssi=-49 snr=12 | topic=0x10 fcounter=N dataLen=3 crc=OK
```

That is M1 success: the co-compiled RadioMesh core serialized a packet on A,
transmitted it over LoRa, and B received, parsed, and CRC-checked it.

## Gotchas

- **Console:** the XIAO's single USB-C is the SoC's native USB. The running
  firmware's USB-Serial-JTAG console discards TX until a host attaches (~1.5 s)
  and drops the port across a reboot — `make monitor` reconnects across both.
- **First flash over factory firmware** (e.g. Meshtastic) needs manual ROM
  download mode: hold **BOOT**, replug USB, keep holding ~2 s, then `make flash`.
  Once our Zephyr image is on, `make flash` resets over JTAG with no button press.
- **`-116` (Tx timeout)** after a known-good build usually means the Wio-SX1262
  shield is not fully seated (DIO1 floating), not a config problem.

## Editor / IntelliSense

Every build writes a `compile_commands.json` into its build dir
(`<workspace>/.build-<role>/`). Point clangd or the C/C++ extension at that file
for accurate IntelliSense on Zephyr headers — **do not** hardcode Zephyr include
paths. Until your editor uses that compile database, Zephyr symbols such as
`LOG_MODULE_REGISTER` will show as unresolved; that is an editor-config artifact,
not a build warning (`make build` compiles warning-free).
