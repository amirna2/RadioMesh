# RadioMesh — Zephyr port (`ports/zephyr`)

Milestone 1 tracer bullet: two XIAO ESP32-S3 + Wio-SX1262 nodes running Zephyr
exchange a `RadioMeshPacket` over LoRa, using the platform-neutral RadioMesh core
co-compiled straight from `src/` (D2). This tree is invisible to PlatformIO (D3).

## One-time workspace setup (T2 topology)

RadioMesh is its own west manifest repo. From the directory that should become the
workspace topdir (the parent of this repo). Requires **Python ≥ 3.10** (Zephyr 4.x):

```bash
python3 -m venv .venv-zephyr && source .venv-zephyr/bin/activate
pip install west
west init -l RadioMesh                        # RadioMesh = this repo's dir name
west update                                   # Zephyr + hal_espressif + loramac-node + picolibc
west zephyr-export
pip install -r zephyr/scripts/requirements.txt   # Zephyr Python deps
west packages pip --install                      # module deps incl. esptool (ESP32 image tool)
west sdk install -t xtensa-espressif_esp32s3_zephyr-elf   # xtensa toolchain only
```

Host build tools (macOS): `brew install cmake ninja gperf dtc libmagic ccache`.
`west update` clones Zephyr *beside* this repo, not inside it — nothing to gitignore.
Pin the Zephyr `revision` in `west.yml` to match your installed Zephyr SDK.

> Verified on macOS 26 / arm64 with Zephyr 4.4.0 + SDK 1.0.1: `west build` below
> produces `zephyr.elf` + an ESP32-S3 image (~155 KB flash). Flashing + the 2-node
> OTA exchange still require the physical boards.

## Build & flash (run per node)

```bash
west build -b xiao_esp32s3/esp32s3/procpu ports/zephyr/app -p always
west flash
```

The same firmware is symmetric (transmit + receive), so flash the identical image
to **both** boards.

## Expected result (manual M1 gate)

On each node's console (`west espressif monitor` or a serial terminal @115200):

```
RadioMesh M1 bring-up: LoRa device ready
TX fcounter=0 len=38 ret=0
RX len=38 rssi=-42 snr=9 | topic=0x10 fcounter=3 dataLen=3 crc=OK
```

Seeing the peer's `RX ... crc=OK` on both nodes is M1 success: the co-compiled M0
core serialized, transmitted, received, and parsed a packet over a real Zephyr
LoRa link.

## Notes / likely on-hardware tweaks

- **Console:** if no serial output appears, the XIAO may route its console over
  native USB-CDC; enable the USB console in `prj.conf` for that board.
- **TCXO:** `dio3-tcxo-voltage` in the overlay is required for this module — without
  it every TX returns `-116` (Tx timeout).
- **PHY interop:** the `lora_config` in `app/src/main.cpp` is pinned to the Arduino
  RadioMesh XIAO preset (915 MHz / BW125 / SF8 / CR4-7 / preamble 8 / private sync);
  changing any value breaks Arduino<->Zephyr interop (D5).
