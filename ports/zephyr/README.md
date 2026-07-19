# RadioMesh — Zephyr port (`ports/zephyr`)

Milestone 1 tracer bullet: two XIAO ESP32-S3 + Wio-SX1262 nodes running Zephyr
exchange a `RadioMeshPacket` over LoRa, using the platform-neutral RadioMesh core
co-compiled straight from `src/` (D2). This tree is invisible to PlatformIO (D3).

## One-time workspace setup (T2 topology)

RadioMesh is its own west manifest repo. From the directory that should become the
workspace topdir (the parent of this repo):

```bash
west init -l RadioMesh        # RadioMesh = this repo's dir name
west update                   # fetches Zephyr + hal_espressif + picolibc as siblings
west zephyr-export
```

`west update` clones Zephyr next to this repo, not inside it — nothing to gitignore.
Pin the Zephyr `revision` in `west.yml` to match your installed Zephyr SDK.

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
