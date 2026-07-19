# M1 Zephyr Bring-Up — Execution/State Handoff

**Purpose:** Resume M1 (Zephyr bring-up + 2-node LoRa exchange) in a fresh session with zero
re-derivation. Unlike the M1 *planning* handoff (`2026-07-18-zephyr-port-m1-planning-HANDOFF.md`,
committed `0b9c028`), this captures **mid-execution state after a real hardware bring-up**.

---

## TL;DR — where we are

**Single-node OTA verified on real hardware.** A Seeed XIAO ESP32-S3 + Wio-SX1262 running our
Zephyr firmware builds a `RadioMeshPacket` with the **co-compiled M0 core**, and **transmits it over
LoRa** on a real SX1262. Serial capture showed:

```
<inf> sx126x: SX126x initialized
=== RadioMesh M1 tracer bullet starting ===
LoRa device ready
<dbg> sx126x: Config: freq=915000000, SF=8, BW=125, CR=3, power=20
<dbg> sx126x: TX done
<inf> rm_m1: TX fcounter=0 len=38 ret=0          (38 = 35-byte header + 3-byte payload)
```

- ✅ Zephyr build → co-compiled `src/` core → SX126x native driver → **over-the-air TX**. The M1
  thesis is proven on silicon.
- ✅ PHY matches the Arduino interop contract: 915 MHz / SF8 / BW125 / CR_4_7 / power 20 dBm.
- 🔲 **2-node `RX … crc=OK` exchange** — NOT yet done; needs a **second board** (only one on hand).
  RX windows currently time out (`IRQ 0x0200`), which is correct with no peer.

## The two root causes that ate most of the session (do NOT re-derive)

1. **Real Wio-SX1262 pins are raw GPIOs 41/42/40/39** (all in the **gpio1 bank, GPIO≥32**):
   CS=GPIO41 `<&gpio1 9>`, RST=GPIO42 `<&gpio1 10>`, BUSY=GPIO40 `<&gpio1 8>`, DIO1=GPIO39
   `<&gpio1 7>`; SCK/MOSI/MISO=7/9/8 (board `spim2_default` pinctrl); DIO2→internal RF switch
   (`dio2-tx-enable`); DIO3→TCXO 1.8 V; RXEN=GPIO38 (external ant switch — only relevant for OTA,
   not init). Sources that all agree: RadioMesh Arduino preset `PinConfig(41,42,40,39)`, Meshtastic
   `variants/esp32s3/seeed_xiao_s3/variant.h`, XIAO Arduino variant identity pin numbering.
   *(A `GPIO4/3/2` mapping was tried and is WRONG — it only faked progress by dodging bug #2.)*
2. **Zephyr 4.4.0 `gpio_esp32` bug:** `gpio_pin_is_valid()`/`gpio_pin_is_output_capable()` use
   `BIT(pin)` (32-bit `1UL<<pin`), so any pin ≥32 evaluates to 0 → "Selected IO pin is not valid" →
   the SX126x driver can't configure its gpio1 control lines. **Fixed upstream (`main`) by switching
   to `BIT64()`.** We applied that 2-line fix to the west-managed tree; it is saved as
   `ports/zephyr/patches/0001-gpio_esp32-BIT64-valid-mask.patch`.

## Decisions locked during bring-up

- **LoRa backend = NATIVE** (`CONFIG_LORA_MODULE_BACKEND_NATIVE=y`), Zephyr's in-tree SX126x driver.
  Chosen over `loramac-node` (deprecated, per user) and `lora-basics-modem` (needs external module).
  Native needs no external module and gives granular init logs. → `loramac-node` can be dropped from
  `west.yml` (still listed; harmless, cleanup pending).

## Environment (all OUTSIDE the repo — already set up on this machine)

- **west venv:** `/Users/nathoo/dev/.venv-z312` (Python **3.12** — Zephyr 4.x needs ≥3.10; system
  py is 3.9, too old). west 1.5.0.
- **Zephyr SDK:** `/Users/nathoo/zephyr-sdk-1.0.1` (xtensa esp32s3 toolchain only).
- **west workspace topdir:** `/Users/nathoo/dev` (T2, `west init -l`); `zephyr/` + `modules/`
  cloned as siblings of `RadioMesh/`. Zephyr pinned `v4.4.0`.
- **build dir:** `/Users/nathoo/dev/.build-m1` (outside repo).
- Host tools via brew: cmake, ninja, dtc, gperf, ccache, libmagic.
- One-time deps done: `pip install -r zephyr/scripts/requirements.txt` + `west packages pip
  --install` (esptool 5.3.1).
- **After any `west update`, RE-APPLY the driver patch:**
  `cd /Users/nathoo/dev/zephyr && git apply /Users/nathoo/dev/RadioMesh/ports/zephyr/patches/0001-gpio_esp32-BIT64-valid-mask.patch`

## Build / flash / observe (verified working commands)

```bash
export PATH=/Users/nathoo/dev/.venv-z312/bin:/opt/homebrew/bin:$PATH
export VIRTUAL_ENV=/Users/nathoo/dev/.venv-z312
# build
west build -b xiao_esp32s3/esp32s3/procpu /Users/nathoo/dev/RadioMesh/ports/zephyr/app -d /Users/nathoo/dev/.build-m1
# flash (board enumerates as the ROM USB-Serial-JTAG, /dev/cu.usbmodem101)
west flash -d /Users/nathoo/dev/.build-m1 --esp-device /dev/cu.usbmodem101
```

**Flashing gotchas (hardware, hard-won):**
- The XIAO's single USB-C is the ESP32-S3 *native* USB. Factory Meshtastic exposes its own CDC
  (`/dev/cu.usbmodemE80690A1055C1`); once our Zephyr (USB-Serial-JTAG console) is flashed, the port
  is the ROM/JTAG `/dev/cu.usbmodem101` and `west flash` connects cleanly via the JTAG reset.
- **First flash over Meshtastic** needed manual download mode: hold **BOOT (left button)**, unplug +
  replug USB, keep holding ~2 s. (There is no reliable auto-reset over the app CDC.)
- After `west flash` ("Hard resetting via RTS pin") the board often does NOT cleanly boot the app —
  do a **plain unplug/replug (no buttons)** to boot.
- **USB-Serial-JTAG discards TX before the host attaches (~1.5 s).** To see boot-time driver logs,
  `prj.conf` currently sets `CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS=3000` (deferred flush) +
  `CONFIG_LORA_LOG_LEVEL_DBG=y`, and `main.cpp` sleeps 2 s before its first `printk`. These are
  DEBUG scaffolding — see cleanup below.

**Serial capture (host holds only one opener; close other monitors first):** use a pyserial reconnect
loop that reopens `/dev/cu.usbmodem*` across the replug and reads ~30 s. (Reference script is in the
session transcript; `west espressif monitor` also works once booted.)

## Git state

- Branch **`feat/zephyr-m1-bringup`**, 3 commits: `fcf6ce1` (scaffold), `0b9c028` (planning handoff),
  `0674a99` (fix: `<algorithm>` + `loramac-node` — from the earlier compile-under-Zephyr work).
- **Uncommitted in-repo** (the working, hardware-verified config):
  `ports/zephyr/app/boards/xiao_esp32s3_procpu.overlay` (correct 41/42/40/39 pins),
  `ports/zephyr/app/prj.conf` (native backend + debug logging), `ports/zephyr/app/src/main.cpp`
  (debug scaffolding), plus the new `ports/zephyr/patches/…patch` and this handoff.
- **Out-of-repo:** `~/dev/zephyr/drivers/gpio/gpio_esp32.c` patched live (BIT64). Captured as the
  patch file above.
- Not pushed. `main` not touched. (git-workflow: feature branch, Conventional Commits,
  `Assisted-by: Claude Code (<model>)` trailer, never commit `CLAUDE.md`/`AI_DEVELOPER.md`/`CONTEXT.md`,
  never merge PRs, explain git before running.)

## Remaining M1 tasks

1. **2-node OTA** (the actual M1 finish): flash the identical image to a **second** board, confirm
   each logs the peer's `RX len=… crc=OK`. (Symmetric TX+RX firmware already does this; RX just needs
   a transmitter.) May need to drive/confirm RXEN=GPIO38 or rely on DIO2 RF-switch if RX doesn't hear.
2. **Zephyr-patch handling decision** (reproducibility): either (a) bump `west.yml` Zephyr revision to
   a release that already contains the `BIT64` fix, or (b) keep pinned 4.4.0 + carry the patch +
   document `git apply` in `ports/zephyr/README.md`. Pick one; (a) is cleaner if a fixed tag exists.
3. **Cleanup before final commit:** strip debug scaffolding from `main.cpp` (2 s delay, `printk`
   FATAL loop, keep a clean startup delay only if needed for the JTAG race) and `prj.conf`
   (`LOG_PROCESS_THREAD_STARTUP_DELAY_MS`, `LORA_LOG_LEVEL_DBG` → back to sane defaults); drop
   `loramac-node` from `west.yml`.
4. **Commit** the working overlay/prj.conf/main.cpp + patch + README update; keep M0 gates green
   (`pio test -e native`, `pio run -e seeed_xiao_esp32s3`) since shared `src/` wasn't touched here.
5. Finish with the finishing-a-development-branch flow (PR for user review; never merge).

## Reference

- Whole-effort memory: `project_zephyr_port.md` (updated with M1 hw-verified state + these findings).
- M0 spec/plan + M1 planning handoff in `docs/superpowers/`.
- Meshtastic pin source: `meshtastic/firmware` → `variants/esp32s3/seeed_xiao_s3/variant.h`.
