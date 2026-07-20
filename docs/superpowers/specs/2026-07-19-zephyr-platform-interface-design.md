# Zephyr Port — Platform Interface Specification

**Date:** 2026-07-19 · **Status: AWAITING REVIEW — no code changes until approved.**
**Scope:** defines EXACTLY what the portable RadioMesh library consumes from the platform, and how each item is provided per platform. Companion to the tracer-bullet roadmap; the M2 implementation plan will be revised to conform to this document after review.

## Principles

1. **Structural separation.** Platform code lives in platform-owned files, selected by the build system or by the single established switch (`Options.h`). **No new `#ifdef` blocks inside logic files** (framework/protocol/.cpp bodies).
2. **Zero architectural change.** The interfaces below are RadioMesh's *existing* seams — nothing is added or hoisted.
3. **Arduino byte-identical.** Every change lists its Arduino diff; gates (`pio test -e native`, seeed + heltec builds) verify on every commit.

## The platform interface (measured, complete)

### P1 — Kernel services

Consumed via free functions that `Arduino.h` provides today (resolved through `Options.h`, which every file includes transitively via `Definitions.h`/`Logger.h`):

| Symbol | Consumers (exact) |
|---|---|
| `millis()` | `RoutingTable.cpp` ×4 (route aging), `InclusionController.cpp` ×3 (protocol timeouts), `DeviceBuilder.cpp:21` (boot wait), `LoraRadio.cpp:188,193` (TX timing log), `Utils.cpp:48` (RNG fallback) |
| `random(long)` | `KeyManager.cpp:79` (network key), `InclusionController.h:191` (nonce), `Utils.cpp:60` (uuid), `Utils.h:163` (Arduino branch only) |
| `delay(ms)` | tests (`test_InclusionProtocol`) and examples only — not the library |
| `analogRead(pin)` | `Utils.cpp:33,36` (`simpleRNG` entropy source) |
| `randomSeed()` | `Utils.h:161` — already inside `RM_ARDUINO_BUILD` branch; Arduino-only, nothing to port |

**Provision:** `Options.h` is already the platform switch (`ARDUINO >= 100` → `Arduino.h` | generic). Its generic branch gains **one structural include** of a dedicated platform file — all shims live there, not in `Options.h` and not in logic files:

```cpp
// Options.h, generic branch — the ONLY edit to a shared header:
#if defined(__ZEPHYR__)
#include <common/inc/platform/PlatformZephyr.h>
#endif
```

- New file `src/common/inc/platform/PlatformZephyr.h`: inline `millis` → `k_uptime_get_32()`, `delay` → `k_msleep()`, `random(n)` → `sys_rand32_get() % n`, `analogRead` → entropy stand-in. Included by nothing on Arduino; invisible to PIO builds.
- Logic files: **zero changes** — calls resolve exactly as they do on Arduino.
- Cleanup (1-line deletions, not guards): `RoutingTable.cpp:2`, `EncryptionService.cpp:5`, `KeyManager.cpp:1` each carry a redundant `#include <Arduino.h>` (Options.h already provides it on Arduino; EncryptionService uses no Arduino symbol at all). Deleting keeps Arduino builds identical — verified by gates.

### P2 — Chip-unique identity

| Symbol | Consumer | Arduino provider |
|---|---|---|
| 64-bit chip id (seed for the deterministic Curve25519 key) | `KeyManager.cpp generateDeterministicKey()` | `ESP.getEfuseMac()` inside the **existing** `#ifdef ESP32` guard |

**Provision — decision for review:**
- **(a) Extend the existing guard in place:** `#elif defined(__ZEPHYR__)` → `hwinfo_get_device_id()`. Smallest diff; but it is a platform branch inside framework logic (extends one that already exists there).
- **(b) Platform function:** `PlatformZephyr.h` (and the Arduino side of the switch) provide `rmChipId()`; `KeyManager` calls it unconditionally. Structurally clean; moves the existing `#ifdef ESP32` block out of `KeyManager.cpp` (touches the shipping Arduino path — behavior identical, gate-verified).

Either way the Zephyr value is per-chip-unique (matching the efuse-MAC semantics); the naked `seed = 12345` fallback would give **identical private keys on every board** — not acceptable even for a tracer.

### P3 — Console initialization

| What | Consumer | Arduino provider |
|---|---|---|
| Bring up the log console at builder start | `DeviceBuilder::start()` (`while (!Serial && millis() < 10000); Serial.begin(115200);`) | Arduino `Serial` |

**Provision — decision for review:**
- **(a) Guard in place** (`#ifdef RM_ARDUINO_BUILD` around the block) — new ifdef in framework logic; violates Principle 1.
- **(b) Platform function `rmConsoleInit()`:** both platform headers provide it (Arduino: the exact Serial block moves there verbatim; Zephyr: no-op — console is up, USB-JTAG enumeration is an app/log-config concern as in M1). `DeviceBuilder::start()` calls one portable function. Recommended.

### P4 — Radio (the hardware interface, already defined by the code)

The radio platform contract **is the existing concrete `LoraRadio` class** — its public API, consumed by `Device` and `PacketRouter` (`LoraRadio::getInstance()` at `PacketRouter.cpp:125`):

```
setup(params) / setup() / setParams / getParams / isRadioSetup
sendPacket(vector<byte>&) / startTransmitPacket(byte*, int) / startReceive()
readReceivedData(vector<byte>*) / checkAndClearRxFlag() / checkAndClearTxFlag()
getRadioStateError() / getRSSI() / getSNR() / standBy() / sleep()
```

**Provision:** one backend `.cpp` per platform, structurally separated — Arduino keeps `src/hardware/src/radio/LoraRadio.cpp` (RadioLib) untouched; Zephyr adds `ports/zephyr/hal/lora_radio_zephyr.cpp` over the Zephyr LoRa API (M1-proven; DT owns pins; `CR 4/7` + `preamble 8` hardcoded to the D5 wire contract since `LoraRadioParams` never carried them). The build system selects the backend (PIO never sees `ports/`; the Zephyr module never lists the RadioLib `.cpp`).
**One header accommodation:** `LoraRadio.h` holds `<RadioLib.h>` + `std::unique_ptr<SX1262>` — the private backend state gets an `RM_ARDUINO_BUILD` block (Arduino members) / else (Zephyr member: `const struct device*`). This is a HAL *declaration*, the same category as the existing `Options.h`/`Logger.h` guards — not logic. Public API untouched. (Alternative if even that guard is unwanted: pimpl — but that edits the shipping Arduino `.cpp`, so it is NOT recommended.)

### P5 — Byte storage

The storage platform contract is the ESP32-Arduino **`EEPROM` API** (`begin(size)→bool, read(addr), write(addr,val), commit()→bool, end()`), consumed **only** by `EEPROMStorage.cpp` (its header is already platform-clean — includes only `IByteStorage.h`).

**Provision:** `ports/zephyr/hal/arduino_compat/EEPROM.h` — a facade class with the identical API backed by one NVS blob on `storage_partition` — supplied purely by the Zephyr include path. `EEPROMStorage.cpp` co-compiles **unchanged** (same 550 lines, same layout, same `test_EEPROMStorage`). **Zero source changes, zero ifdefs.**

### P6 — Crypto primitives

`Crypto.h/AES.h/CTR.h/Curve25519.h/SHA256.h` from operatorfoundation/Crypto 0.4.0 — the used modules contain zero Arduino references (grep-verified). Co-compile the same pinned sources on Zephyr (fetch mechanics: west-manifest pin vs vendored subset — decision at review). Exclusions: `RNG.cpp` (unused; its `#include <RNG.h>` in `KeyManager.cpp` is dead and gets deleted), `AESEsp32.cpp` (ESP-IDF hardware path; software AES is wire-identical).

### P7 — Logging

Already ported: M0/M1 `Logger.h` guards (`__ZEPHYR__` → printk path, Arduino byte-identical). Precedent for the P1 mechanism.

## Complete file-touch inventory (for review)

| File | Change | Arduino diff |
|---|---|---|
| `src/common/inc/Options.h` | +3 lines: structural include of `PlatformZephyr.h` in generic branch | none (generic branch) |
| `src/common/inc/platform/PlatformZephyr.h` | **new** — all P1 shims | n/a (never compiled) |
| `src/core/protocol/src/routing/RoutingTable.cpp` | delete redundant `#include <Arduino.h>` | none (transitively provided) |
| `src/core/protocol/src/crypto/EncryptionService.cpp` | delete `#include <Arduino.h>` (no symbol used) | none |
| `src/framework/device/src/KeyManager.cpp` | delete `#include <Arduino.h>` + dead `<RNG.h>`; P2 per decision (a) or (b) | none / behavior-identical |
| `src/framework/builder/src/DeviceBuilder.cpp` | P3 per decision (a) or (b) | none / behavior-identical (Serial block verbatim) |
| `src/hardware/inc/radio/LoraRadio.h` | guarded private backend members (P4) | none (Arduino branch = today's members) |
| `ports/zephyr/hal/lora_radio_zephyr.cpp` | **new** — Zephyr radio backend | n/a |
| `ports/zephyr/hal/arduino_compat/EEPROM.h` + `eeprom_nvs_backend.cpp` | **new** — storage facade (P5) | n/a |
| `zephyr/CMakeLists.txt`, `west.yml`, examples/tests under `ports/zephyr/` | build glue, apps, on-target test runner | n/a |

**Not touched, ever (this milestone):** `Device.h/.cpp`, `PacketRouter.*`, `RoutingTable.h`, `InclusionController.*`, all `framework/interfaces/*`, `Packet.*`, `MicService.*`, `AesCrypto.*`, `AesCmac.*`, `EEPROMStorage.cpp/.h`, `DeviceStorage.h`, Arduino `LoraRadio.cpp`.

## Decisions requested

1. **P2 chip id:** (a) extend existing `#ifdef ESP32` chain in `KeyManager.cpp`, or (b) `rmChipId()` on the platform surface. 
2. **P3 console init:** (a) guard in place, or (b) `rmConsoleInit()` on the platform surface (recommended).
3. **P6 crypto fetch:** west-manifest pin or vendored subset (`lib/crypto/`, provenance README).
4. **P4 header accommodation:** guarded private members in `LoraRadio.h` (recommended) — acceptable?

## Process (fixing what went wrong today)

This document → **your review** → M2 implementation plan revised to match → execution task-by-task, with the V0/V1/V2 gates as report-back checkpoints. No `src/` edit lands without tracing to a line in this spec.
