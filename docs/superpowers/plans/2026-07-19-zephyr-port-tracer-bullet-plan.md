# RadioMesh Zephyr Port — Tracer-Bullet Plan

**Date:** 2026-07-19
**Supersedes:** the M2–M8 horizontal decomposition in `2026-07-18-zephyr-port-milestone0-design.md` (its §1 project-level decisions stand: dual-hat monorepo, co-compilation, D5 wire-compat, RM_ARDUINO_BUILD guards).
**Hardware:** Seeed XIAO ESP32-S3 + Wio-SX1262, exclusively — both hats (Arduino RadioMesh already targets it: `LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262`, `test_seeed_xiao_esp32s3`).

## Porting principles (rev. 2 — see the platform interface spec)

**Complete the existing clean architecture; never emulate Arduino elsewhere.** The core (`src/common`, `src/core`, `src/framework`) becomes 100% platform-free and compiles universally (Arduino, Zephyr, Linux host, any C++17). Kernel services go through the `RadioMeshPlatform` contract (`src/common/inc/platform/RadioMeshPlatform.h`), implemented per platform (`src/hardware/src/platform/ArduinoPlatform.cpp`, `ports/zephyr/hal/src/platform/ZephyrPlatform.cpp`). `IRadio` is completed with the TX/RX surface and injected (`PacketRouter::setRadio`, mirroring its existing `setCrypto` pattern); Zephyr implements the interfaces natively (`ZephyrLoraRadio : IRadio`, `ZephyrNVSStorage : IByteStorage`) in `ports/zephyr/hal/`. No platform `#ifdef`s in logic; no redesign of the protocol, wire format, or public application API. Authoritative design: `docs/superpowers/specs/2026-07-19-zephyr-platform-interface-design.md`.

One tracer bullet: a thin thread through the **entire** RadioMesh system at once — `DeviceBuilder` → `Device` → inclusion (ECDH) → encryption/MIC → `PacketRouter`/`RoutingTable`/dedup → radio → OTA — validated by a **working mini cluster**. No capability sequencing: a mesh library whose first milestone can't mesh has traced nothing. After the bullet works, milestones *thicken* it — each verified on the running cluster.

M0–M1 were bootstrap: de-Arduinized packet core + native test env (M0); proof Zephyr drives the SX1262 + build/flash/monitor tooling (M1). The M1 `tx`/`rx` examples bypass the library and are retired by the bullet.

## Why this is feasible: the measured coupling surface

Full read of the library compile-closure (~5.5k lines). The **entire** remaining Arduino surface is:

| File | Coupling | Treatment (per the interface spec) |
|---|---|---|
| `common/utils/Utils.h/.cpp` | `random()`, `millis()`, `analogRead()` entropy | call sites → `RadioMeshPlatform::`; `simpleRNG` moves to the Arduino port |
| `core/.../crypto/EncryptionService.cpp` | `#include <Arduino.h>` — **uses no Arduino symbol** | delete the include |
| `core/.../routing/RoutingTable.cpp` | `<Arduino.h>` + 4× `millis()` (route aging) | call sites → `RadioMeshPlatform::millis()` |
| `framework/.../InclusionController.h/.cpp` | 3× `millis()` (timeouts), `random(256)` (nonce) | `RadioMeshPlatform::millis()/randomBytes()` |
| `framework/.../KeyManager.cpp` | `<Arduino.h>`, `random(256)`, `ESP.getEfuseMac()` | `RadioMeshPlatform::randomBytes()/chipId()`; efuse code moves to the Arduino port |
| `framework/.../DeviceBuilder.cpp` | `Serial` + `millis()` wait in `start()` | `RadioMeshPlatform::consoleInit()`; Serial block moves to the Arduino port |
| `hardware/.../radio/LoraRadio.h/.cpp` | RadioLib backend; TX/RX missing from `IRadio` | complete `IRadio` (+7 virtuals, `override` on `LoraRadio`); `Device`/`PacketRouter` consume `IRadio*`; Zephyr: `ports/zephyr/hal` `ZephyrLoraRadio : IRadio` |
| `hardware/.../storage/eeprom/EEPROMStorage.cpp` | Arduino `EEPROM` API (Arduino-port internals) | untouched; `Device` consumes `IByteStorage*`; Zephyr: `ZephyrNVSStorage : IByteStorage` over NVS |

Everything else — `Device.cpp`, `PacketRouter.cpp`, `MicService`, `AesCrypto`, `AesCmac`, `PacketTracker`, `Packet`, `DeviceStorage` — co-compiles as-is.

**Crypto library:** of the `operatorfoundation/Crypto` 0.4.0 modules RadioMesh uses (`AES*`, `CTR`, `Crypto`, `Curve25519`, `SHA256`, `AESCommon`, cipher base classes), **zero contain Arduino references** (grep-verified; only the unused `RNG.cpp` and hardware-path `AESEsp32.cpp` do). The same sources co-compile on Zephyr → ECDH/AES-CTR/CMAC **byte-identical by construction**. No PSA rewrite.

Protocol facts that force the whole-system bullet (no shortcut exists):
- `Device::initialize()` **unconditionally** requires storage + `InclusionController` (`Device.cpp:519-539`); `run()` calls `checkProtocolTimeouts()` every loop.
- `sendData()` is inclusion-gated: a STANDARD device must be INCLUDED to send app topics.
- A relay must also be INCLUDED: forwarding happens only after MIC verification, which needs the network key.
- App payloads are AES-encrypted + CMAC'd once included — crypto is on the main path.

## M2 — verification ladder (strict order)

### V0 — FIRST: the full library builds for Zephyr / XIAO

`west build -b xiao_esp32s3/esp32s3/procpu` compiles the **complete** RadioMesh library — Device, DeviceBuilder, InclusionController, KeyManager, protocol stack, crypto, plus the `ports/zephyr/hal` implementations (`ZephyrPlatform`, `ZephyrLoraRadio`, `ZephyrNVSStorage`) — with `RM_NO_DISPLAY`/`RM_NO_WIFI` (existing guards) and zero warnings. Arduino gates stay green (`pio run -e seeed_xiao_esp32s3`, heltec, `pio test -e native`).

### V1 — SECOND: every existing unit test runs ON the XIAO board (not native)

Mirror of the existing `test_seeed_xiao_esp32s3` practice: co-compile Unity + the **same** `test/test_*` sources into Zephyr test firmware, execute on the board, parse PASS/FAIL over serial (`make test`-style target). Suites naming concrete platform classes (`test_LoraRadio`, `test_EEPROMStorage`) exercise the platform's implementation via the `RadioMeshPlatform` factories; only those instantiation lines are adapted.

| Suite | On Zephyr/XIAO | Note |
|---|---|---|
| test_Packet, test_PacketTracker, test_Crypto, test_DynamicKeyExchange, test_InclusionProtocol, test_Example | **run** | pure protocol/crypto |
| test_Device, test_DeviceBuilder | **run** | full framework on board |
| test_EEPROMStorage | **run** | against `ZephyrNVSStorage` via the platform factory |
| test_LoraRadio | **run** | against `ZephyrLoraRadio` via the platform factory |
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

- **W1 — `RadioMeshPlatform` contract + Arduino port + core conversion**: contract header in `src/common/inc/platform/`; `ArduinoPlatform.cpp` (Arduino code moved verbatim); all core call sites converted; core is then platform-free (purity grep enforced).
- **W2 — `IRadio` completion + `ZephyrLoraRadio`**: +7 pure virtuals on `IRadio`; `Device`/`PacketRouter` consume `IRadio*` via the platform factory; `ports/zephyr/hal/{inc,src}/radio/ZephyrLoraRadio` over the M1-proven LoRa API (DT pins; CR 4/7 + preamble 8 fixed per D5). Async RX feeds the same rx/tx flags `Device::run()` polls.
- **W3 — `ZephyrNVSStorage : IByteStorage`**: native NVS implementation in `ports/zephyr/hal/{inc,src}/storage/`; `Device` consumes `IByteStorage*`; Arduino `EEPROMStorage` untouched.
- **W4 — crypto co-compilation**: pinned Crypto 0.4.0 sources in the Zephyr build (manifest vs vendored subset — decision at execution), excluding `RNG.cpp`/`AESEsp32.cpp`.
- **W5 — V0 build target** (whole library) → **W6 — on-target test runner + suites (V1)** → **W7 — cluster examples** `ports/zephyr/examples/hub|standard` on the real API, retiring `tx`/`rx` → **W8 — Gates A/B (V2)**.

### Decisions at execution start

1. **Crypto sources on Zephyr** — west-manifest pin vs vendored subset; whichever keeps the version verifiably identical to PlatformIO's.
2. **Test-runner mechanics** — per-suite firmware (exact `pio test` mirror) vs single combined runner; decided in the implementation plan.

### Explicit non-goals (replicate as-is, do not "fix")

Deterministic chipID-seeded private key, static all-zero IV, `random()`-based nonces — ported behavior-equivalent; deferred security audit. No redesign of the protocol, wire format, or public application API (interface *completion* per the spec is in scope). Display / WiFi / portal stay compiled out.

## After the bullet — thickening (each verified on the live cluster)

- **H1 — test depth**: extend on-target + native coverage (route aging with injectable time, CMAC/ECDH vectors); wire `test_PacketTracker` into the native filter.
- **H2 — product polish**: Zephyr sample apps + docs to shipping quality; RadioMesh usable as a Zephyr library by outsiders.
- **H3 — peripheral parity (demand-driven)**: display, WiFi/portal on Zephyr.

## Verification model

Every commit: both PlatformIO gates + Zephyr builds, zero warnings. Milestone: V0 → V1 → V2 in order. Bite-sized implementation plan (writing-plans) authored when M2 execution starts, against this document.

**Resume from:** this file + `project_zephyr_port.md` (memory).
