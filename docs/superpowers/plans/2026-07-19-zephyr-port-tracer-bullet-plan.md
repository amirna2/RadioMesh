# RadioMesh Zephyr Port — Tracer-Bullet Plan

**Date:** 2026-07-19
**Supersedes:** the M2–M8 horizontal decomposition in `2026-07-18-zephyr-port-milestone0-design.md` (its §1 project-level decisions stand: dual-hat monorepo, co-compilation, D5 wire-compat, RM_ARDUINO_BUILD guards).
**Hardware:** Seeed XIAO ESP32-S3 + Wio-SX1262, exclusively — both hats (Arduino RadioMesh already targets it: `LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262`, `test_seeed_xiao_esp32s3`).

## Porting principles

**This is a porting exercise, not a redesign. Close to zero architectural changes.** RadioMesh's existing abstraction level is sufficient: the concrete HAL classes (`LoraRadio`, `EEPROMStorage`) ARE the platform contract. The port supplies **Zephyr backend implementations of the same classes** — new `.cpp` files selected by the build system, guarded members only where a HAL header holds platform types. `Device`, `PacketRouter`, `DeviceBuilder`, the interfaces, the protocol stack: **unchanged**. No interface hoisting, no dependency injection, no seam refactors.

One tracer bullet: a thin thread through the **entire** RadioMesh system at once — `DeviceBuilder` → `Device` → inclusion (ECDH) → encryption/MIC → `PacketRouter`/`RoutingTable`/dedup → radio → OTA — validated by a **working mini cluster**. No capability sequencing: a mesh library whose first milestone can't mesh has traced nothing. After the bullet works, milestones *thicken* it — each verified on the running cluster.

M0–M1 were bootstrap: de-Arduinized packet core + native test env (M0); proof Zephyr drives the SX1262 + build/flash/monitor tooling (M1). The M1 `tx`/`rx` examples bypass the library and are retired by the bullet.

## Why this is feasible: the measured coupling surface

Full read of the library compile-closure (~5.5k lines). The **entire** remaining Arduino surface is:

| File | Coupling | Treatment |
|---|---|---|
| `common/utils/Utils.h/.cpp` | `random()`, `millis()` in RNG fallback | extend M0 guards (rng/time shims) |
| `core/.../crypto/EncryptionService.cpp` | `#include <Arduino.h>` — **uses no Arduino symbol** | drop/guard the include |
| `core/.../routing/RoutingTable.cpp` | `<Arduino.h>` + 4× `millis()` (route aging) | time shim |
| `framework/.../InclusionController.h/.cpp` | 3× `millis()` (timeouts), `random(256)` (nonce) | time + rng shims |
| `framework/.../KeyManager.cpp` | `<Arduino.h>`, `random(256)`, `ESP.getEfuseMac()` (already `#ifdef ESP32`) | rng shim + Zephyr `hwinfo` device-id branch |
| `framework/.../DeviceBuilder.cpp` | `Serial` + `millis()` wait in `start()` | guarded console-init shim |
| `hardware/.../radio/LoraRadio.h/.cpp` | RadioLib backend + `<RadioLib.h>`/`SX1262` member in header | **Zephyr backend `.cpp` for the same class**; RM_ARDUINO_BUILD-guarded private members (public API untouched → `PacketRouter.cpp:125` and `Device` work as-is) |
| `hardware/.../storage/eeprom/EEPROMStorage.cpp` | Arduino `EEPROM` API (**header is clean** — includes only `IByteStorage.h`) | **Zephyr backend `.cpp` for the same class** over NVS; zero header change |

Everything else — `Device.cpp`, `PacketRouter.cpp`, `MicService`, `AesCrypto`, `AesCmac`, `PacketTracker`, `Packet`, `DeviceStorage` — co-compiles as-is.

**Crypto library:** of the `operatorfoundation/Crypto` 0.4.0 modules RadioMesh uses (`AES*`, `CTR`, `Crypto`, `Curve25519`, `SHA256`, `AESCommon`, cipher base classes), **zero contain Arduino references** (grep-verified; only the unused `RNG.cpp` and hardware-path `AESEsp32.cpp` do). The same sources co-compile on Zephyr → ECDH/AES-CTR/CMAC **byte-identical by construction**. No PSA rewrite.

Protocol facts that force the whole-system bullet (no shortcut exists):
- `Device::initialize()` **unconditionally** requires storage + `InclusionController` (`Device.cpp:519-539`); `run()` calls `checkProtocolTimeouts()` every loop.
- `sendData()` is inclusion-gated: a STANDARD device must be INCLUDED to send app topics.
- A relay must also be INCLUDED: forwarding happens only after MIC verification, which needs the network key.
- App payloads are AES-encrypted + CMAC'd once included — crypto is on the main path.

## M2 — verification ladder (strict order)

### V0 — FIRST: the full library builds for Zephyr / XIAO

`west build -b xiao_esp32s3/esp32s3/procpu` compiles the **complete** RadioMesh library — Device, DeviceBuilder, InclusionController, KeyManager, protocol stack, crypto, both Zephyr backends — with `RM_NO_DISPLAY`/`RM_NO_WIFI` (existing guards) and zero warnings. Arduino gates stay green (`pio run -e seeed_xiao_esp32s3`, heltec, `pio test -e native`).

### V1 — SECOND: every existing unit test runs ON the XIAO board (not native)

Mirror of the existing `test_seeed_xiao_esp32s3` practice: co-compile Unity + the **same** `test/test_*` sources into Zephyr test firmware, execute on the board, parse PASS/FAIL over serial (`make test`-style target). Test sources get the same shim treatment as the library (only 2 of 13 suites have Arduino references).

| Suite | On Zephyr/XIAO | Note |
|---|---|---|
| test_Packet, test_PacketTracker, test_Crypto, test_DynamicKeyExchange, test_InclusionProtocol, test_Example | **run** | pure protocol/crypto |
| test_Device, test_DeviceBuilder | **run** | full framework on board |
| test_EEPROMStorage | **run** | against the NVS backend |
| test_LoraRadio | **run** | against the Zephyr backend |
| test_CustomDisplay, test_WifiAccessPoint, test_WifiConnector | excluded | subsystems compiled out (`RM_NO_DISPLAY`/`RM_NO_WIFI`) |

### V2 — the tracer bullet: mini cluster (all XIAO)

Mirror of `examples/DeviceInclusion` (MiniHub + StandardDevice — the canonical app).

**Gate A — mixed cluster:** XIAO-**Arduino** MiniHub (today's example, XIAO preset) + XIAO-**Zephyr** standard relay + XIAO-**Zephyr** standard leaf:
1. Hub opens inclusion → both Zephyr nodes complete full ECDH inclusion and persist INCLUDED.
2. Leaf telemetry (topic 0x10, AES + CMAC) transits the relay to the hub (multi-hop forced via TX-power/antenna attenuation).
3. Dedup: hub sees each packet once.
4. Leaf power-cycle → returns INCLUDED from NVS, resumes without re-inclusion.

Gate A proves **live Arduino↔Zephyr wire-compat** (D5) on identical hardware.

**Gate B — role swap:** XIAO-**Zephyr** hub (inclusion trigger via console/timer instead of the portal) includes an Arduino standard node and a Zephyr leaf — hub-side paths (network-key generation, INCLUDE_RESPONSE/SUCCESS) on Zephyr.

Requires 3 XIAO kits (2 on hand — third board, or run Gate A reduced to hub+leaf first and add the relay hop when the third arrives).

### Work items feeding the ladder

- **W1 — platform shims** (`common/`, M0 guard pattern): time (`millis`-equiv → `k_uptime_get_32`), RNG, console init, device-unique-id (→ `hwinfo`). Applied to the shim files above; Arduino byte-identical.
- **W2 — `LoraRadio` Zephyr backend**: same class, Zephyr `.cpp` over the M1-proven LoRa API (DT pins; params supply band/BW/SF/power/sync). Async RX feeds the same rxDone/txDone flags `Device::run()` polls. Guarded private members in the header.
- **W3 — `EEPROMStorage` Zephyr backend**: same class, `.cpp` over NVS. `writeAndCommit` maps naturally.
- **W4 — crypto co-compilation**: pinned Crypto 0.4.0 sources in the Zephyr build (manifest vs vendored subset — decision at execution), excluding `RNG.cpp`/`AESEsp32.cpp`.
- **W5 — V0 build target** (whole library) → **W6 — on-target test runner + suites (V1)** → **W7 — cluster examples** `ports/zephyr/examples/hub|standard` on the real API, retiring `tx`/`rx` → **W8 — Gates A/B (V2)**.

### Decisions at execution start

1. **Crypto sources on Zephyr** — west-manifest pin vs vendored subset; whichever keeps the version verifiably identical to PlatformIO's.
2. **Test-runner mechanics** — per-suite firmware (exact `pio test` mirror) vs single combined runner; decided in the implementation plan.

### Explicit non-goals (replicate as-is, do not "fix")

Deterministic chipID-seeded private key, static all-zero IV, `random()`-based nonces — ported behavior-identical; deferred security audit. No interface/architecture changes. Display / WiFi / portal stay compiled out.

## After the bullet — thickening (each verified on the live cluster)

- **H1 — test depth**: extend on-target + native coverage (route aging with injectable time, CMAC/ECDH vectors); wire `test_PacketTracker` into the native filter.
- **H2 — product polish**: Zephyr sample apps + docs to shipping quality; RadioMesh usable as a Zephyr library by outsiders.
- **H3 — peripheral parity (demand-driven)**: display, WiFi/portal on Zephyr.

## Verification model

Every commit: both PlatformIO gates + Zephyr builds, zero warnings. Milestone: V0 → V1 → V2 in order. Bite-sized implementation plan (writing-plans) authored when M2 execution starts, against this document.

**Resume from:** this file + `project_zephyr_port.md` (memory).
