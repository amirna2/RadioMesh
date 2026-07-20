# M2 Handoff — Task 1 DONE, start Task 2

**Date:** 2026-07-19 · **Branch:** `feat/zephyr-m2-tracer-bullet` (NOT pushed) · **Milestone:** M2 tracer bullet (mini cluster on Zephyr/XIAO).

## Governing documents (read in this order)

1. `docs/superpowers/specs/2026-07-19-zephyr-platform-interface-design.md` — THE design authority. Doctrine: complete the clean architecture; core 100% platform-free; platform code only in porting layers; NO Arduino emulation; NO `#ifdef` in logic files; PascalCase C++ files/classes everywhere incl. `ports/` (`ZephyrNVSStorage` like `EEPROMStorage`); `inc/<domain>/` + `src/<domain>/` layout.
2. `docs/superpowers/plans/2026-07-19-zephyr-port-m2-implementation.md` — task list T1–T8 (rev. 2).
3. `docs/superpowers/plans/2026-07-19-zephyr-port-tracer-bullet-plan.md` — roadmap + V0/V1/V2 gates.

## Non-negotiable process rules (user-enforced, violations caused blowups)

- **Present each task's scope and WAIT for explicit approval before writing any code.** Plan-writing approval ≠ execution approval.
- Gates on EVERY `src/` commit: `pio test -e native` (4/4) + `pio run -e seeed_xiao_esp32s3` + `pio run -e heltec_wifi_lora_32_V3` SUCCESS + **zero warnings in RadioMesh code** (vendor warnings: ESP32-core `esp32-hal-uart.c`, U8g2 — tolerated) + purity grep (below).
- Conventional Commits; body ends `Assisted-by: Claude Code (<model>)`. Never commit CLAUDE.md/AI_DEVELOPER.md/CONTEXT.md. Never merge PRs. No line-wrapping in PR bodies.
- Hardware: XIAO ESP32-S3 + Wio-SX1262 ONLY (2 boards on hand; 3rd needed for Gate A).
- `pio` is at `~/.platformio/penv/bin` (not on default PATH).

Purity grep (must return only `Options.h:5` — excluded below):

    grep -rnE '(^|[^:A-Za-z_"])((millis|random|analogRead|randomSeed|delayMicroseconds)\s*\()|Arduino\.h|Serial\.|ESP\.' src/common src/core src/framework --include='*.h' --include='*.cpp' | grep -v 'inc/platform/' | grep -v 'RadioMeshPlatform::' | grep -v 'Options.h:5'

## State: Task 1 COMPLETE (`d5a1ae2`), all gates green

The `RadioMeshPlatform` contract exists and the core is de-Arduinofied:

- `src/common/inc/platform/RadioMeshPlatform.h` — namespace contract: `millis, delay, random(max), randomBytes, chipId, consoleInit, logWrite(data,len)`, and composition factories `IRadio* loraRadio()`, `IByteStorage* byteStorage()` (forward-declared classes).
- `src/hardware/src/platform/ArduinoPlatform.cpp` — Arduino impls; `simpleRNG` (analog entropy), `Serial` console bring-up, efuse chip-id moved here verbatim; RNG seeds once per boot (was per-ID; documented in commit). Factories return `LoraRadio::getInstance()` / `EEPROMStorage::getInstance()`.
- `src/common/inc/platform/HostPlatform.h` — inline host reference impl, structurally selected by the contract header when neither `ARDUINO` nor `__ZEPHYR__` (keeps native tests linking; factories return `nullptr`).
- `Logger.h` sinks through `RadioMeshPlatform::logWrite` (no ARDUINO/`__ZEPHYR__` branches left in core). Converted: `RoutingTable.cpp`, `InclusionController.{h,cpp}`, `KeyManager.cpp` (dead `<RNG.h>` dropped), `DeviceBuilder.cpp`, `Utils.{h,cpp}` (single portable `getRandomBytesArray`; `simpleRNG` removed from public namespace — no external callers), `EncryptionService.cpp`, `AsyncDevicePortal.cpp`. Also fixed 2 pre-existing warnings (`-Wreorder` Device ctor, `-Wmaybe-uninitialized` loadedState).

Branch history: `b092f73`/`fa87715`/`e8f3fb6` (roadmap evolution), `a68806f` (impl plan rev.1, superseded), `b3644da`+`68e5c50` (spec + rev.2 alignment), `d5a1ae2` (T1).

## NEXT: Task 2 — presented to user, **get explicit approval at session start, then code**

`refactor(hal): complete IRadio contract; Device/PacketRouter consume interfaces`

| File | Exact change |
|---|---|
| `src/framework/interfaces/IRadio.h` | +7 pure virtuals (Doxygen, house style): `int setParams(LoraRadioParams)`, `int sendPacket(std::vector<byte>&)`, `int startReceive()`, `int readReceivedData(std::vector<byte>*)`, `bool checkAndClearRxFlag()`, `bool checkAndClearTxFlag()`, `int getRadioStateError()`. (`setup/getSNR/getRSSI/standBy/sleep` already there; header already includes `RadioConfigs.h` + `Packet.h`.) |
| `src/hardware/inc/radio/LoraRadio.h` | add `override` to those 7 (now-virtual) methods; LoraRadio-specific extras (`getParams`, `isRadioSetup`, `startTransmitPacket`, `onInterrupt`) stay non-interface. Implementation `.cpp` untouched. |
| `src/framework/device/inc/Device.h` | `LoraRadio* radio` → `IRadio* radio`; `EEPROMStorage* eepromStorage` → `IByteStorage* byteStorage`; drop `#include <hardware/inc/radio/LoraRadio.h>` and `<hardware/inc/storage/eeprom/EEPROMStorage.h>` (keep the `RM_NO_*`-guarded display/wifi ones); include `IRadio.h`/`IByteStorage.h` + `RadioMeshPlatform.h`. |
| `src/framework/device/src/Device.cpp` | `initializeRadio`: `radio = RadioMeshPlatform::loraRadio()` + after `setParams` OK → `router->setRadio(radio)`. `initialize`: `byteStorage = RadioMeshPlatform::byteStorage()` then `begin()` — **the `setParams(ByteStorageParams(EEPROM_STORAGE_MAX_SIZE))` call moves into the Arduino factory** (`ArduinoPlatform.cpp::byteStorage()`), since sizing is port business; `getByteStorage()` returns the renamed member. |
| `src/core/protocol/inc/routing/PacketRouter.h` | `#include <framework/interfaces/IRadio.h>`; member `IRadio* radio = nullptr;` + `void setRadio(IRadio* r)` (exact style of existing `setCrypto`). |
| `src/core/protocol/src/routing/PacketRouter.cpp` | drop `#include <hardware/inc/radio/LoraRadio.h>`; `sendPacket`: guard `radio == nullptr` (logerr + `RM_E_DEVICE_INITIALIZATION_FAILED`, mirroring the micService guard) then `radio->sendPacket(buffer)`. |
| `src/framework/device/inc/DeviceStorage.h` | swap `#include <hardware/inc/storage/eeprom/EEPROMStorage.h>` → `<framework/interfaces/IByteStorage.h>`. |

Gates: full set above; after T2 the purity grep concept extends — additionally verify core includes no `hardware/` headers: `grep -rn 'hardware/inc' src/common src/core src/framework | grep -v RM_NO_` should show only the guarded display/wifi includes in `Device.h`.

**Watch-outs:** `test_Device`/`test_DeviceBuilder`/`test_LoraRadio` (Arduino-target suites) may name `LoraRadio`/`EEPROMStorage` concretely — they still compile on Arduino (classes unchanged); do NOT adapt tests in T2 (that's T6). `Device.h` still includes `InclusionController.h` (fine). `EEPROM_STORAGE_MAX_SIZE` lives in `EEPROMStorage.h` — after the move, `Device.cpp` must not reference it.

## Then: T3–T8 (per impl plan rev. 2)

T3 Zephyr HAL (`ports/zephyr/hal/`: `ZephyrPlatform.cpp`, `ZephyrLoraRadio`, `ZephyrNVSStorage`) → T4 crypto pin (west vs vendor — user decision) → T5 **V0** full-library Zephyr build + `examples/standard` skeleton → T6 **V1** on-target Unity runner, 10 suites on XIAO → T7 **V2** hub/standard cluster examples, Gates A′/A/B (retire `tx`/`rx` + `lora_phy.h`) → T8 handoff + PR (user merges).

## Environment

- West workspace topdir `/Users/nathoo/dev` (zephyr 4.4.0 + carried gpio BIT64 patch — re-run `make setup` after any `west update`); Makefile in `ports/zephyr/` auto-detects the bootstrap venv; M1 tooling: `make build/flash/monitor/run/boards`, `tools/rmon.py` (stdlib-only).
- Memory: `project_zephyr_port.md` + feedback memories (review-gate, porting doctrine, naming, zero-warnings, tracer-bullet) — all current as of this handoff.
