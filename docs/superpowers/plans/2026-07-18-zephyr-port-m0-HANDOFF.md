# M0 Execution Handoff — RadioMesh → Zephyr Port

**Purpose:** Resume **inline execution** of Milestone 0 in a fresh (post-compaction) session with
zero re-derivation. Read this doc, then the plan, then execute. Everything needed is in-repo.

---

## TL;DR — how to resume

1. `cd /Users/nathoo/dev/RadioMesh`, then `git checkout refactor/dearduinofy-core-slice`.
   Verify: `git status` is clean and `git log --oneline -3` shows the three docs commits listed
   under **State** below.
2. Read the plan: `docs/superpowers/plans/2026-07-18-zephyr-port-m0-dearduinofy.md`.
   (Optional deeper context: spec `.../specs/2026-07-18-zephyr-port-milestone0-design.md`.)
3. Invoke the **`superpowers:executing-plans`** skill and work Task 1 → 2 → 3 in order. TDD:
   write test → run to see it fail → apply the fix → run to green → run the Arduino gate → commit.
   Checkpoint with the user between tasks.
4. Commit per the **`git-workflow`** skill: conventional-commit prefix, one-line trailer
   `Assisted-by: Claude Code (<model>)`, **never commit to `main`**, stay on this branch.

---

## State (as of 2026-07-18)

- **Branch:** `refactor/dearduinofy-core-slice` (branched off `main`).
- **Commits so far — DOCS ONLY, no source code changed yet:**
  - `004e184` — spec
  - `5c0a08d` — spec correction (real de-Arduino surface)
  - `1253cd7` — implementation plan
- Working tree clean. Nothing pushed. `main` untouched.
- **Execution mode chosen by user: inline (option 2).**

## What M0 is

Make the protocol core's **packet slice** compile with **zero Arduino dependency**, guarded so
the Arduino build is byte-identical, and prove it with a new host `native` test.

**Two gates — both must stay green:**
1. `pio run -e heltec_wifi_lora_32_V3` **and** `pio run -e seeed_xiao_esp32s3` still build.
2. `pio test -e native` compiles the slice with no Arduino and passes.

## The 6 files (exact changes are in the plan)

| File | Change |
|---|---|
| `platformio.ini` | add `[env:native]` (platform=native, no framework, unity, `test_filter=test_Packet`, `lib_ldf_mode=off`, `-std=gnu++17 -I ./include -I ./src`) |
| `src/common/utils/RadioMeshCrc32.h` | drop `#include <Arduino.h>` → `<cstddef>`+`<cstdint>` (unconditional; uses no `byte`) |
| `src/common/inc/Options.h` | in the `#else` (`RM_GENERIC_BUILD`) branch add `#include <cstdint>` + `typedef uint8_t byte;` |
| `src/common/inc/Logger.h` | host output path in `rmPrintf` + guarded `<cstdarg>/<cstdio>/<new>` |
| `src/common/utils/Utils.h` | guard the Arduino RNG (`random`/`randomSeed`) in `getRandomBytesArray` |
| `test/test_Packet/test_Packet.cpp` | **create** — host tests (smoke, CRC props, Packet round-trip) |

## Non-obvious gotchas (why the plan looks the way it does)

- **`byte` is an Arduino typedef.** Off-Arduino it's undefined; `Options.h` already has a dead
  `RM_GENERIC_BUILD` (`#else`) branch — that's where the `byte` typedef goes.
- **`Logger.h` does not compile off-Arduino as-is.** `rmPrintf` (defined unconditionally) calls
  `OUTPUT_PORT.write(...)`; `OUTPUT_PORT` is `#define`d **only** under `defined(ARDUINO)`. Add an
  `#else` path `fwrite(buffer, 1, len, stdout)`. Key the new includes on the **same**
  `!defined(ARDUINO)` condition (not `RM_ARDUINO_BUILD`) so guard and output can't diverge.
- **`Utils.h` has Arduino RNG inside a template.** `getRandomBytesArray` calls `random()` /
  `randomSeed()`. These are *non-dependent* names → **clang (the macOS `native` compiler) rejects
  them at template definition, even though nothing instantiates it.** Fix: guard the RNG loop and
  use `std::rand()` on host. Do **not** declare a global `random(long)` — it clashes with POSIX
  `long random(void)`.
- **CRC32 is non-standard** (reflects both input bytes and output). Do NOT assert a canonical
  CRC-32 constant like `0xCBF43926`; the plan uses property tests (determinism, change-detection).
- **The native test must NOT use the Arduino harness.** Use `int main()` (+ empty
  `setUp`/`tearDown`), include slice headers **directly** (`Packet.h`, `RadioMeshCrc32.h`) — never
  the `RadioMesh.h` umbrella (it pulls RadioLib/WiFi) — and define **no** `RM_LOG_*` (log macros
  stay no-ops, so the stdout path isn't even exercised).
- **Existing `test_*` suites are Arduino-only** (`#include <RadioMesh.h>`, `setup()/loop()`).
  `test_filter = test_Packet` keeps them out of the `native` env.
- The round-trip slice is **header-only** — the `native` env compiles no `src/*.cpp`
  (`Utils.cpp` is untouched this milestone).

## Constraints / out of scope (do NOT do in M0)

- No Zephyr / `ports/zephyr/` content, no `zephyr/module.yml` (that's M1).
- No file relocation or directory restructure.
- No changes to crypto, routing, framework, `DeviceBuilder`, `IRadio`, or `Utils.cpp` internals.
- No Zephyr `printk`/`LOG_*` logging backend (M1) — M0 adds only the generic stdout path.

## Environment prerequisites

- PlatformIO CLI (`pio`) installed and on PATH.
- Internet access for the **first** `pio test -e native` (PlatformIO fetches the `native`
  platform on first use).
- **No ESP32 hardware needed for M0** — the Arduino gate is a *compile* (`pio run`), and the
  functional tests run on the host. (On-device Unity suites are out of scope this milestone.)

## Definition of done

All boxes in the plan's "Verification Summary" green: `pio test -e native` passes (4 tests);
both board builds succeed; all shared-file edits guarded; nothing changed outside the 6 files.

## After M0 (context for later, not now)

M1 = Zephyr bring-up + two-node LoRa exchange. Facts already researched (see spec §1 / memory
`project_zephyr_port.md`): board target `xiao_esp32s3/esp32s3/procpu`; SX1262 wiring SPI
SCK7/MISO8/MOSI9, CS41/RST42/BUSY40/DIO1 39, DIO2→RF-switch, DIO3→TCXO 1.8V; driver
`semtech,sx1262` + `<zephyr/drivers/lora.h>`; pin Zephyr v4.4.0; prior art `liquidraver/zephcore`
and `tensop-au/zephyr-esp32s3-lorawan-fuota` (ready overlay).
