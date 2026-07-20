# RadioMesh Zephyr Port — Vertical-Slice Porting Plan

**Date:** 2026-07-19
**Supersedes:** the M2–M8 (horizontal, subsystem-by-subsystem) decomposition in `2026-07-18-zephyr-port-milestone0-design.md`. M0–M1 and all project-level decisions in that spec stand.

## Approach — tracer bullets, not horizontal modules

Every remaining slice is a **thin end-to-end path through the real RadioMesh library API** (`DeviceBuilder` → `Device` → `sendData`/callback → `IRadio` → LoRa → OTA → RX → callback). Each slice ships a capability that is *usable* and *end-to-end testable* on hardware the moment it lands, then the next slice thickens it. We do **not** port subsystems in isolation.

**Why the horizontal plan was wrong:** `RadioMeshDevice` is concretely wired to every subsystem — `LoraRadio* radio`, `EEPROMStorage* eepromStorage`, `EncryptionService`/`MicService` as *value members*, `PacketRouter::getInstance()`, an owned `InclusionController` (`Device.h:213-240`). Value members and `getInstance()` force all those types to fully compile before the library can build. A bottom-up plan therefore makes the library the *last* thing that can exist (old M6). That indefinitely postpones the one thing that matters: RadioMesh being *usable* as a Zephyr library.

**Invariant for every slice:** both platforms stay green. `src/` is co-compiled, so a Zephyr change is an Arduino change. Gates below run every slice; wire-compat with Arduino is verified wherever a slice touches the wire. Zero warnings, always.

## Done — M0–M1 (bootstrap, merged to `main`)

- **M0** — de-Arduinized the core packet slice; stood up the `native` host-test env. Both gates green.
- **M1** — proved Zephyr drives the SX1262 over LoRa; two nodes exchange a `RadioMeshPacket`; `make`-based build/flash/monitor loop. The M1 `tx`/`rx` examples deliberately bypass the library (poke the LoRa driver + packet struct directly) — they are scaffolding, retired in Slice 1.

## The keystone finding (drives Slice 1)

`IRadio` (`interfaces/IRadio.h`) exposes only `setup/getSNR/getRSSI/standBy/sleep` — **no TX, no RX**. The send/receive surface lives only on concrete `LoraRadio` (`sendPacket`, `startReceive`, `readReceivedData`, `checkAndClearRx/TxFlag`, `getRadioStateError`, `setParams`). Consequences:

- `Device` holds `LoraRadio*` (not `IRadio*`) to reach those methods (`Device.cpp:331,362,445-474`).
- `PacketRouter::sendPacket()` calls `LoraRadio::getInstance()->sendPacket()` directly (`PacketRouter.cpp:125`), and `PacketRouter.cpp` `#include`s `hardware/.../LoraRadio.h`.

Hoisting the TX/RX contract onto `IRadio` and injecting an `IRadio*` into `PacketRouter` is the single change that lets the *same* `Device`/`PacketRouter` run on both Arduino and Zephyr. It is the enabler for the entire vertical approach.

## Verification model (applies to every slice)

1. **Native unit tests** — `pio test -e native` for portable logic (packet, routing decisions, dedup, crypto vectors, state machines).
2. **Hardware tracer gate** — the slice's OTA scenario, run through the *real* library API.
3. **Regression gates** — `pio test -e native` (currently 4/4) + `pio run -e seeed_xiao_esp32s3` SUCCESS + Arduino heltec build unaffected. Zero warnings.
4. **Wire-compat** — Zephyr ↔ Arduino interop for any slice that changes bytes on the air.

Each slice, when started, gets its own bite-sized implementation plan (writing-plans) + a HANDOFF. **This document is the roadmap, not the task list.**

---

## Slices

### Slice 1 (M2) — Usable leaf: the real library API on Zephyr  **[KEYSTONE]**

**Tracer bullet:** `DeviceBuilder().start().withLoraRadio(p).withRxPacketCallback(cb).build(name, id, STANDARD)`, then `device->sendData(topic, data, dest)` / `device->run()`. Two Zephyr nodes exchange an **application packet through `sendData` + the RX callback** — a real RadioMesh leaf node.

**Scope:**
- **Hoist the TX/RX contract onto `IRadio`** (`sendPacket`, `startReceive`, `readReceivedData`, `checkAndClearRx/TxFlag`, `getRadioStateError`, `setParams/getParams`, `isRadioSetup`). `Device.radio` becomes `IRadio*`; inject `IRadio*` into `PacketRouter` and drop its `hardware/.../LoraRadio.h` include.
- **Make non-radio subsystems optional** so a radio-only `Device` builds on Zephyr (absent → feature off): shallow-de-Arduinize `EncryptionService`/`MicService` so they *compile* unused (security = NONE at this slice); guard/forward-declare `EEPROMStorage` (no persistence yet); keep `InclusionController` inert.
- **Zephyr `IRadio` implementation** — a `LoraRadio` for Zephyr wrapping the M1 LoRa device + PHY preset (D5 interop).
- **New examples on the real API** (leaf tx / rx or a single leaf), retiring the M1 driver-poking examples.

**Recommended sequencing (safest blast radius):** (a) hoist TX/RX to `IRadio` + inject into `PacketRouter` **on the existing platforms first** — a pure refactor, verified by the current Arduino/native test suites with zero behavior change; then (b) add the Zephyr `IRadio` impl + optional-subsystem guards + Zephyr example.

**Verify:** Arduino heltec + `seeed` builds unchanged; `native` still 4/4; two Zephyr nodes exchange an app packet via `sendData`/callback OTA; zero warnings.

**Risk:** largest blast radius of the whole port — it edits the shipping `Device`/`PacketRouter`. Contained by doing (a) under existing green tests before touching Zephyr, plus co-compilation + wire-compat.

### Slice 2 (M3) — Routing / relay

**Tracer bullet:** three nodes, A → B → C multi-hop forward via `withRelayEnabled(true)`, through the real API.

**Scope:** bring `PacketRouter`'s forwarding path live on Zephyr; de-Arduinize `RoutingTable` — its only Arduino dependency is `millis()` (route aging/timeout, 4 calls). Add a platform time primitive (Arduino `millis()` / Zephyr `k_uptime_get_32()` / native `std::chrono`), mirroring M0's guarded-branch pattern in `common/`. Wire the existing `PacketTracker` dedup suite into `native`.

**Verify:** native tests for route learning/aging + dedup; 3-node OTA relay via the API; both gates green.

### Slice 3 (M4) — Crypto / MIC

**Tracer bullet:** `withSecureMessaging(sec)`; an encrypted Zephyr node interoperates with an Arduino node (decrypt + MIC-verify a real message both ways).

**Scope:** AES-CTR + AES-CMAC building and running on Zephyr, byte-identical to Arduino. Backend decision (rweather vs. PSA) is made at the *start* of this slice — flagged, not resolved here.

**Verify:** native crypto/MIC vectors; Zephyr ↔ Arduino encrypted interop OTA; both gates.

### Slice 4 (M5) — Storage

**Tracer bullet:** keys/config written on a Zephyr node survive a reboot.

**Scope:** a Zephyr `IByteStorage` implementation (NVS / settings / flash) behind the interface `Device` already consumes; replaces the Slice-1 no-persistence stub.

**Verify:** write → reboot → read; both gates.

### Slice 5 (M6) — Inclusion

**Tracer bullet:** a Zephyr leaf is included by a hub end-to-end (the staged ECDH handshake).

**Scope:** `InclusionController` + `KeyManager` live on Zephyr; interop with an Arduino hub.

**Verify:** full inclusion OTA; native tests for the state machine; both gates.

### Slice 6 (M7) — Display · Slice 7 (M8) — WiFi + portal

ESP32-centric, outside the protocol core. Each is a thin capability add behind `IDisplay` / `IWifi*` / `IDevicePortal`, tested on hardware. Lowest priority; sequenced after the mesh is complete.

---

## Open decisions (flagged, resolved when the slice starts)

- **Slice 1 — optional-subsystem mechanism:** interface-injected-and-nullable vs. shallow-compile-then-harden. Recommendation: nullable pointers where the members already are pointers (`EEPROMStorage`, `InclusionController`); shallow-compile crypto (value members, unused at Slice 1).
- **Slice 3 — crypto backend:** rweather/Crypto vs. PSA Crypto (Zephyr-native). Decide at Slice 3 start.
- **Housekeeping:** remove the unused `loramac-node` from `west.yml` (needs a `west update` + rebuild to confirm).

## Resume from

This file + `project_zephyr_port.md` (memory). Next action: on approval, write the Slice 1 implementation plan (writing-plans) and open the Slice 1 branch.
