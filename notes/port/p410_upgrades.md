# P4.10 player-facing upgrades

2026-11-08. Parts 1–3 complete; part 4 investigated and deferred. Paths and commands are repository-relative.
Existing unrelated worktree changes preserved; no commit.

## 1. Arcade touch cleanup — complete

`p410-arcade-touch.patch`, applied after the four P4.9 patches, gates Saturn
pad display, touch input and touch menu button on `!g_cfg.arcade`. The ST-V
pause menu has one column, including its sizing and section labels. Saturn
retains the existing two-column layout and touch settings behavior.

`port/p46_build_window.sh` passes (`work/p410/touch-build.log`). SDL dummy
video/software rendering with `[touch] controls = on` and `menu_button = true`
still shows no touch controls. Inspected `work/p410/touch/game.png`,
`pause.png` and `launcher.png`; the pause menu contains Resume/Quit only.
`work/p410/ui_driver.cpp` injects Escape through SDL events; the same run resumes
and reaches VB60. The launcher uses `SATURN_LAUNCHER_SHOT`. No guest/device
execution changes were made in this part. Full reference regression follows
at the end of the milestone.

## 2. Missing settings — complete

The launcher adds **Enable cabinet pause (F3)** and **Violence (blood)** under
Game settings. Both default off, matching USA factory settings. These apply at
next Start and persist as `[stv] pause_button` and `violence`. F3 sends the
cabinet pause switch; Escape/P still opens the host menu. The F3 binding is
editable as Cabinet pause (`[arcade] cabinet_pause`). Factory game settings
resets both switches. No actor state or cart instruction is patched.

MAME 0.285 US BIOS: fresh EEPROM for each independent menu change, exit all
menus to save, then reboot the saved EEPROM for gameplay. Exact inputs,
PNGs and NVRAM: `work/p410/mame/{factory,pause,violence}`; runner and Lua probe
are in `work/p410/mame/`. `pause-play` boots the saved pause EEPROM.

| Change | EEPROM data offsets (hex, both records) | BIOS work byte USA | Guest byte |
|---|---|---|---|
| Pause NO-USE -> USE | `20`, `5C`: `00 -> 01` | `060007A1` bit 0 | `060987EF` bit 0 |
| Violence OFF -> ON | `20`, `5C`: `00 -> 02` | `060007A1` bit 1 | `060987EF` bit 1 |

Checksums at `08/09`, `44/45` and sequence at `0E/4A` also change on save;
these are not settings data. The launcher follows P4.9's BIOS work-copy method,
selects the region using the cart's table, preserves unrelated flag bits, and
lets BIOS service `0x634` copy the eight-byte block normally. Source confirms:
`project/src_functional/022016ac.c` tests pause bit 0; `0222f6a8.c` and
`022308a0.c` test violence bit 1. Those files were read only.

`python3 port/p410_verify_settings.py` passes three native runs of **2200 VB**.
Report: `work/p410/settings/report.json`; log: `work/p410/settings-effects.log`.
Pause holds actor state through **175 sampled frames**; VB1350 and VB1450 PNGs
are byte-identical, and the second press resumes. MAME's frame1850/frame2050
PNGs are also identical while paused. Violence changes **7/76 sampled PNGs**,
with identical actors and PRNG in all **1001 captured frames**. At VB1800,
**156 pixels** differ: inspected red blood at the punch contact in
`work/p410/settings/violence/shot-1800.png`, absent in the factory image.
MAME assignment screens at frame1660 were inspected for both switch values.
The script requires Pillow for image comparison, as existing project tooling does.

Launcher edits were driven through real SDL events; both values reload as
true (`work/p410/settings-ui.ini`). Inspected `work/p410/settings-edited.png`
and `settings-final.png`, captured with SDL dummy + `SATURN_LAUNCHER_SHOT`.
Window builds pass, and **134 runtime tests, 0 failed** (`work/p410/tests.log`).
Runtime changes: `p410-game-settings.patch`; harness bytes: `port/main_stv.cpp`.

## 3. Quick save/load — complete

**F5** writes the game's quick slot; **F7** loads it. The pause menu has both
commands; the launcher has **Load quick save** beside Start. One slot is
provided for Die Hard Arcade. With a settings file the slot is
`SETTINGS.diehard.quickstate`; without one it is `OUT/diehard.quickstate`.
`--quick-state FILE` selects another location. `--load-state` loads it at boot.
Missing, stale or damaged states show a rejection message and boot normally.
`--save-at N` and `--load-at N` request operations at a post-poll boundary from
VBlank N onward, for deterministic verification. These switches are optional.

Example (the same options work with the macOS build):

```bash
port/play.sh --launcher --settings work/p410/player.ini
port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --window --settings work/p410/player.ini --load-state
python3 port/p410_verify_states.py
```

### Continuations and captured machine state

The old task-switch saver could not save an active slave; Die Hard has no
runtime-managed task-switch safe point. `port/p410_continuations.py` augments
the generated static recompile with **11,993 continuation labels** and
**6,124 guest return addresses**. Ordinary execution retains its C++ calls,
branches, delay slots and exact poll budget. A load unwinds the old master and
slave C++ stacks, resumes at the saved post-poll labels, and routes subsequent
returns using the guest PC and saved guest stack. Indirect branch targets and
delayed conditions that live across a poll are explicit context fields.
No native stack bytes, host pointers, interpreter or gameplay replay is used.
Snapshots occur outside master interrupts while the slave is parked. The
Die Hard worker yields at poll boundaries; other games' idle-FRT continuations
are outside this ST-V saver. Saturn's task-state API remains separate.

The **4,758,781-byte** tested state contains 21 named parts:

- Both SH-2 register sets, continuation fields, budgets, virtual time, interrupt
  counts, slave power/idle/slice state and the clock divider.
- Both 1 MiB work-RAM banks, active module identities, both cache RAMs and
  64 KiB backup RAM. BIOS vectors, EEPROM-derived work copies and game state
  are in those banks; BIOS credits, timer, physical pins, autoplay continuation
  and pending input events are also serialized.
- VDP1 VRAM, both framebuffers and draw/erase state; VDP2 VRAM, CRAM/registers;
  SCU interrupts/DMA/timers, external IRQ latches and DSP; both on-chip units.
- SCSP RAM, slots, envelopes, LFOs, timers, IRQs and DSP/carry; the full 68000
  register/flag/prefetch/interrupt state, sample cursor and cycle debt; pending
  sound output and the sound autostart flag.
- SMPC registers, command/interrupt state, peripheral transfer and input
  cursors/held pins; CD block, queues, transfer state and drive timers.

The native HLE has no separate serial EEPROM device: its EEPROM-derived BIOS
work copies are captured in WRAM. Cartridge ROM identity is checked against
both loaded images. External launcher INI files remain the next-start settings.
This preserves all modeled machine state; it does not add missing hardware.
Host audio is cleared/restarted after load, and pacing/statistics are rebased.
WAV recording appends the rewound audio, so a recording describes what played.

The **STVQUICK v1** header includes a source/generated-code/compiler/ABI and ROM
fingerprint, payload length, checksum and SH-2 context size. Named parts and
bounds are validated before applying a file; a temporary file and rename
replace the slot. Changing the runtime/build or ROMs invalidates old slots.

### Verification

`work/p410/states-final-verification.log` and `work/p410/states/report.json`:

| Check | Result |
|---|---|
| Save mid-combat VB1500, play to VB1800, load, compare VB1501..1800 | **300/300** identical frame hashes, full 2 MiB RAM hashes and semantic records |
| New process loads the file, compare VB1501..2000 | **500/500** identical pixels, RAM and semantic records |
| Audio following save/load | **368,574 stereo PCM frames**, byte-identical across original, rewind and cold load; non-silent |
| Stale version/build, truncated payload, checksum damage | **4/4** rejected cleanly before normal boot |
| Null-host build loads SDL build's slot | **4/4** PNGs match at VB1501/1550/1800/1900 |

Saved sample cursor: **1,102,700**; final cursor: **1,471,274**.
PCM SHA256: `c6e6d6a37bdc705c65714dbecd46734b42104240e5b03d8c74cc5865b71680c1`.
The verifier hashes every composed frame, rather than selected PNGs alone,
and compares unmasked RAM. Its warm run emits 800 post-save frame records,
including the original 300; the cold run emits another 500. Capture wall times
are retained in the report and are not isolated speed measurements.

Real SDL event injection verifies F5 then F7 and both pause-menu actions:
`work/p410/ui/run.log` records saves/loads at VB1528 and VB1576, then play to
VB1650, with **0 audio empty/drop/resync events**. Launcher Load is clicked in
`work/p410/ui/launcher-load.log`. Inspected `work/p410/ui/launcher.png`,
`pause.png` and `loaded.png` (dummy video/software rendering). A separate Saturn
host harness, `work/p410/saturn_ui.cpp`, confirms the pad and touch column
remain visible: inspected `work/p410/touch/saturn-touch.png` and
`saturn-pause.png`. No physical speaker/pad or desktop test is claimed.

`port/p46_build_window.sh` and the null-host build pass. **136 runtime tests,
0 failed**, including physical cabinet-pause edges and rejection without CPU/RAM
mutation (`work/p410/final-tests.log`). The three incremental P4.10 patches
reconstruct **15 runtime files byte for byte, zero fuzz**
(`work/p410/patch-validation.log`).

The build wrappers on Linux and macOS both run the idempotent continuation
pass; `port/p41_build.py --generate-only` regenerates it after emitting C++.
`port/stv.cmake` computes the compatibility fingerprint. The Mac bundle still
runs that same binary and persistent settings path; its packaging assumptions
and version are unchanged. Bash syntax and Python compilation pass; macOS
compilation and desktop execution are unavailable on this Linux host.

The initial all-instruction continuation prototype also passed all 56 reference
PNGs and four trace hashes, but inhibited compiler optimization. Only actual
continuations/returns are retained in the final code (binary 6.6 MiB versus
15 MiB for that prototype). Final route results are recorded below.

Final P4.10 reference regression: **56/56 PNG hashes and all four deterministic
trace hashes match the existing P4.9 reference**. All four runs exit 0, zero
FATAL. This preserves the P4.7-based default route and its documented historical
differences from original P4.5; it does not undo those earlier changes.
`work/p410/final-regression.log`; manifests:
`port/run/P48/p49-final/manifest.json` and
`port/run/P48/p410-final/manifest.json`.

| Schedule | VBlanks | PNGs | Wall seconds |
|---|---:|---:|---:|
| No-coin attract cycle | 9000 | 30/30 | 169.470 |
| Game over -> attract -> new game | 9000 | 16/16 | 166.834 |
| Combat | 2100 | 5/5 | 37.647 |
| Legacy multi-cart combat | 2100 | 5/5 | 37.666 |

These runs shared this loaded host with builds/captures and are correctness
checks, not isolated performance benchmarks. Attract returns at VB8870; paid
GAME OVER is VB5311, attract VB5613, second paid game VB6120. Runtime changes
retain the same scheduler turns and emulated timing.

## 4. BIOS TEST — investigated; implementation deferred

**Not feasible as a small addition to this milestone. TEST retains its existing
logged HLE exit; the launcher settings path is intact.** This is the requested
stopping point for part 4. No BIOS execution, reset or default boot code was
changed during this investigation.

### What is available and what currently runs

The local `diehard.zip` contains the **524,288-byte** US `epr-17952a.ic8`.
`python3 port/p410_bios_inventory.py --generate` extracts/word-swaps it into
`work/p410/bios/us.bin`, records hashes and IPL copy descriptors in
`inventory.json`, and regenerates an isolated ROM-only static recompile in
`work/p410/bios/recomp`. Current result: **691 functions / 65,144 instructions /
9 function C++ files**, with 223 unresolved indirect calls, 16 unresolved jumps,
11 static non-entry targets and 167 targets outside the ROM image. These are
analysis findings, not counts of proven runtime failures. This isolated output
was generated, not linked into the game or executed as a successful BIOS boot.
`notes/port/real_bios_spike.md` records an older runnable ROM-only experiment
with the same RAM-relocation boundary; its external scratch files were not used.

The game's actual generated `modules.cpp` registers **only `p_dh`**. The runtime
`bios_rom_read()` supplies 16 reset-vector bytes and three special logo pointers;
it supplies zero for ordinary firmware data. `sh2_lookup()`/`sh2_call_unknown()`
execute registered compiled functions, then HLE BIOS services, then fail on an
unknown target. There is **no C++ SH-2 instruction interpreter fallback**.
The Python `sh2emu.py` is a separate reference tool and is not a live runtime
CPU backend. The P4.10 continuation dispatcher resumes compiled code only.

### Why jumping to the BIOS entry is insufficient

`port/run/P45/bios_dis.txt` and the freshly extracted table at ROM `04B4..04E3`
show these IPL copies (end addresses exclusive):

| ROM source | Runtime destination | Bytes |
|---|---|---:|
| `00008000` | `06010000..06038000` menu program/data | 163,840 |
| `00000600` | `06000000..06000700` vectors/trampolines | 1,792 |
| `00030000` | `06000C00..06003000` BIOS services | 9,216 |
| `20001100` (cache-through ROM `00001100`) | `060002C0..060002E0` overlay | 32 |

The cart copy `06004000..06094000` overwrites the whole menu program. A TEST
transition must restore and activate it, replacing the active overlapping cart
module. It must also restore real vector/service data and the menu's video
workspace; jumping into current WRAM would execute cart bytes.

ROM **413A** (`*0600060C`) resets SP to `06100000`, updates BIOS mode bits, and
selects its next action. It never returns to the cart caller. ROM **32CE** is the
BIOS dispatcher; ROM **4704/471A** reloads the cart, and the cart's entry table
selects the new boot reason. ROM **8000** immediately uses RAM targets including
`06015278`, `060153DE`, `06024860`, `06013A34`, `0601547E`; ROM **8070** copies menu
video data from ROM `66000` to `060F0000`. The ROM-only recompile cannot dispatch
these relocated targets. The cart's boot-reason-2 assignment program, already
shown in P4.9, covers its game menu, not this BIOS shell and its system pages.

Further concrete runtime blockers:

- Full BIOS data reads and relocated code images are absent. Code dispatch only
  normalizes cache-through aliases; the real reset path also uses `E0000000`.
- SMPC SYSRES currently stops the run. ST-V's reset/re-entry handshake and GPIO
  inputs/EEPROM behavior need a real implementation. BIOS code accesses
  cache-through SMPC ports including `20100075/77`; the present runtime is
  console-pad/HLE-oriented and has no 93C46 serial EEPROM device.
- The harness catches ordinary program-start exceptions as a stop, rather than
  managing BIOS/cart re-entry. TEST must unwind the old master, stop/reset the
  slave and sound appropriately, switch modules/vectors, run BIOS, reload the
  cart and return through its correct trampoline. The new state continuations
  solve saved cart execution, not that firmware lifecycle.

Reproduced physical TEST at native VB500: exactly one logged
`*0x0600060C(1)` call, then ordinary game execution to VB650, no FATAL.
`work/p410/bios/native-test/run.log`; inspected `shot-510.png` (game intro, no
BIOS menu). MAME with physical TEST at frame600 shows the real **ST-V TEST MENU**
at frame750: inspected `work/p410/mame/bios-root/shot-750.png`. Its root lists
Memory/CRT/Input/Clock/Coin/System/Bookkeeping/Backup/EACH GAME pages. The real
Game Assignments pause/violence pages were inspected in part 2. No native BIOS
menu screenshot comparison is claimed, because the native BIOS path was not
implemented.

### Concrete follow-up plan and size

Estimate **7–15 engineering days plus unknown hardware/debugging work** (roughly
2–3 weeks), not a proven upper bound. ROM discovery alone is already 65k SH-2
instructions. Expect ROM plus at least two RAM execution ranges and the small
vector overlay, around **1.5–3k maintained lines** across runtime/build/device
integration plus tests, and substantially larger generated code. The estimate
is an implementation judgment, not a measurement.

1. **1–3 days: firmware mappings/execution.** Trace real BIOS instruction fetches
   in MAME across TEST and return, inventory all copied/built thunks, emit modules
   at their execution addresses, bind full ROM reads, and normalize code aliases
   in a BIOS-scoped path. Restore menu/service/workspace contents from firmware.
   A new portable C++ interpreter would be a separate alternative project; the
   current Python reference does not provide a cheap runtime fallback.
2. **2–4 days: ST-V device/persistence path.** Implement SMPC/GPIO board inputs,
   reset handshake and 93C46 serial operations; persist the 128-byte EEPROM with
   both records/checksum/sequence rules. Use the measured P4.9/P4.10 EEPROM files
   as tests. Establish how real menu writes and launcher selections share the
   persistent settings, so launching cannot silently overwrite a menu edit.
3. **2–4 days: BIOS/cart lifecycle.** Add a typed non-returning TEST transition to
   the harness, rebuild BIOS vectors, select/activate overlapping modules and
   re-enter the cart via its real boot-reason table. Reset/coordinate both CPUs,
   interrupts, timers and audio. Extend continuation generation to BIOS modules
   and the state format to any new EEPROM/device state before allowing saves in
   that mode. Keep the verified HLE launch path until the real route passes.
4. **2–4 days: acceptance.** Compare MAME root, Input, CRT, Clock, Coin, System
   and Game Assignments page PNGs; change/save/reboot every exposed setting;
   TEST from attract and combat, then exit into a new playable cart. Repeat
   56-shot/default trace checks, state/frame/PCM checks and launcher interaction.
   Stop and record any new unresolved relocation or hardware boundary rather
   than adding synthetic game writes.

The immediate blockers are executable RAM mappings, board/EEPROM I/O and
non-returning firmware/cart lifecycle. Leaving TEST logged preserves the
working launcher settings and the byte-identical default route.

## Reproduction and remaining limits

```bash
port/p46_build_window.sh
cmake --build work/p410/runtime-tests -j8
work/p410/runtime-tests/runtime_tests
python3 port/p410_verify_settings.py
python3 port/p410_verify_states.py
python3 port/p48_verify.py --binary port/run/P46/build-sdl/saturn_stv \
  --label p410-repro --schedules nocoin,gameover,combat,legacy-combat \
  --reference port/run/P48/p49-final/manifest.json
python3 port/p410_bios_inventory.py --generate
```

Settings verification first requires the real menu EEPROM captures under
`work/p410/mame/`; action files and `run.sh` retain the exact schedules.
The headless state verifier uses the existing read-only scenario pipe and
local FIFO capture; this is test tooling, not a platform API in the product.
P4.10 uses standard C++/Python and SDL3; macOS scripts are consistent but a Mac
build, desktop presentation, real pads and speaker audio remain hardware checks.
Existing P4.7 timing/rendering fidelity gaps and missing real EEPROM/BIOS
bookkeeping hardware remain. Quick states cover the modeled HLE machine and
are intentionally tied to its exact source/build/ROM identity.

No commit, sudo, GPU operation, prohibited-directory access or process killing
was performed. Only the requested source areas and generated evidence were
written; existing unrelated changes were preserved.
