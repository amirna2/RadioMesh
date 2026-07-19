# M1 Planning Handoff — RadioMesh → Zephyr Port

**Purpose:** Resume the **planning cycle** for Milestone 1 in a fresh (post-compaction) session with
zero re-derivation. Unlike the M0 handoff (which resumed *execution* of a finished plan), **M1 has
no spec or plan yet** — this doc kicks off brainstorming. Read this, then start the brainstorming
skill. Do NOT jump to writing a plan or code.

---

## TL;DR — how to resume

1. `cd /Users/nathoo/dev/RadioMesh`; confirm you're on `main`, clean, and it contains M0
   (`git log --oneline -3` shows the squash commit `Refactor/dearduinofy core slice (#39)`).
2. Read this handoff in full, then skim the "researched Zephyr facts" and "open questions" below.
3. Invoke the **`superpowers:brainstorming`** skill and run the M1 design conversation
   (one question at a time). The author (Amir) prefers to **resolve logistics/infrastructure before
   scope** — with M0 he wanted the repo-topology settled first; expect the same for the west
   workspace + build wiring here.
4. Brainstorm → write spec to `docs/superpowers/specs/YYYY-MM-DD-zephyr-port-m1-*.md` → get approval
   → `writing-plans` → then a NEW execution session (as with M0).
5. Create an M1 branch when the work starts (git-workflow: `<type>/<topic>`, e.g.
   `feat/zephyr-m1-bringup`). **Never commit to `main`.**

---

## State (as of 2026-07-18)

- **M0 DONE & MERGED to `main`** as squash commit `a4ccf40` (#39). The core packet slice
  (`RadioMeshPacket` + `CRC32` + Definitions/Logger/Utils chain) compiles + round-trips with **no
  Arduino** via `pio test -e native` (4/4), and heltec + xiao Arduino builds are unchanged. Every
  shared-file edit is guarded (`RM_ARDUINO_BUILD`/`RM_GENERIC_BUILD` / `#if defined(ARDUINO)`).
- **Also landed on `main`:** PR #36 from andrea3x (Italy user) — `PacketRouter.h/.cpp` +
  `Device.cpp` "tracking packet right after it's received." Routing is **not** in M1 scope (that's
  M2), so this is context, not a dependency — but the tree moved beyond just M0.
- Currently on `main`, working tree clean. No M1 branch yet. Nothing about Zephyr is on disk yet
  (no `zephyr/module.yml`, no `ports/zephyr/`).

## What M1 is (the tracer bullet)

**Zephyr bring-up + two-node LoRa packet exchange.** The first time the portable core meets a
genuinely foreign RTOS. Rough goal (refine in brainstorming): two Seeed XIAO ESP32-S3 + Wio SX1262
nodes, running **Zephyr**, exchange a `RadioMeshPacket` over LoRa — node A transmits on a topic
every N seconds; node B receives, parses via the buffer ctor, CRC-validates, and logs it. Success =
the M0 portable core drives a real Zephyr LoRa link. Wire-compatible framing (D5) means an Arduino
node could in principle receive the same packet — whether to *demonstrate* Arduino↔Zephyr interop in
M1 or defer it is an open question below.

Deliberately a tracer bullet: prove the end-to-end path (Zephyr build → co-compiled core → SX1262
driver → over-the-air → parse) thin but complete. No routing, no relay, no crypto, no inclusion.

## Project decisions that constrain M1 (inherited — see spec §1)

From `docs/superpowers/specs/2026-07-18-zephyr-port-milestone0-design.md` §1 (D1–D5):
- **D1** Monorepo evolved in place; same repo/name/URL; PlatformIO users undisturbed.
- **D2** Shared core by **co-compilation** — Zephyr's CMake compiles the *same* physical `src/`
  files as PlatformIO. No submodule, no copy. Wire-compat by construction.
- **D3** Dual-hat: PlatformIO reads root `library.json` (compiles `src/`); Zephyr reads root
  `zephyr/module.yml` + builds from `ports/zephyr/` (invisible to PIO). ← **M1 creates these.**
- **D5** Interop preserved (Zephyr + Arduino nodes coexist on one mesh).

## Researched Zephyr facts (carry forward — do NOT re-derive)

- **Board target:** `xiao_esp32s3/esp32s3/procpu` (Zephyr v4.4.0).
- **Radio driver:** `semtech,sx1262` via devicetree; API `<zephyr/drivers/lora.h>`
  (`lora_config`, `lora_send`, `lora_recv`, `lora_recv_async`).
- **Confirmed wiring (Wio SX1262 on XIAO):** SPI SCK=7, MISO=8, MOSI=9; CS=41, RST=42, BUSY=40,
  DIO1=39; DIO2→RF switch; DIO3→TCXO 1.8 V.
- **C++ on Zephyr:** `CONFIG_CPP`, `CONFIG_STD_CPP17`, `CONFIG_GLIBCXX_LIBCPP`. **No STL threads/
  mutex** — use `k_thread` / `k_msgq` / Zephyr primitives.
- **west topology:** T2 (manifest repo + module). `hal_espressif` **blobs are NOT needed** for a
  LoRa-only build (they're for WiFi/BT).
- **Prior art:** `liquidraver/zephcore`; `tensop-au/zephyr-esp32s3-lorawan-fuota` (has a ready
  XIAO+SX1262 overlay to crib the devicetree from).
- **Toolchain reality check (M0 lesson):** clang/libc++ (and Zephyr's libc) are stricter than the
  ESP32 Arduino toolchain — M0 already caught `random()` vs POSIX `long random(void)` and `byte` vs
  C++17 `std::byte`. Expect Zephyr's compiler to surface more of the same in any newly-compiled core
  file; the guarded-shim pattern from M0 is the template.

## Open questions for brainstorming (decisions to MAKE, not made)

1. **west workspace layout.** Is RadioMesh itself the T2 *manifest repo* (has `west.yml`), or a
   *module* consumed by a separate Zephyr app workspace that lives outside the repo? Where does the
   west workspace live relative to this repo? This is the "logistics first" question Amir will want
   settled early.
2. **App shape for two nodes.** One app with a role flag (TX vs RX chosen at build/runtime), or two
   tiny apps? Where do the app(s) live under `ports/zephyr/`?
3. **Co-compilation mechanics (D2 made concrete).** How does the Zephyr `CMakeLists.txt` pull the
   neutral core in? Which files does M1 actually need — just the header-only packet slice
   (`Packet.h` + `RadioMeshCrc32.h` + Definitions/Logger/Utils headers), or also `Utils.cpp`
   (`getVersion`/`convertToHex`/…)? M0 deliberately did NOT de-Arduino `Utils.cpp`; if M1 needs any
   of it, that de-Arduino work folds into M1 (guarded, same as M0).
4. **Radio abstraction.** Known issue: `IRadio` is leaky — TX/RX live on concrete `LoraRadio`, not
   the interface (deferred to M6). So M1's Zephyr radio code is almost certainly a **standalone
   Zephyr LoRa wrapper**, NOT an `IRadio` implementation yet. Confirm this scoping and where it
   lives (`ports/zephyr/...`). Do a quick read of `src/hardware/src/radio/LoraRadio.cpp` during
   brainstorming to see what the Arduino side does (RadioLib SX1262 config: freq, BW, SF, CR, sync
   word, preamble) so the Zephyr `lora_config` matches for interop.
5. **Logging backend.** M0 added a generic `stdout` path to `Logger.h`'s `rmPrintf`; the Zephyr
   `printk`/`LOG_*` backend was explicitly deferred to M1. Add it here (guarded), so core logging
   works on-device.
6. **Verification gate.** M0's superpower was a hardware-free host gate. M1 inherently needs
   hardware (2 boards) for the OTA test. What's the *automated* gate — "the Zephyr app compiles for
   `xiao_esp32s3`" — plus a manual 2-node bring-up procedure? Keep `pio test -e native` green too
   (M0 regression). Is Arduino↔Zephyr interop demonstrated in M1 or listed as a stretch/M-later?
7. **LoRa PHY parameters.** Pin exact freq/BW/SF/CR/sync-word/preamble so both nodes (and, for
   interop, the Arduino build) agree. Source of truth = the Arduino `LoraRadio` config.

## Likely scope boundaries (tentative — confirm/adjust in brainstorming)

- **In:** `zephyr/module.yml`, `ports/zephyr/` (app + CMake + devicetree overlay + prj.conf),
  Zephyr LoRa wrapper, Zephyr logging backend in `Logger.h`, whatever *minimal* extra core de-Arduino
  M1 needs (guarded), a build gate + 2-node procedure.
- **Deferred:** routing/relay (M2), crypto/MIC (M3), storage (M4), inclusion (M5), `IRadio` fix +
  DeviceBuilder/framework parity (M6), display (M7), wifi/portal (M8).

## Constraints

- **Git (git-workflow skill):** work on a feature branch off `main`; Conventional Commits; end every
  commit with exactly `Assisted-by: Claude Code (<model>)` (NOT `Co-Authored-By`); never commit to
  `main`; never commit `CLAUDE.md`/`AI_DEVELOPER.md`/`CONTEXT.md`; never merge PRs (Amir reviews +
  merges); explain git commands before running; least-invasive (no force-push / history rewrite /
  touching contributor forks).
- **Domain (from memory):** don't re-raise the static all-zero IV (known/deferred, security-audit
  item); no Bloom/cuckoo filters (dedup is LRU by design); no AODV / RREQ-RREP / proactive routing
  (routing is opportunistic by design) — and routing is out of M1 scope anyway.

## Environment prerequisites (planning should verify)

- Zephyr SDK + `west` installed and a workspace initialized? (Unknown — check early; may be a setup
  sub-step of M1.) Zephyr v4.4.0.
- 2× Seeed XIAO ESP32-S3 + Wio SX1262 boards on hand (Amir has the hardware).
- `pio` at `/Users/nathoo/.platformio/penv/bin/pio` (not on PATH) — for the M0 native regression and
  any Arduino-interop node.

## Reference docs & memory

- Project decisions: `docs/superpowers/specs/2026-07-18-zephyr-port-milestone0-design.md` §1.
- M0 pattern to reuse (guarded shims, two-gate verification): the M0 spec + plan +
  `docs/superpowers/plans/2026-07-18-zephyr-port-m0-HANDOFF.md` (its "After M0" section seeded these
  Zephyr facts).
- Memory: `project_zephyr_port.md` (whole-effort context + the native-env `lib_ignore` gotcha),
  `project_routing_*`, `project_inclusion_protocol_deliberate_design`, `feedback_git_least_invasive_explain_first`.

## Process reminder

M1 = **brainstorm → spec → (user approves) → writing-plans → (fresh session) execute**. This doc
gets you to the start of *brainstorm*. Nothing is designed yet; the open questions above are the
agenda, not answers.
