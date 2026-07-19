# M1 Status Handoff — 2026-07-19

**Milestone:** M1 — Zephyr bring-up + 2-node LoRa exchange (tracer bullet).
**Branch:** `feat/zephyr-m1-bringup` · **PR:** [#40](https://github.com/amirna2/RadioMesh/pull/40) (OPEN, MERGEABLE).
**Bottom line:** M1's goal is achieved and hardware-verified. The PR is functionally ready to merge; the open items below are follow-ups, not M1 blockers.

## M1 goal vs. result

| Success criterion | Result |
|---|---|
| Two Zephyr nodes exchange a `RadioMeshPacket` over LoRa | **Met** — receiver logs `RX … crc=OK` from a separate transmitter |
| Driven by the co-compiled, platform-neutral core (`src/`) | **Met** — same packet + CRC code as Arduino/native |
| Wire-compatible PHY (D5) | **Met** — pinned to the Arduino XIAO preset in `common/lora_phy.h` |
| Arduino/PIO build unaffected (M0 gates) | **Met** — `pio test -e native` 4/4, `pio run -e seeed_xiao_esp32s3` SUCCESS |

## Verified on hardware (macOS, XIAO ESP32-S3 + Wio-SX1262)

- `rm_tx: TX … ret=0` — over-the-air transmit (dedicated `tx` example).
- `rm_rx: RX len=38 … topic=0x10 dataLen=3 crc=OK` — live decode + CRC (dedicated `rx` example).
- Both examples compile **warning-free**.
- Dev tooling exercised: `make help / setup / boards / build (tx+rx) / flash (rx) / monitor / run`.
- `rmon.py` runs under system `python3` (no venv, no pyserial); pyflakes-clean.

## Not verified (honest gaps — none block M1's goal)

| Item | Status | Note |
|---|---|---|
| Linux end-to-end | Not run | Cross-platform by construction (`/dev/ttyACM*` + `termios`); no Linux board tested here |
| `make bootstrap` full run | Dry-run only (`make -n`) | Would re-download Zephyr on this set-up machine; underlying commands are the ones already used here |
| `make flash ROLE=tx` via make | Not separately run | Identical code path to the verified `flash ROLE=rx` |
| Single-capture new-`tx` → new-`rx` | Not captured | Both halves verified independently; interop is transitive |
| `loramac-node` removal from `west.yml` | Deferred | Unused under the NATIVE backend; needs a `west update` + rebuild to confirm |

## PR #40 — ready to merge?

**Yes for M1's scope.** The milestone's success criteria are met and hardware-verified, the code is warning-free, M0 gates are green, and the branch is mergeable. The gaps above are follow-ups. Merge decision is the maintainer's.

Contents: T2 `west.yml`; module glue (`zephyr/`); `common/` (shared overlay + PHY); `examples/{tx,rx}`; carried `gpio_esp32` patch; dev tooling (`Makefile`, `tools/rmon.py`, `DEVELOPING.md`); two guarded `src/` edits (logger + `<algorithm>`).

## What's next

1. **M2 — routing** (per the M0–M8 decomposition): bring `PacketRouter`/`RoutingTable` onto the Zephyr build.
2. **Follow-ups** (can trail M1): verify on Linux hardware; run `make bootstrap` on a fresh machine; remove `loramac-node`; optional single-screen `tx`→`rx` capture.

**Resume from:** this file + `project_zephyr_port.md` (memory).
