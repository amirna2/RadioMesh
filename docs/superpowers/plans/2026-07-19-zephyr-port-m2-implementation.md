# M2 Tracer Bullet — Implementation Plan (rev. 2: architecture-completion)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax. **Each task is presented to the user for approval before its code is written.**

**Rev. 2 supersedes rev. 1 (`a68806f`)**, whose mechanism (Arduino-API emulation shims + ifdefs) violated the architecture. This revision implements `docs/superpowers/specs/2026-07-19-zephyr-platform-interface-design.md`: complete de-Arduinofication of the core via the `RadioMeshPlatform` contract, `IRadio` completion, native Zephyr implementations in `ports/zephyr/hal/`.

**Goal:** V0 full library builds for Zephyr/XIAO → V1 every applicable unit test passes ON the board → V2 mini cluster (inclusion + encrypted multi-hop + dedup + NVS persistence).

## Global constraints

- Branch `feat/zephyr-m2-tracer-bullet`; Conventional Commits + `Assisted-by: Claude Code (<model>)`; never commit CLAUDE.md/AI_DEVELOPER.md/CONTEXT.md; never merge PRs.
- **Gates on every `src/` commit:** `pio test -e native` 4/4, `pio run -e seeed_xiao_esp32s3` + `-e heltec_wifi_lora_32_V3` SUCCESS, **zero warnings**, and core-purity grep (no `Arduino.h|millis(|random(|analogRead|Serial` outside the contract header and ports).
- Naming: PascalCase C++ files/classes everywhere incl. `ports/`; `inc/<domain>/` + `src/<domain>/` layout; ecosystem artifacts keep mandated names.
- Arduino behavior equivalent, code moved verbatim where possible; wire behavior identical.
- Hardware: XIAO ESP32-S3 + Wio-SX1262 only.

---

### Task 1: Platform services contract + Arduino port + core conversion (Seam 1)

**Files:**
- Create: `src/common/inc/platform/RadioMeshPlatform.h` — the namespace contract from the spec (`millis, delay, random, randomBytes, chipId, consoleInit, loraRadio, byteStorage` — the two factories forward-declare `IRadio`/`IByteStorage`).
- Create: `src/hardware/src/platform/ArduinoPlatform.cpp` — Arduino implementations; `simpleRNG`+`analogRead` entropy and the `Serial` boot block and `ESP.getEfuseMac()` move here **verbatim**; factories return `LoraRadio::getInstance()` / `EEPROMStorage::getInstance()`.
- Modify (call-site conversion, no logic changes): `RoutingTable.cpp` (4× `millis` → `RadioMeshPlatform::millis`, delete `<Arduino.h>`), `InclusionController.cpp` (3× `millis`), `InclusionController.h` (`generateNonce` → `RadioMeshPlatform::randomBytes`), `KeyManager.cpp` (`random(256)` loop → `randomBytes`; chip-id → `chipId()`; delete `<Arduino.h>`, dead `<RNG.h>`), `DeviceBuilder.cpp` (`Serial` block → `RadioMeshPlatform::consoleInit()`), `Utils.cpp/.h` (uuid + `getRandomBytesArray` collapse to one portable path over the contract; `simpleRNG` leaves core), `EncryptionService.cpp` (delete unused `<Arduino.h>`), `Logger.h` (route output through the contract, removing the `__ZEPHYR__` guard).

- [ ] 1.1 Write contract header + `ArduinoPlatform.cpp`.
- [ ] 1.2 Convert core call sites file-by-file.
- [ ] 1.3 Gates: native 4/4, seeed + heltec SUCCESS, zero warnings, purity grep clean.
- [ ] 1.4 Commit: `refactor(platform): introduce RadioMeshPlatform contract; de-Arduinofy core`

### Task 2: Complete IRadio + interface-typed Device (Seams 2 & 3, core side)

**Files:**
- Modify: `framework/interfaces/IRadio.h` — add pure virtuals: `setParams(LoraRadioParams)`, `sendPacket(std::vector<byte>&)`, `startReceive()`, `readReceivedData(std::vector<byte>*)`, `checkAndClearRxFlag()`, `checkAndClearTxFlag()`, `getRadioStateError()` (Doxygen per house style).
- Modify: `hardware/inc/radio/LoraRadio.h` — mark the now-virtual methods `override` (impl untouched).
- Modify: `framework/device/inc/Device.h` — `LoraRadio* radio` → `IRadio* radio`; `EEPROMStorage* eepromStorage` → `IByteStorage* byteStorage`; drop `hardware/` includes (keep the `RM_NO_*`-guarded ones).
- Modify: `framework/device/src/Device.cpp` — `initializeRadio`: `RadioMeshPlatform::loraRadio()` + `router->setRadio(radio)`; `initialize`: `RadioMeshPlatform::byteStorage()` + `begin()` (params/sizing now the port's concern); `getByteStorage()` returns the member.
- Modify: `core/protocol/inc/routing/PacketRouter.h` + `.cpp` — `setRadio(IRadio*)` (mirrors `setCrypto`); `sendPacket` uses the injected pointer; drop the `LoraRadio.h` include.
- Modify: `framework/device/inc/DeviceStorage.h` — include `IByteStorage.h` instead of `EEPROMStorage.h`.

- [ ] 2.1 Apply in the order above; keep each diff minimal.
- [ ] 2.2 Gates (full set) — this is the highest-blast-radius task of the milestone.
- [ ] 2.3 Commit: `refactor(hal): complete IRadio contract; Device/PacketRouter consume interfaces`

### Task 3: Zephyr port HAL (`ports/zephyr/hal/`)

**Files (new):**
- `ports/zephyr/hal/src/platform/ZephyrPlatform.cpp` — `k_uptime_get_32`, `k_msleep`, `sys_rand32_get`/`sys_rand_get`, `hwinfo_get_device_id`, no-op `consoleInit`, factories returning the two impls below.
- `ports/zephyr/hal/inc/radio/ZephyrLoraRadio.h` + `src/radio/ZephyrLoraRadio.cpp` — `IRadio` over the Zephyr LoRa API: DT `lora0` device; params→`lora_modem_config` (band MHz→Hz, bw switch, SF, power, `public_network=!privateNetwork`; `CR_4_7` + preamble 8 fixed per D5, comment; `pinConfig` ignored — DT owns pins); async RX callback buffers packet + RSSI/SNR and sets the rx flag (driver auto re-arms RX; PHY-CRC-bad frames dropped in-driver — protocol CRC32/MIC still validate); `sendPacket` cancels async RX (driver-enforced modem exclusivity) then non-blocking `lora_send_async` + `k_poll_signal` — the signal IS the tx flag (`checkAndClearTxFlag` = `k_poll_signal_check`; the driver raises it on TxDone AND on TX timeout with `-ETIMEDOUT` → `RM_E_RADIO_TX_TIMEOUT`, and a 2×`lora_airtime()` deadline backstops a lost completion); error mapping to `RM_E_*`. Mirrors Arduino's non-blocking `startTransmit` semantics exactly (spec: "Zephyr TX model").
- `ports/zephyr/hal/inc/storage/ZephyrNVSStorage.h` + `src/storage/ZephyrNVSStorage.cpp` — `IByteStorage` over NVS (`<zephyr/kvss/nvs.h>`, `CONFIG_NVS`): `storage_partition` mount in `begin()` (`PARTITION_DEVICE/OFFSET/SIZE` macros — the `FIXED_PARTITION_*` forms are deprecated in 4.4); key→id mapping for the five reserved 2-char keys (+ deterministic hash for others); NVS writes are atomic + immediately persistent → direct-write with `commit` no-op, documented; `read/remove/exists/clear/available/isFull/defragment` mapped honestly (NVS GC makes `defragment` a no-op returning `RM_E_NONE`; `clear` requires re-mount).
- DT: **verified** — `storage_partition` present for `xiao_esp32s3/esp32s3/procpu` via `espressif/partitions_0x0_amp_4M.dtsi` (`partition@3b0000`, 192 KiB); no overlay change.

- [ ] 3.1 Write the three impls (+ headers).
- [ ] 3.2 Compile check rides Task 5 (no Zephyr app builds them yet); PIO gates unaffected (`ports/` invisible).
- [ ] 3.3 Commit: `feat(zephyr): platform, radio, and storage implementations in ports/zephyr/hal`

### Task 4: Crypto sources pinned into the Zephyr build

- [ ] 4.1 Decide (present to user): west-manifest pin of `OperatorFoundation/Crypto` at the 0.4.0-matching commit vs vendored subset. Verify tree identity against `.pio/libdeps/*/Crypto/src`.
- [ ] 4.2 Wire chosen source into the build; compile set: `Crypto, Cipher, BlockCipher, AES256, AESCommon, CTR, Curve25519, BigNumberUtil, SHA256, Hash` (finalize via linker); exclude `RNG.cpp`, `AESEsp32.cpp`.
- [ ] 4.3 Commit: `build(zephyr): co-compile pinned Crypto 0.4.0 sources`

### Task 5: V0 — full library builds for Zephyr/XIAO

**Files:**
- Modify: `zephyr/CMakeLists.txt` — `zephyr_library()` with the 12 core `.cpp` (Utils, EncryptionService, MicService, AesCrypto, AesCmac, PacketRouter, RoutingTable, DeviceBuilder, Device, InclusionController, KeyManager — **not** `ArduinoPlatform.cpp`, not `LoraRadio.cpp`, not `EEPROMStorage.cpp`) + the three `ports/zephyr/hal` `.cpp` + crypto sources; include dirs (`src`, `ports/zephyr/hal/inc`, crypto); `RM_NO_DISPLAY`/`RM_NO_WIFI` definitions.
- Create: `ports/zephyr/examples/standard/` (CMake + `prj.conf` [M1 base + `CONFIG_NVS/FLASH/FLASH_MAP/ENTROPY_GENERATOR/HWINFO`] + `src/main.cpp` building a STANDARD device via the real `DeviceBuilder` chain and running `device->run()`).
- Modify: `ports/zephyr/Makefile` — roles.

- [ ] 5.1 Iterate `make build ROLE=standard` to zero errors/warnings.
- [ ] 5.2 **V0 GATE:** Zephyr build SUCCESS + all PIO gates green. Boot-smoke on board (builder logs, NVS mount, radio configured).
- [ ] 5.3 Commit: `feat(zephyr): full RadioMesh library builds and boots on XIAO (V0)`

### Task 6: V1 — existing unit suites on the board

- Create: `ports/zephyr/tests/runner/` — Zephyr app taking `-DSUITE=<name>`; compiles `test/<SUITE>/*.cpp` + vendored Unity (same version as PIO; provenance note) + adapter `main` (suite entry renamed via `COMPILE_DEFINITIONS main=rm_test_main;setup=rm_test_setup;loop=rm_test_loop`; weak-symbol dispatch; `RM_TEST_DONE rc=` sentinel).
- Modify: `ports/zephyr/Makefile` — `make test SUITE=<name>`: build, flash, capture serial until sentinel, parse Unity summary, non-zero exit on failure.
- Suites on Zephyr/XIAO: `test_Packet, test_PacketTracker, test_Crypto, test_DynamicKeyExchange, test_InclusionProtocol, test_Example, test_Device, test_DeviceBuilder, test_EEPROMStorage*, test_LoraRadio*` (*concrete-class suites exercise the platform impl via `RadioMeshPlatform` factories — adapt instantiation lines only where they name the concrete class; excluded: display/wifi suites, subsystems compiled out).

- [ ] 6.1 Runner + `test_Packet` green on board.
- [ ] 6.2 Remaining suites, one commit per fix-cluster.
- [ ] 6.3 **V1 GATE:** all 10 suites, 0 failures, on the XIAO; log captured.
- [ ] 6.4 Commit: `test(zephyr): full unit suite passes on XIAO hardware (V1)`

### Task 7: V2 — cluster examples and gates

- Extend `examples/standard` to the full `StandardDevice.ino` mirror (telemetry topic `0x10` when INCLUDED; relay variant via app Kconfig → `withRelayEnabled(true)`).
- Create `ports/zephyr/examples/hub/` — `MiniHub.ino` mirror minus portal (timed/console inclusion trigger, comment the substitution).
- Delete `ports/zephyr/examples/{tx,rx}` + `ports/zephyr/common/lora_phy.h` (contract absorbed by `ZephyrLoraRadio`); update Makefile roles + README/DEVELOPING (concise).

- [ ] 7.1 **Gate A′ (2 boards):** Zephyr hub + Zephyr leaf — full inclusion sequence, encrypted+MIC telemetry, leaf power-cycle rejoins from NVS.
- [ ] 7.2 **Gate A (3 boards):** XIAO-Arduino MiniHub + Zephyr relay + Zephyr leaf — inclusion of both, multi-hop telemetry (min TX power/antenna attenuation), dedup at hub. **M2 acceptance.**
- [ ] 7.3 **Gate B:** Zephyr hub includes an Arduino standard node.
- [ ] 7.4 Commits per component; docs commit last.

### Task 8: Handoff + PR

- [ ] 8.1 `docs/superpowers/plans/<date>-zephyr-port-m2-HANDOFF.md` — exact state, gate evidence, honest gaps.
- [ ] 8.2 Push; open PR (template; no line-wrapping; `Assisted-by` only). Stop — user merges.

## Execution protocol

Strict order T1→T8. **Before coding each task: present the task summary and get approval.** Report at every gate (V0/V1/V2). Any deviation discovered mid-task → stop, surface, re-approve.
