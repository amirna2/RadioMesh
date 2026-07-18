# Zephyr Port — Milestone 0 (De-Arduino-ify Core Packet Slice) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the protocol core's packet slice compile with zero Arduino dependency (guarded so the Arduino build is byte-identical), and prove it via a new host `native` test.

**Architecture:** Add a PlatformIO `native` (host, frameworkless) test env that compiles a header-only slice of the core — `RadioMeshPacket` + `CRC32` — with no Arduino. Every shared-file edit is guarded by the existing `RM_ARDUINO_BUILD` / `RM_GENERIC_BUILD` macros so the Arduino preprocessor output does not change. Two gates must stay green: the Arduino firmware still builds, and the slice compiles + round-trips on the host.

**Tech Stack:** PlatformIO, Unity test framework, C++17, host `g++`/clang (PlatformIO `native` platform), target ESP32-S3 (`heltec_wifi_lora_32_V3`, `seeed_xiao_esp32s3`).

## Global Constraints

Every task's requirements implicitly include these (values copied from the spec):

- **C++ standard:** `-std=gnu++17` on every env.
- **Guard macro:** all shared-file edits gated by `RM_ARDUINO_BUILD` (Arduino) / `RM_GENERIC_BUILD` (host) — both defined at the top of `src/common/inc/Options.h`. Arduino-branch preprocessor output must stay byte-identical.
- **Native test rules:** the host test uses `int main()` (NOT Arduino `setup()/loop()`); includes slice headers **directly** (`Packet.h`, `RadioMeshCrc32.h`), NOT the `RadioMesh.h` umbrella; defines **no** `RM_LOG_*` (log macros stay no-ops); compiles **no** `src/*.cpp` (the round-trip slice is header-only).
- **Two success gates:** (1) `pio run -e heltec_wifi_lora_32_V3` and `-e seeed_xiao_esp32s3` still build; (2) `pio test -e native` compiles the slice without Arduino and passes.
- **Out of scope (do NOT touch):** Zephyr/`ports/zephyr/` content, file relocation, crypto/routing/framework/`DeviceBuilder`/`IRadio`, `Utils.cpp` internals.
- **Packet facts (for tests):** `HEADER_LENGTH` == 35, `DEV_ID_LENGTH` == 4, `RM_PROTOCOL_VERSION` == 4, `PACKET_LENGTH` == 256.

---

## File Structure

| File | Responsibility | Action |
|---|---|---|
| `platformio.ini` | Build envs | **Modify** — add `[env:native]` |
| `src/common/utils/RadioMeshCrc32.h` | CRC32 (header-only) | **Modify** — drop `<Arduino.h>` |
| `src/common/inc/Options.h` | Platform macro + base types | **Modify** — `byte` typedef in host branch |
| `src/common/inc/Logger.h` | Logging macros + `rmPrintf` | **Modify** — host output path + includes |
| `src/common/utils/Utils.h` | Utility decls + templates | **Modify** — guard Arduino RNG in one template |
| `test/test_Packet/test_Packet.cpp` | Host unit tests for the slice | **Create** |

---

## Task 1: Native test env + smoke test

Stand up the `native` env end-to-end before fighting the de-Arduino compile, so a green here means "the host toolchain + Unity + `int main()` harness work" and later reds are unambiguously about RadioMesh code.

**Files:**
- Create: `test/test_Packet/test_Packet.cpp`
- Modify: `platformio.ini` (append a new env)

**Interfaces:**
- Consumes: nothing (PlatformIO `native` platform, Unity).
- Produces: a runnable `pio test -e native` target and a `test/test_Packet/` suite that later tasks extend. `int main(int, char**)` is the test entry point.

- [ ] **Step 1: Write the smoke test**

Create `test/test_Packet/test_Packet.cpp`:

```cpp
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_env_smoke(void)
{
    TEST_ASSERT_EQUAL(1, 1);
}

int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_smoke);
    return UNITY_END();
}
```

- [ ] **Step 2: Add the `native` env**

Append to `platformio.ini` (after the last env):

```ini
[env:native]
platform = native
test_framework = unity
test_filter = test_Packet
lib_ldf_mode = off
build_flags =
    -std=gnu++17
    -I ./include
    -I ./src
```

Notes: `lib_ldf_mode = off` stops PlatformIO from auto-pulling the Arduino library graph from `lib_dir = ./src`; headers are found via the explicit `-I` paths. `test_filter = test_Packet` keeps the Arduino-only suites (`test_PacketTracker`, etc.) out of this env.

- [ ] **Step 3: Run the smoke test**

Run: `pio test -e native`
Expected: PlatformIO installs the `native` platform on first run, then:
```
test/test_Packet/test_Packet.cpp:... test_env_smoke   [PASSED]
...
1 test cases: 1 succeeded
```

- [ ] **Step 4: Commit**

```bash
git add platformio.ini test/test_Packet/test_Packet.cpp
git commit -m "build: add PlatformIO native host-test env" -m "Assisted-by: Claude Code (Opus 4.8)"
```

---

## Task 2: De-Arduino `RadioMeshCrc32.h` + host CRC test

`RadioMeshCrc32.h` includes `<Arduino.h>` but uses only `uint8_t`/`uint32_t`/`size_t` — never `byte`. A CRC-only test needs only this one fix to go green, so it is the smallest independent slice.

**Files:**
- Modify: `src/common/utils/RadioMeshCrc32.h:3`
- Modify: `test/test_Packet/test_Packet.cpp`

**Interfaces:**
- Consumes: `RadioMeshUtils::CRC32` — `void reset()`, `void update(const uint8_t* data, size_t length)`, `uint32_t finalize()`.
- Produces: nothing new for later tasks (proves the CRC header is host-clean).

- [ ] **Step 1: Add the CRC tests (they will fail to compile)**

In `test/test_Packet/test_Packet.cpp`, add the include at the top (below `#include <unity.h>`):

```cpp
#include <common/utils/RadioMeshCrc32.h>
```

Add these two test functions:

```cpp
void test_crc32_is_deterministic(void)
{
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    RadioMeshUtils::CRC32 a;
    a.update(data, sizeof(data));
    RadioMeshUtils::CRC32 b;
    b.update(data, sizeof(data));
    TEST_ASSERT_EQUAL_HEX32(a.finalize(), b.finalize());
}

void test_crc32_detects_single_bit_change(void)
{
    const uint8_t data1[] = {0xDE, 0xAD, 0xBE, 0xEF};
    const uint8_t data2[] = {0xDE, 0xAD, 0xBE, 0xEE};
    RadioMeshUtils::CRC32 a;
    a.update(data1, sizeof(data1));
    RadioMeshUtils::CRC32 b;
    b.update(data2, sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(a.finalize(), b.finalize());
}
```

(These are property tests — determinism and change-detection — deliberately NOT a hardcoded CRC constant, because this CRC32 reflects both input bytes and output and is not the standard zlib value.)

Update `main()` to run them:

```cpp
int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_smoke);
    RUN_TEST(test_crc32_is_deterministic);
    RUN_TEST(test_crc32_detects_single_bit_change);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to verify it fails (compile error)**

Run: `pio test -e native`
Expected: FAIL — compile error, because `RadioMeshCrc32.h` includes `<Arduino.h>`:
```
src/common/utils/RadioMeshCrc32.h:3:10: fatal error: Arduino.h: No such file or directory
```

- [ ] **Step 3: Drop the Arduino include**

In `src/common/utils/RadioMeshCrc32.h`, replace line 3:

```cpp
#include <Arduino.h>
```

with:

```cpp
#include <cstddef>
#include <cstdint>
```

This is unconditional (no guard): `<cstddef>`/`<cstdint>` exist on the ESP32/Arduino toolchains too, and `CRC32` uses no Arduino symbol.

- [ ] **Step 4: Run to verify it passes**

Run: `pio test -e native`
Expected: PASS
```
test_env_smoke                     [PASSED]
test_crc32_is_deterministic        [PASSED]
test_crc32_detects_single_bit_change [PASSED]
3 test cases: 3 succeeded
```

- [ ] **Step 5: Arduino regression gate**

Run: `pio run -e heltec_wifi_lora_32_V3`
Expected: `SUCCESS` (build unchanged). If it fails, some file relied on `RadioMeshCrc32.h` transitively including `<Arduino.h>`; add the missing include to *that* file rather than reverting.

- [ ] **Step 6: Commit**

```bash
git add src/common/utils/RadioMeshCrc32.h test/test_Packet/test_Packet.cpp
git commit -m "refactor: drop Arduino.h from RadioMeshCrc32 for host builds" -m "Assisted-by: Claude Code (Opus 4.8)"
```

---

## Task 3: De-Arduino the Packet header chain + round-trip test

`Packet.h` transitively pulls `Definitions.h` (needs `byte`), `Logger.h` (uses `OUTPUT_PORT`), and `Utils.h` (Arduino RNG in a template). These three block the host compile together, so they are one task ending in a green round-trip test.

**Files:**
- Modify: `src/common/inc/Options.h` (host branch, ~lines 7–10)
- Modify: `src/common/inc/Logger.h` (includes near line 3; `rmPrintf` body line 62)
- Modify: `src/common/utils/Utils.h` (`getRandomBytesArray`, ~lines 150–160)
- Modify: `test/test_Packet/test_Packet.cpp`

**Interfaces:**
- Consumes: `RadioMeshPacket` — default ctor; fields `protocolVersion`, `sourceDevId`/`destDevId`/`packetId`/`lastHopId`/`nextHopId` (`std::array<byte,4>`), `topic`/`deviceType`/`hopCount` (`uint8_t`), `packetCrc`/`fcounter` (`uint32_t`), `packetData` (`std::vector<byte>`); `std::vector<byte> toByteBuffer() const`; `explicit RadioMeshPacket(const std::vector<byte>&)`. Macros `HEADER_LENGTH`, `DEV_ID_LENGTH`, `RM_PROTOCOL_VERSION`.
- Produces: the packet slice now compiles host-side — the foundation M1 consumes.

- [ ] **Step 1: Add the round-trip test (it will fail to compile)**

In `test/test_Packet/test_Packet.cpp`, add the include (below the CRC include):

```cpp
#include <core/protocol/inc/packet/Packet.h>
```

Add the test:

```cpp
void test_packet_header_and_payload_roundtrip(void)
{
    RadioMeshPacket pkt;
    pkt.protocolVersion = RM_PROTOCOL_VERSION;
    pkt.sourceDevId = {0x01, 0x02, 0x03, 0x04};
    pkt.destDevId = {0x05, 0x06, 0x07, 0x08};
    pkt.packetId = {0xAA, 0xBB, 0xCC, 0xDD};
    pkt.topic = 0x10;
    pkt.deviceType = 0x02;
    pkt.hopCount = 3;
    pkt.packetCrc = 0x12345678;
    pkt.fcounter = 42;
    pkt.lastHopId = {0x11, 0x22, 0x33, 0x44};
    pkt.nextHopId = {0x55, 0x66, 0x77, 0x88};
    pkt.packetData = {0xDE, 0xAD, 0xBE, 0xEF};

    std::vector<byte> buf = pkt.toByteBuffer();
    TEST_ASSERT_EQUAL_UINT32(HEADER_LENGTH + 4, buf.size());

    RadioMeshPacket parsed(buf);
    TEST_ASSERT_EQUAL(pkt.protocolVersion, parsed.protocolVersion);
    TEST_ASSERT_EQUAL(pkt.topic, parsed.topic);
    TEST_ASSERT_EQUAL(pkt.deviceType, parsed.deviceType);
    TEST_ASSERT_EQUAL(pkt.hopCount, parsed.hopCount);
    TEST_ASSERT_EQUAL_HEX32(pkt.packetCrc, parsed.packetCrc);
    TEST_ASSERT_EQUAL_UINT32(pkt.fcounter, parsed.fcounter);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pkt.sourceDevId.data(), parsed.sourceDevId.data(), DEV_ID_LENGTH);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pkt.destDevId.data(), parsed.destDevId.data(), DEV_ID_LENGTH);
    TEST_ASSERT_EQUAL_UINT32(pkt.packetData.size(), parsed.packetData.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pkt.packetData.data(), parsed.packetData.data(), pkt.packetData.size());
}
```

Update `main()` to add `RUN_TEST(test_packet_header_and_payload_roundtrip);` before `return UNITY_END();`.

- [ ] **Step 2: Run — fails on `byte`**

Run: `pio test -e native`
Expected: FAIL — `Definitions.h` uses `byte` (line 29) with no host typedef:
```
src/common/inc/Definitions.h:29: error: 'byte' does not name a type
```

- [ ] **Step 3: Define `byte` in the host branch of `Options.h`**

In `src/common/inc/Options.h`, change the `#else` (host) branch from:

```cpp
#else
// generic build
#include <stdio.h>
#define RM_GENERIC_BUILD
#endif
```

to:

```cpp
#else
// generic build
#include <cstdint>
#include <cstdio>
typedef uint8_t byte;
#define RM_GENERIC_BUILD
#endif
```

- [ ] **Step 4: Run — now fails on `OUTPUT_PORT`**

Run: `pio test -e native`
Expected: FAIL — `Logger.h`'s `rmPrintf` uses `OUTPUT_PORT`, defined only under `ARDUINO`:
```
src/common/inc/Logger.h:62: error: 'OUTPUT_PORT' was not declared in this scope
```
(may also report missing `va_list`/`std::nothrow`).

- [ ] **Step 5: Add a host output path + includes to `Logger.h`**

In `src/common/inc/Logger.h`, immediately after `#include "Options.h"` (line 3), add (keyed on
the same `defined(ARDUINO)` condition that guards `OUTPUT_PORT`, so the two never diverge):

```cpp
#if !defined(ARDUINO)
#include <cstdarg>
#include <cstdio>
#include <new>
#endif
```

Then change the output line inside `rmPrintf` (line 62) from:

```cpp
    len = OUTPUT_PORT.write((const uint8_t*)buffer, len);
```

to:

```cpp
#if defined(ARDUINO)
    len = OUTPUT_PORT.write((const uint8_t*)buffer, len);
#else
    len = fwrite(buffer, 1, len, stdout);
#endif
```

This matches the existing `#if defined(ARDUINO)` guard that defines `OUTPUT_PORT`. The Zephyr-specific `printk` backend is a later (M1) refinement.

- [ ] **Step 6: Run — now fails on `random`/`randomSeed`**

Run: `pio test -e native`
Expected: FAIL — `Utils.h`'s `getRandomBytesArray` template calls Arduino RNG (clang rejects these non-dependent names at definition):
```
src/common/utils/Utils.h:154: error: use of undeclared identifier 'randomSeed'
src/common/utils/Utils.h:157: error: use of undeclared identifier 'random'
```

- [ ] **Step 7: Guard the Arduino RNG in `Utils.h`**

In `src/common/utils/Utils.h`, add after the includes (after line 6, `#include <vector>`):

```cpp
#ifndef RM_ARDUINO_BUILD
#include <cstdlib>
#endif
```

Then replace the body of `getRandomBytesArray` (lines ~150–160) so the RNG calls are guarded:

```cpp
template <std::size_t length>
std::array<byte, length> getRandomBytesArray()
{
    const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::array<byte, length> bytes;
#ifdef RM_ARDUINO_BUILD
    randomSeed(simpleRNG(4));
    for (std::size_t i = 0; i < length; i++) {
        bytes[i] = digits[random(36)];
    }
#else
    for (std::size_t i = 0; i < length; i++) {
        bytes[i] = digits[std::rand() % 36];
    }
#endif
    return bytes;
}
```

(A `std::rand`-based host path avoids declaring a global `random`, which would clash with POSIX `long random(void)`.)

- [ ] **Step 8: Run to verify it passes**

Run: `pio test -e native`
Expected: PASS — all four tests:
```
test_env_smoke                            [PASSED]
test_crc32_is_deterministic               [PASSED]
test_crc32_detects_single_bit_change      [PASSED]
test_packet_header_and_payload_roundtrip  [PASSED]
4 test cases: 4 succeeded
```

- [ ] **Step 9: Arduino regression gate (both boards)**

Run: `pio run -e heltec_wifi_lora_32_V3 && pio run -e seeed_xiao_esp32s3`
Expected: `SUCCESS` for both — the shims are guarded, so Arduino output is unchanged.

- [ ] **Step 10: Commit**

```bash
git add src/common/inc/Options.h src/common/inc/Logger.h src/common/utils/Utils.h test/test_Packet/test_Packet.cpp
git commit -m "refactor: make core packet slice compile without Arduino" -m "Guarded byte typedef (Options.h), host stdout path in Logger rmPrintf, and a host RNG branch in Utils getRandomBytesArray. Verified via the native host test and unchanged heltec/xiao Arduino builds." -m "Assisted-by: Claude Code (Opus 4.8)"
```

---

## Verification Summary (M0 done when all true)

- [ ] `pio test -e native` passes (4 tests) — the slice compiles + round-trips with no Arduino.
- [ ] `pio run -e heltec_wifi_lora_32_V3` builds.
- [ ] `pio run -e seeed_xiao_esp32s3` builds.
- [ ] Every shared-file edit is guarded (`RM_ARDUINO_BUILD`/`RM_GENERIC_BUILD` or `#if defined(ARDUINO)`), except the `RadioMeshCrc32.h` include which is safe on both toolchains.
- [ ] No Zephyr content, no file moves, no changes outside the six files listed.

## Notes / Deferred

- On-target Unity suites (`test_heltec_wifi_lora_32_V3` etc.) run on hardware; M0's automated gate is the *compile* (`pio run`), plus host tests. Running on-device tests is optional this milestone.
- `Utils.cpp` internals (e.g. `millis`, real RNG) remain Arduino-coupled; pulled in just-in-time by a later milestone.
- Logger's Zephyr `printk`/`LOG_*` backend lands in M1.
