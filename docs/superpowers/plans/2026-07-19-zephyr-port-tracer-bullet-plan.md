# RadioMesh Zephyr Port — Tracer-Bullet Plan

**Date:** 2026-07-19
**Supersedes:** the M2–M8 horizontal decomposition in `2026-07-18-zephyr-port-milestone0-design.md` (its §1 project-level decisions stand: dual-hat monorepo, co-compilation, D5 wire-compat, RM_ARDUINO_BUILD guards).

## Approach

One tracer bullet: a thin thread through the **entire** RadioMesh system at once — `DeviceBuilder` → `Device` → inclusion (ECDH) → encryption/MIC → `PacketRouter`/`RoutingTable`/dedup → radio → OTA — validated by a **working mini cluster**. No capability sequencing: a mesh library whose first milestone can't mesh has traced nothing. After the bullet works, milestones *thicken* it (hardening, tests, peripherals) — each verified on the running cluster.

M0–M1 were bootstrap: de-Arduinized packet core + native test env (M0); proof Zephyr drives the SX1262 + build/flash/monitor tooling (M1). The M1 `tx`/`rx` examples bypass the library and are retired by the bullet.

## Why one milestone is feasible: the measured coupling surface

Full read of the library compile-closure (~5.5k lines: Device, DeviceBuilder, InclusionController, KeyManager, DeviceStorage, EncryptionService, MicService, AesCrypto, AesCmac, PacketRouter, RoutingTable, PacketTracker, LoraRadio, EEPROMStorage, interfaces, Definitions). The **entire** remaining Arduino surface is:

| File | Coupling | Treatment |
|---|---|---|
| `common/utils/Utils.h/.cpp` | `random()`, `millis()` in RNG fallback | extend M0 guards (rng/time shims) |
| `core/.../crypto/EncryptionService.cpp` | `#include <Arduino.h>` — **uses no Arduino symbol** | drop/guard the include |
| `core/.../routing/RoutingTable.cpp` | `<Arduino.h>` + 4× `millis()` (route aging) | time shim |
| `framework/.../InclusionController.h/.cpp` | 3× `millis()` (timeouts), `random(256)` (nonce) | time + rng shims |
| `framework/.../KeyManager.cpp` | `<Arduino.h>`, `random(256)`, `ESP.getEfuseMac()` (already `#ifdef ESP32`) | rng shim + Zephyr `hwinfo` device-id branch |
| `framework/.../DeviceBuilder.cpp` | `Serial` + `millis()` wait in `start()` | guarded console-init shim |
| `hardware/.../radio/LoraRadio.h/.cpp` | RadioLib backend (whole impl) + `<RadioLib.h>` in header | Zephyr radio implementation (seam decision below) |
| `hardware/.../storage/eeprom/EEPROMStorage.cpp` | Arduino `EEPROM` API (**header is clean** — includes only `IByteStorage.h`) | Zephyr backend `.cpp` for the same class (NVS) |

Everything else — `Device.cpp`, `PacketRouter.cpp` (minus its radio call), `MicService`, `AesCrypto`, `AesCmac`, `PacketTracker`, `Packet`, `DeviceStorage` — co-compiles as-is.

**Crypto library:** of the `operatorfoundation/Crypto` 0.4.0 modules RadioMesh uses (`AES*`, `CTR`, `Crypto`, `Curve25519`, `SHA256`, `AESCommon`, cipher base classes), **zero contain Arduino references** (verified by grep; only the unused `RNG.cpp` and the hardware-path `AESEsp32.cpp` do). The same sources co-compile on Zephyr → ECDH/AES-CTR/CMAC are **byte-identical by construction**. No PSA rewrite in the bullet; the rweather-vs-PSA question is deferred to hardening.

Protocol facts that force the whole-system bullet (no shortcut exists):
- `Device::initialize()` **unconditionally** requires storage + `InclusionController` (`Device.cpp:519-539`); `run()` calls `checkProtocolTimeouts()` every loop.
- `sendData()` is gated by inclusion: a STANDARD device must be INCLUDED to send app topics (`InclusionController::canSendMessage`).
- A relay must also be INCLUDED: forwarding happens only after MIC verification, which needs the network key.
- App payloads are AES-encrypted + CMAC'd once included — crypto is on the main path, not an option.

## M2 — the tracer bullet: RadioMesh mini cluster on Zephyr

Mirror of the existing `examples/DeviceInclusion` pair (MiniHub + StandardDevice) — the canonical RadioMesh application — with Zephyr nodes.

**Gate A — mixed cluster (primary):**
Arduino **MiniHub** (Heltec V3, today's unmodified example) + **Zephyr standard relay** + **Zephyr standard leaf**:
1. Hub opens inclusion → both Zephyr nodes complete the full ECDH inclusion (INCLUDE_OPEN → REQUEST → RESPONSE → CONFIRM → SUCCESS) and persist INCLUDED.
2. Leaf sends telemetry (topic 0x10, AES + CMAC) that transits the relay to the hub (multi-hop forced by TX-power/antenna attenuation).
3. Dedup: hub sees each packet once despite relay re-broadcast.
4. Power-cycle the leaf → it comes back INCLUDED from NVS and resumes sending without re-inclusion.

Gate A also proves **live Arduino↔Zephyr wire-compat** (D5) — the strongest possible validation of co-compilation.

**Gate B — role swap:** Zephyr **hub** (inclusion mode via console/timed trigger instead of the portal UI) includes an Arduino StandardDevice and a Zephyr leaf — exercises the hub-side code paths (network-key generation, INCLUDE_RESPONSE/SUCCESS) on Zephyr. Same firmware set, roles permuted.

One gate run validates: DeviceBuilder, Device, InclusionController + KeyManager + DeviceStorage, EncryptionService (Direct-ECC + AES-CTR), MicService (AES-CMAC), PacketRouter, RoutingTable, PacketTracker, radio backend, storage backend. The whole library, thin.

### Work breakdown (all inside M2)

- **W1 — platform shims** (`common/`, M0 guard pattern): monotonic time (`millis`-equivalent → `k_uptime_get_32`), RNG byte/int (→ existing M0-guarded RNG path), console init, device-unique-id (→ `hwinfo`). Apply to the 6 shim-files in the table. Arduino builds stay byte-identical.
- **W2 — radio backend**: Zephyr implementation of the `LoraRadio` role over the M1-proven Zephyr LoRa API (DT overlay pins; `LoraRadioParams` supplies band/BW/SF/power/sync — pins come from devicetree). Async RX (`lora_recv_async`) feeds the same rxDone/txDone flag model `Device::run()` polls.
- **W3 — storage backend**: Zephyr `.cpp` for `EEPROMStorage` over NVS (header unchanged; `writeAndCommit` maps naturally). Inclusion state/keys survive reboot (Gate A step 4).
- **W4 — crypto co-compilation**: add pinned Crypto 0.4.0 sources to the Zephyr build (manifest or vendored subset — decision at execution), excluding `RNG.cpp`/`AESEsp32.cpp` (software AES path; wire format identical).
- **W5 — cluster examples**: `ports/zephyr/examples/hub` + `ports/zephyr/examples/standard` (leaf/relay = same app, relay flag) on the real API, mirroring the `.ino` pair; retire `examples/tx|rx`.
- **W6 — gates**: regression every step (`pio test -e native` 4/4, `pio run -e seeed_xiao_esp32s3` + heltec SUCCESS, zero warnings); then Gate A, Gate B on hardware.

### Decisions at execution start (recommendations attached)

1. **Radio seam** — (a) hoist TX/RX (`sendPacket`, `startReceive`, `readReceivedData`, flag/state accessors) onto `IRadio`, `Device.radio` becomes `IRadio*`, inject `IRadio*` into `PacketRouter` (kills `core → hardware` include + the `LoraRadio::getInstance()` call at `PacketRouter.cpp:125`), platform factory supplies the instance; or (b) platform-split `.cpp` of concrete `LoraRadio` with a guarded header. **Recommend (a)** — comparable size, and the header is RadioLib-infested either way; (b) leaves the known leak in place.
2. **Crypto sources on Zephyr** — west-manifest pin vs vendored subset in-repo. Decide by what keeps the pinned version verifiably identical to PlatformIO's.
3. Hardware for the 3rd node — Heltec V3 (Arduino) preferred for Gate A wire-compat; a 3rd XIAO also works.

### Explicit non-goals of the bullet (replicate as-is, do not "fix")

Deterministic chipID-seeded private key, static all-zero IV, `random()`-based nonces — ported behavior-identical; they belong to the deferred security audit. Display / WiFi / portal stay behind the existing `RM_NO_DISPLAY` / `RM_NO_WIFI` guards (Zephyr builds define both).

## After the bullet — thickening (each verified on the live cluster)

- **H1 — seams & tests**: finish whichever seam shortcuts the bullet took; native-test expansion with injectable time (RoutingTable aging, PacketTracker, CMAC/ECDH vectors host-side).
- **H2 — product polish**: Zephyr sample apps + docs to shipping quality (`make`-driven cluster bring-up, README walkthrough) — RadioMesh usable as a Zephyr library by outsiders.
- **H3 — peripheral parity (demand-driven)**: display; WiFi/portal on Zephyr ESP32-S3. Optional PSA crypto backend behind the same service interfaces.

## Verification model

Every commit: both PlatformIO gates + Zephyr example builds, zero warnings. Milestone: the cluster gates above. Bite-sized implementation plan (writing-plans) is authored when M2 execution starts, against this document.

**Resume from:** this file + `project_zephyr_port.md` (memory).
