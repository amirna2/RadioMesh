# Zephyr Port — Milestone 0: De-Arduino-ify the Core Packet Slice

**Date:** 2026-07-18
**Status:** Design approved; ready for implementation planning
**Repo:** RadioMesh (this repo — changes land here, in the current PlatformIO/Arduino project)

---

## 1. Project context & decisions (preamble)

This milestone is the first deliverable of a larger effort: producing a **Zephyr RTOS
port of RadioMesh** targeting the Seeed XIAO ESP32-S3 + Wio SX1262 kit. The port serves
two goals — deepening Zephyr skills (used professionally at the author's employer) using a
known-quantity protocol as the vehicle, and repositioning RadioMesh toward
commercial/industrial users rather than hobbyists only.

The following project-level decisions were made during brainstorming and constrain every
milestone. They are recorded here so they are not lost; later milestones get their own
specs but inherit these.

### D1 — Monorepo, evolved in place, no rename, no user disruption
The Zephyr port lives in the **current RadioMesh repo** (same name, same URL). It is **not**
a new repo and **not** a multi-repo split. Existing PlatformIO users must remain
undisturbed: `library.json`, `include/`, `src/`, `examples/`, `pio-config/` stay where they
are and keep working. A multi-repo split (core + per-platform ports) is a *possible future*
migration if independent port maintainers or divergent release cadences appear — not now.

### D2 — Shared core by co-compilation (not a submodule or copy)
The platform-neutral protocol code (`src/core`, `src/common`, neutral parts of
`src/framework`) is shared between the Arduino and Zephyr builds by having **both build
systems compile the same physical source files**. There is no submodule, no vendored copy,
no separate core package. Consequence: wire-compatibility between Arduino and Zephyr nodes
is guaranteed **by construction** — there is exactly one implementation of `Packet`, CRC,
crypto, and routing on disk.

### D3 — Dual-hat repo layout
The repo carries two independent build manifests that never conflict:
- **PlatformIO** reads `library.json` (root) and compiles `src/` recursively. It never sees
  `zephyr/` or `ports/`.
- **Zephyr** reads `zephyr/module.yml` (root) and a `west.yml` manifest; its CMake
  co-compiles the neutral `src/` slice plus the Zephyr HAL in `ports/zephyr/`.

```
RadioMesh/
├── library.json            # PIO reads this. UNCHANGED by the port.
├── include/RadioMesh.h     # Arduino umbrella header. UNCHANGED.
├── src/
│   ├── common/             ┐ platform-neutral → compiled by BOTH builds
│   ├── core/               ┘   (de-Arduino-ify happens here, guarded)
│   ├── framework/          #   logic (neutral) + hardware wiring (Arduino)
│   ├── hardware/           # Arduino HAL (RadioLib, EEPROM, U8g2, portal)
│   └── main.cpp            # Arduino build only
├── examples/               # Arduino — UNCHANGED
├── platformio.ini          # Arduino — a `native` env is ADDED here in M0
├── pio-config/             # Arduino build fragments — UNCHANGED (may gain a native fragment)
├── zephyr/module.yml       # NEW (M1). west reads this. PIO ignores it.
└── ports/zephyr/           # NEW (M1). Invisible to PIO.
```
Platform separation is **structural** (by directory) and, for shared files, **guarded by
preprocessor macro** — never `#ifdef ARDUINO` scattered ad hoc through HAL logic.

### D4 — Decomposition roadmap
The full port is multi-subsystem and too large for one spec. It is decomposed into
dependency-ordered milestones, each its own spec → plan → implementation:

0. **De-Arduino-ify the core packet slice + add a host build gate** ← *this spec*
1. Zephyr bring-up + two-node LoRa packet exchange (tracer bullet)
2. Routing + relay (RoutingTable, PacketRouter, dedup)
3. Secure messaging (AES-CTR, CMAC/MIC; rweather-vs-PSA crypto-backend decision)
4. Persistent storage HAL (NVS / settings)
5. Inclusion + KeyManager (Curve25519 ECDH; depends on 3 + 4)
6. Device / DeviceBuilder framework parity (includes fixing the leaky `IRadio` abstraction)
7. Display HAL (optional)
8. WiFi + DevicePortal (highest uncertainty; may become a BLE/USB-config rethink)

There is deliberately **no big upfront "extract the whole core" refactor**. Each milestone
de-Arduino-ifies only the slice it needs, in place and guarded.

### D5 — Interop preserved
A Zephyr node and an Arduino node must be able to coexist on the same physical mesh (same
packet framing, CRC, and — once M3 lands — byte-identical crypto/MIC). This is a direct
consequence of D2.

---

## 2. Milestone 0 — Goal

Make the **packet slice** of the protocol core compile with **zero Arduino dependency**,
with every change **guarded** so the Arduino build's preprocessor output is byte-identical,
and prove correctness from **both** sides (Arduino unchanged; non-Arduino compiles + passes
tests).

This milestone requires no Zephyr toolchain, no board, and no devicetree. It is a
self-contained improvement to the current repo and the prerequisite that de-risks M1.

---

## 3. Scope — the "packet slice"

The minimal set of neutral source needed to build and round-trip a `RadioMeshPacket`:

- `src/core/protocol/inc/packet/Packet.h` — `RadioMeshPacket` (serialize/parse/CRC field)
- `src/common/utils/RadioMeshCrc32.h` — CRC32 (currently `#include <Arduino.h>` at line 3)
- `src/common/inc/Definitions.h` — uses `byte` (e.g. line 397); topic/type enums
- `src/common/inc/Options.h` — the `RM_ARDUINO_BUILD` / `RM_GENERIC_BUILD` gate (lines 1–11)
- `src/common/inc/Logger.h` — has an existing `#if defined(ARDUINO) … #else std::cout` split
- `src/common/utils/Utils.{h,cpp}` — hex helpers etc. (already free of the Arduino-isms grep set)

The exact transitive include set is confirmed during implementation; the list above is the
expected slice. Anything outside it (crypto, routing, framework, `IRadio`, HAL) is **out of
scope**.

---

## 4. Changes (all guarded by the existing platform macro)

1. **`byte` typedef for non-Arduino** — in the existing `#else /* RM_GENERIC_BUILD */` branch
   of `Options.h`, add `#include <cstdint>` and `typedef uint8_t byte;` (Arduino supplies
   `byte` via `<Arduino.h>`; the generic branch currently does not, which is the core gap).
   Include ordering must ensure `byte` is defined before `Definitions.h` uses it.

2. **`RadioMeshCrc32.h`** — replace `#include <Arduino.h>` with `#include <common/inc/Options.h>`,
   which supplies `byte` and fixed-width ints under both platform branches. No behavioral change.

3. **New `[env:native]` in `platformio.ini`** — `platform = native`, **no** `framework`,
   Unity test framework, C++17, building only the packet slice + its tests. Extends the
   existing `common_test` conventions where possible.

4. **`test/test_Packet/test_Packet.cpp`** — a new focused host test: build a
   `RadioMeshPacket`, serialize via `toByteBuffer()`, parse via the buffer constructor,
   assert field-for-field equality; assert CRC compute/verify. Must compile and run under
   the `native` env with no Arduino.

The `Logger.h` Zephyr `printk` branch is **deferred to M1** — the `native` build exercises
the existing `std::cout` branch, so no logger change is needed in M0.

---

## 5. Why two verification gates

"Still builds for Arduino" is **necessary but not sufficient**. The Arduino build keeps
`<Arduino.h>` on the include path, so a lingering Arduino dependency would compile silently
and stay hidden until Zephyr's compiler rejected it in M1. The only real proof that the
Arduino-ectomy took is a **second compile path with no Arduino at all**. Hence a host
`native` build is part of M0, not a later nicety.

Bonus: a `native` env gives the existing Arduino project fast, hardware-free unit tests for
protocol logic — value independent of the Zephyr port.

---

## 6. Success criteria

Both gates green:

1. **Arduino unchanged:** `pio run -e heltec_wifi_lora_32_V3` and
   `pio run -e seeed_xiao_esp32s3` build successfully; existing on-target test envs are
   unaffected (no source behavior change under `RM_ARDUINO_BUILD`).
2. **Non-Arduino proven:** `pio test -e native` compiles the packet slice **without
   Arduino** and passes the `test_Packet` round-trip + CRC assertions.

---

## 7. Non-goals

- No Zephyr toolchain, board, devicetree, or `ports/zephyr/` content (that is M1).
- No file relocation or directory restructure.
- No changes to crypto, routing, framework, `DeviceBuilder`, or the `IRadio` interface.
- No `Logger.h` `printk` branch (deferred to M1).
- No fixing of the leaky `IRadio` abstraction (deferred to M6, done across both platforms).

---

## 8. Risks & notes

- **Include ordering / `byte` visibility:** files that use `byte` must transitively include
  `Options.h` (or the compat definition) before use. Verify `Definitions.h`'s include order.
- **`native` platform quirks:** PlatformIO's `native` uses the host compiler; the slice must
  avoid anything host-toolchain-specific. The slice is STL + fixed-width ints, so low risk.
- **Scope creep:** the temptation is to de-Arduino-ify more than the packet slice while in
  these files. Resist — later milestones pull in the rest, just-in-time.
