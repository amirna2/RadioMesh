# RadioMesh Portability — Platform Interface Specification

**Date:** 2026-07-19 · **Status:** direction approved; each implementation step is still individually reviewed before coding.
**Goal:** make RadioMesh portable to Arduino, Zephyr, Linux, and any future OS/HW platform by **completing the existing clean architecture** — not by emulating Arduino elsewhere.

## Architectural intent (what RadioMesh already is)

Application → Framework (`DeviceBuilder`, `Device`, `InclusionController`) → Protocol (`PacketRouter`, `RoutingTable`, `Packet`, crypto services) → HAL (`framework/interfaces/`: `IRadio`, `IByteStorage`, `IDisplay`, `IWifi*`, …), with dependency injection as the declared composition mechanism (`DeviceBuilder`, `withCustomDisplay(IDisplay*)`, `DeviceStorage(IByteStorage*)`, `PacketRouter::setCrypto/setEncryptionService/setMicService`).

**The defect this port fixes is single-platform drift, not design:** raw Arduino calls (`millis`, `random`, `analogRead`, `Serial`, `ESP.getEfuseMac`) inside protocol/framework code; the TX/RX contract missing from `IRadio` (so `Device` holds `LoraRadio*` and `PacketRouter` calls `LoraRadio::getInstance()`, `PacketRouter.cpp:125`); a raw `EEPROM` API dependency inside the Arduino storage implementation.

## Principles

1. **The core compiles universally.** `src/common`, `src/core`, `src/framework` contain zero platform includes and zero platform `#ifdef`s — any C++17 toolchain, any OS, no OS.
2. **Platform code lives only in porting layers.** `src/hardware` is the Arduino port (existing location, unchanged role); `ports/zephyr/hal` is the Zephyr port; `ports/host` (future) is the Linux/host port.
3. **No emulation.** Other platforms implement RadioMesh's contracts natively (e.g. NVS behind `IByteStorage`) — never fake Arduino APIs.
4. **Complete, don't redesign.** The interface set, protocol, wire format, and public application API are unchanged. Changes are limited to finishing the abstractions the architecture already promises.
5. **Showcase consistency.** RadioMesh C++ sources/classes are PascalCase (acronyms stay caps: `ZephyrNVSStorage`, like `EEPROMStorage`), organized `inc/<domain>/` + `src/<domain>/` — everywhere, including `ports/`. Ecosystem artifacts keep their mandated names (`CMakeLists.txt`, `prj.conf`, `*.overlay`, `Kconfig`, `west.yml`).

## The three seams

### Seam 1 — Platform services contract (new, narrow)

The only genuinely new abstraction: kernel services the core needs from any platform.

```cpp
// src/common/inc/platform/RadioMeshPlatform.h — declarations only, no platform includes
namespace RadioMeshPlatform
{
uint32_t millis();                          // monotonic ms since boot
void delay(uint32_t ms);
uint32_t random(uint32_t max);              // uniform [0, max)
void randomBytes(uint8_t* buf, size_t len); // best entropy available
uint64_t chipId();                          // per-chip-unique identity
void consoleInit();                         // bring up the log console (no-op where N/A)
IRadio* loraRadio();                        // platform composition: the LoRa radio impl
IByteStorage* byteStorage();                // platform composition: the byte storage impl
} // namespace RadioMeshPlatform
```

| Implementation | File | Provides |
|---|---|---|
| Arduino | `src/hardware/src/platform/ArduinoPlatform.cpp` | Arduino `millis/delay/random`; `randomBytes` = the existing `randomSeed(simpleRNG())` + `analogRead` entropy trick, moved verbatim out of `Utils.cpp`; `chipId` = `ESP.getEfuseMac()` (moved out of `KeyManager.cpp`); `consoleInit` = the `Serial` block moved verbatim out of `DeviceBuilder.cpp`; factories return today's `LoraRadio::getInstance()` / `EEPROMStorage::getInstance()` — **Arduino behavior unchanged** |
| Zephyr | `ports/zephyr/hal/src/platform/ZephyrPlatform.cpp` | `k_uptime_get_32`, `k_msleep`, `sys_rand*` (HW TRNG), `hwinfo_get_device_id`, no-op console; factories return `ZephyrLoraRadio` / `ZephyrNVSStorage` |
| Host (future) | `ports/host/...` | `std::chrono`, `/dev/urandom`, stub/sim radio — enables full-stack tests on Linux |

Core call-site conversion (complete de-Arduinofication — the full list, measured): `RoutingTable.cpp` (4× millis), `InclusionController.cpp/.h` (3× millis + nonce `random`), `KeyManager.cpp` (`random`, chip-id; delete dead `<RNG.h>` + `<Arduino.h>`), `DeviceBuilder.cpp` (console init), `Utils.cpp/.h` (uuid random, RNG entropy — `simpleRNG`/`analogRead` move to the Arduino port; the existing `RM_ARDUINO_BUILD` split in `getRandomBytesArray` collapses to one portable path), `EncryptionService.cpp` (delete unused `<Arduino.h>`), `Logger.h` (output backend folds into the contract, removing the M1 `__ZEPHYR__` guard from core). After this: `grep -r 'Arduino|millis(|random(|analogRead|Serial' src/common src/core src/framework` hits nothing but the contract header.

### Seam 2 — Complete `IRadio` (the architecture's missing piece)

Hoist onto `IRadio` the TX/RX surface that exists only on concrete `LoraRadio` today: `setParams`, `sendPacket`, `startReceive`, `readReceivedData`, `checkAndClearRxFlag`, `checkAndClearTxFlag`, `getRadioStateError`. Consequences:

- `LoraRadio` (Arduino/RadioLib) adds `override` — implementation untouched.
- `Device.radio` becomes `IRadio*`, obtained via `RadioMeshPlatform::loraRadio()`; `Device.h` drops its `hardware/` includes.
- `PacketRouter` gains `setRadio(IRadio*)` — the exact pattern of its existing `setCrypto`/`setMicService` setters — and `PacketRouter.cpp` drops `hardware/inc/radio/LoraRadio.h`. Core no longer includes hardware.
- Zephyr: `ports/zephyr/hal/{inc,src}/radio/ZephyrLoraRadio.{h,cpp}` implements `IRadio` over the M1-proven Zephyr LoRa API (DT owns pins — `LoraRadioParams.pinConfig` documented as ignored; `CR 4/7` + `preamble 8` fixed to the D5 wire contract, absorbing M1's `lora_phy.h`).

### Seam 3 — Storage: `IByteStorage` is already the contract

No interface change. `EEPROMStorage` (layout, defrag, `EEPROM` API) is Arduino-port internals and stays untouched. Zephyr: `ports/zephyr/hal/{inc,src}/storage/ZephyrNVSStorage.{h,cpp}` implements `IByteStorage` natively over NVS (string key → NVS id mapping; `writeAndCommit`/`commit` map to NVS semantics; `storage_partition` from DT). `Device`'s member becomes `IByteStorage*`; storage sizing/config moves into the platform factory (`ByteStorageParams` is an impl detail of each port). `DeviceStorage.h` swaps its `EEPROMStorage.h` include for `IByteStorage.h` (it already holds `IByteStorage*`).

## What stays the same (guaranteed)

Public app API (`DeviceBuilder` chain, `IDevice`, presets, callbacks) · wire protocol/packet format/crypto scheme/inclusion state machine · the interface set · protocol-layer singletons · Arduino examples · known-deferred security items (chipID-seeded key, zero IV, `random` nonces — replicated behavior-equivalent, audit later).

## Crypto library

operatorfoundation/Crypto 0.4.0 — used modules (`AES*`, `CTR`, `Curve25519`, `SHA256`, cipher bases) verified free of Arduino references; co-compiled on Zephyr, byte-identical wire crypto. Excluded: `RNG.cpp` (unused), `AESEsp32.cpp` (ESP-IDF path). **Open decision:** west-manifest pin vs vendored subset with provenance.

## File-touch inventory

| Area | Files | Nature |
|---|---|---|
| New contract | `src/common/inc/platform/RadioMeshPlatform.h` | declarations only |
| Arduino port | `src/hardware/src/platform/ArduinoPlatform.cpp` (new) | Arduino code moved here verbatim |
| Core conversion | `RoutingTable.cpp`, `InclusionController.{h,cpp}`, `KeyManager.cpp`, `DeviceBuilder.cpp`, `Utils.{h,cpp}`, `EncryptionService.cpp`, `Logger.h` | call sites → `RadioMeshPlatform::`; delete stray Arduino includes |
| Interface completion | `framework/interfaces/IRadio.h` (+7 pure virtuals), `LoraRadio.h` (`override`), `Device.{h,cpp}` (`IRadio*`/`IByteStorage*`, factories), `PacketRouter.{h,cpp}` (`setRadio`), `DeviceStorage.h` (include swap) | completes existing abstractions |
| Zephyr port | `ports/zephyr/hal/src/platform/ZephyrPlatform.cpp`, `hal/{inc,src}/radio/ZephyrLoraRadio.*`, `hal/{inc,src}/storage/ZephyrNVSStorage.*` + module CMake, examples, on-target test runner | all Zephyr-specific code |

**Never touched:** protocol logic (`Packet`, `RoutingTable.h` logic, dedup, `MicService`, `AesCrypto`, `AesCmac`, inclusion state machine), Arduino `LoraRadio.cpp` / `EEPROMStorage.{h,cpp}` internals, examples' application code.

## Verification (unchanged from the roadmap)

Every commit: `pio test -e native` 4/4 + seeed & heltec builds SUCCESS + zero warnings + core-purity grep. Milestone gates: **V0** full library builds for Zephyr/XIAO → **V1** every applicable unit suite passes ON the XIAO (concrete-class suites `test_LoraRadio`/`test_EEPROMStorage` run against the platform's impl via the factories) → **V2** mini cluster (Gates A′/A/B per the tracer-bullet plan).
