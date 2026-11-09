# P4.7: MAME fidelity — measured partial result

2026-11-07. No commit. MAME 0.285, US `epr-17952a`, factory NVRAM,
canonical single-cart power-on. No game variable, HP, scene or RNG seed was
forced. No P4.6 speed code was edited.

**The fidelity target is not achieved.** The initial RAM pattern is corrected,
and the checker now measures real port RAM. The audited BIOS correction passes
leave the first scene boundary unequal. The first divergence remains at MAME
frame 519 (port frame changes from 11 to 9 because boot initialization changes).
The paid seed difference remains three counts. This is a stopping report, not a
claim that combat or attract matches throughout.

## Measurement and patches

`work/scenario_port_hook.patch` was applied, rebuilt and recorded as
`port/runtime-patches/p47-scenario-capture.patch`, with `a/runtime/...` paths for
application inside the nested runtime clone. It is explicitly headless only,
off unless `STV_SCENARIO_PIPE` is set, read-only, and captures before VBlank-IN
interrupt delivery. It supplies full 2 MiB work RAM, the initial RNG contents,
all semantic fields and optional composed pixels. The physical TEST token is
also installed. `STV_SCENARIO_MEMORY=0` / `STV_SCENARIO_VIDEO=0` disable payloads.

The hook-enabled, otherwise unchanged P4.6 executable produced the **before**
measurements in `work/scenario_p47_baseline/`. All four runs have zero unavailable
port fields and full RAM comparisons. Its primary results reproduce v1.00's
port 11 / MAME 519 boundary. Unlike the old store-watch fallback, RNG is known
from the first sample. The first no-coin RNG mismatch *after MAME leaves the
pattern* is therefore observable at port 863 / MAME 1371, even before the port's
first PRNG write.

`port/runtime-patches/p47-bios-state-input-phase.patch` records the BIOS services
and their three tests. `port/main_stv.cpp` contains the HLE entry-state and bulk
RAM operations. Both patches reconstruct byte-for-byte against the saved P4.6
files with **zero fuzz** (`work/p47/patch-validation.log`). The README has the
application order. Checker/transport version is **1.01**; comparison fields,
comparison start rule, calibration and RAM masks are unchanged. The hook
regenerator now handles an already installed hook without duplicating it.

## BIOS boot inventory and changes

`work/p47/probe_boot.lua` installs read-only instruction-fetch taps at
`06005020` and its cache-through alias. On the first main-entry fetch it dumps
both RAM banks and CPU registers, then exits MAME. It performs no game writes.
The resulting inventory is `work/p47/boot-inventory.json`, with original RAM
images and registers alongside it. This observes the actual entry, rather than
mistaking an arbitrary frame-600 game snapshot for BIOS initialization.

The decoded IPL establishes the broad fill independently: ROM `0268..028C`
loads `53454741` and calls the longword fill at `0D00` using table `0494`:

| Operation | Range (end exclusive) | Evidence / result |
|---|---|---|
| Fill low WRAM | `00200000..00300000` | `0x40000` longwords, all SEGA in MAME |
| Fill high WRAM, first block | `0600C000..06100000` | `0x3D000` longwords |
| Fill high WRAM, second block | `06004000..0600C000` | `0x2000` longwords |
| Clear BIOS scratch | `06000B00..06000C00` | ROM `0282..0286`, 64 zero longwords |
| Copy BIOS program/services | into `06000000..06004000` and `06010000..06038000` | ROM `0288..0298`; menu program subsequently overwritten by cart |
| Copy full cart block | `06004000..06094000` | ROM `471A`, header count `0x24000` longwords |
| BIOS menu video workspace | `060F0000..060FB600` | ROM `8070..8084` copies ROM `66000` to `060F0000`; menu drawing also uses `060FA000` |
| Remaining untouched high RAM | `06094000..060F0000`, `060FB600..060FFF34` | SEGA except the cart trampoline's boot reason |
| Old BIOS master stack | `060FFF34..06100000` | 204 bytes of BIOS call residue at entry |

The HLE now performs both work-RAM pattern fills before loading the game.
Consequently `0609957C` receives the same initial bytes as every other untouched
longword; there is no special seed write. The extracted module is `0x8DE20`
bytes, while the BIOS copies `0x90000` bytes. The missing tail is now copied from
the mapped cart ROM, including its padding, rather than left at zero/pattern.

At the actual main-entry boundary MAME reports:

| Register | Value | Register | Value |
|---|---|---|---|
| PC/R0 | `06005020` | SP/R15 | `06100000` |
| SR | `00000001` | VBR | `06000000` |
| R1 | `060987CC` | R2 | `0600064C` |
| R3 | `FFFFFFFD` | R4 | `06094000` |
| R5 | `22120000` | R7 | `06005004` |
| PR | `00004516` | GBR | `FFFFFE00` |
| R6/R8..R14 | zero | MACH/MACL | zero |

The BIOS also leaves `SYS_GETSYSCK` (`06000324`) = 1 and VDP2 `TVMD` =
`0001`: the 352-dot clock, display disabled. The harness now sets the clock
through `onchip_set_clock(true)` and initializes that hardware register.
This fixes the startup 320-vs-352 capture geometry without writing game state.
The short 2,100-frame semantic records are identical before/after this clock
correction (`work/scenario_p47_clock/`); 55 of 56 reference PNGs also remain
byte-identical. Only the initial FBI-screen shot at VB 300 changes from 320x240
to 352x240 (`work/p47/clock-regression-equivalence.json`). It does not repair
the seed gap. The first screen divergence moves from 11/519 to 13/523; at that
later mismatch both captures are 352x240, so startup geometry is no longer the
explanation.

The harness now starts with these registers, including the correct master
stack. Previously it used the Saturn reset stack `06002000` and interrupt mask
15. It still supports an explicit `--sp` override. The existing power-on boot
reason comes from the cart trampoline; it was not adjusted to align a scenario.

Two further HLE service errors were confirmed in ROM:

- `0x634` is ROM `4DC0`, copying exactly eight bytes from
  `0600079E + 10*slot` to the caller. It was a no-op, leaving an SMPC/pattern
  buffer instead of the game's configuration. The factory setting bytes
  `1B 02 1B 00 1B 00 00 00` are copied from the cart header `06004F48`, matching
  BIOS `48B8..48DE`, and the service now copies them.
- `0x63C` is ROM `372C`, returning `20180100 + 0xF00*slot`, the slot's backup-RAM
  area. It is not a device query taking the caller's argument. Its former no-op
  return is corrected.
- The physical panel timeline and host panel now change pins at the raster
  boundary; BIOS raw records change when the guest calls `0x610`, as ROM
  `30114 -> 31498` does. Previous/current/press/release words are published there,
  and coin crediting follows the scanned rising edge. The records are initially
  cleared as ROM `3080` does. This avoids publishing BIOS RAM before the guest
  BIOS service has scanned the physical input. It does **not** by itself fix the
  compared START consumption phase.

**Remaining boot state is explicitly incomplete.** The HLE continues to use
sentinel vectors and service bodies, covered by the existing BIOS RAM mask. It
also does not execute the BIOS menu renderer or reconstruct its 46,592-byte
workspace and 204-byte old stack. Those latter regions remain visible to the
checker; they were not added to the mask or copied from a MAME snapshot. The
MAME on-chip/VDP2/backup register dumps are retained in `work/p47/`; they are
observations, not imported save states. In particular, the inherited FRT count
and BIOS backup-counter epoch are not restored by this HLE. Thus this task has an inventory of the
remaining state, **not a claim to reproduce every BIOS byte/register**.

## Timing diagnosis and stopping point

The seed is loaded from the game's `060987BC` logical frame counter by the cart
at `060098D8`, not directly from the SH-2 FRT. That counter advances when the
master enters `06027C74`, which then waits on VBlank gates. Loading work can
therefore lose logical counts while hardware VBlanks continue. A three-count
seed gap is not evidence of an incorrect PRNG multiply or an FRT prescaler bug.

Before the changes, with calibrated offset +508:

| Port frame | Port counter | MAME counter | Observation |
|---:|---:|---:|---|
| 316 | 155 | 155 | First attract scene 1 -> 2 anchor agrees |
| 321 | 156 | 156 | Still agrees during scene loading |
| 322 | 156 | 157 | First lost count in the load |
| 325 | 158 | 160 | Two counts lost |
| 600 | 433 | 435 | Two-count difference persists |
| 700 | 533 | 535 | START scheduled |
| 701 | 534 | 536 | Port has reset to scene 0; MAME still scene 2 with START edge |
| 702 | 534 | 537 | MAME resets and has seeded from 536; port seeded from 533 |

After the RAM/register correction, scene 1 starts at port 9 and ends at 314;
MAME still starts at 520 and ends at 824. Automatic calibration becomes **+510**.
Its first scene lasts 305 port vs 304 MAME samples. Changing only an overall boot
offset cannot make both edges agree. With that calibration, paid seed values
become **535 vs 538**, still a three-count difference. The BIOS settings/input
service correction produces the same primary boundary and seed gap.

The runtime clock in `machine.cpp` explicitly advances by `256 * 400 ns` per
master poll, counting compiler safe points at calls/back edges rather than SH-2
instructions, bus waits or cycles. Heavy scene initialization therefore has a
work-dependent duration unrelated to MAME's instruction timing. The missing two
logical counts arise during that load; the remaining count accompanies the
START sampling/task phase. This localizes the timing problem. It does **not**
establish a faithful replacement cycle schedule, and the precise within-frame
cause of the one-frame startup/START boundary remains unresolved.

Read-only probes: `work/p47/probe_timing.lua` / `mame-timing.log` and
`probe_phase2.lua` / `phase2.log`. The latter measures MAME entry at emulated
8.614812886 s. MAME's DRC bypasses ordinary write taps for fast RAM; missing tap
output is not interpreted as missing guest stores. A diagnostic `-nodrc` run
changes boot timing itself and is excluded from acceptance results. FRT/VDP2
register dumps similarly are not proof of instruction-level timing equality.

The audited boot/service corrections have provided no gain in the first divergence,
while RAM capture exposes additional BIOS/stack and load-phase differences.
I stopped further timing changes at this point. No seed compensation, delayed
scene write, scenario-specific pause, learned RAM mask or safe-point-duration
tuning was introduced. A follow-up needs instruction/cycle and bus-wait
accounting, BIOS service execution costs and a shared raster/input sample phase;
matching a guessed constant to these scenarios would not establish fidelity.

## Before/after scenario table

Final full-capture results are recorded in `work/scenario_p47_verified/`, using the
same scripts, automatic one-anchor calibration and unchanged RAM mask as before.
The intermediate state-only experiments are in `work/scenario_p47_memory/`,
`work/scenario_p47_bios/` and `work/scenario_p47_clock/`. The pre-clock full
capture is `work/scenario_p47_final/`.

The final hardware pass reruns all port captures. It reuses the completed MAME
captures from the pre-clock pass only after checking unchanged MAME/cart/input
hashes and independently measuring the same automatic +510 calibration.
`work/p47/recheck_hardware.py` uses the checker's capture/comparison functions;
`work/scenario_p47_verified/manifest.json` and per-engine `reuse.json` record
provenance. No later scene is realigned. The normal checker CLI also rechecks the
final saved captures. A fresh full two-engine reproduction is shown below.

| Scenario | Frames | Before: first field P/M | After: first field P/M | Coverage before -> after | Equal field records after |
|---|---:|---|---|---|---:|
| attract | 9300 | scene 11/519 | scene 9/519 | PASS -> PASS | 312/9292 |
| game_over | 5600 | scene 11/519 | scene 9/519 | PASS -> UNMET | 308/5592 |
| scene1_combat | 2100 | scene 11/519 | scene 9/519 | PASS -> PASS | 308/2092 |
| scene3_plus | 10000 | scene 11/519 | scene 9/519 | UNMET -> UNMET | 308/9992 |

All runs are **DIVERGED**, with zero unavailable port fields. Full masked RAM
equality is zero frames on every scenario. After calibration is +510; before
is +508. `scene3_plus` now misses scene 9 on **both** engines (before: port met
it, MAME missed it). `game_over` misses attract return on the port; MAME meets it.

| Additional diagnostic | Before | After |
|---|---|---|
| First screen mismatch, all scripts (P/M) | 11/519, 320x224 vs 352x224 | 13/523, both 352x240 |
| Paid short-script seeds (port / MAME) | 533 / 536 | 535 / 538 |
| Long-script seeds (port / MAME) | 464 / 467 | 466 / 469 |
| Initial RNG contents (port / MAME) | 0 / `53454741` | `53454741` / `53454741` |

The port frame numbers change with initialization. Compare the MAME boundary as
well: an earlier-numbered port divergence is not a gain in matched execution.
The v1.00 fallback's initially unknown RNG also must not be counted as agreement.

At the same MAME-519 boundary, masked differing bytes fall from **1,483,355 to
70,437**, and the entire low-RAM bank changes from 1,048,576 differing bytes to
zero. The first differing masked region moves from `00200000..0020FFFF` to
`06090000..0609FFFF`. This is a real initialization improvement, although it
never makes a whole-RAM frame equal. Evidence: `work/p47/ram-improvement.json`.
The corrected no-coin PRNG first produces `BF86696E`, then `8F29E2A3`, then
`39D64C21` on both engines; the port calls occur two aligned frames later.

## Regressions and verification

- Runtime: **131 tests, 0 failed** — all 128 existing tests plus three covering
  guest-timed BIOS scanning/edges, bounded per-slot settings copy and backup-area
  pointer selection (`work/p47/tests.log`).
- Checker: **12 tests pass** (`work/p47/checker-tests.log`). The normal CLI
  `--reuse` reproduces all four final results (`work/p47/recompare.log`).
  Artifact validation checks 56,040 captured frame records, all 62 retained PNGs
  against their RGB hashes and all 72 retained 2 MiB RAM images against their raw,
  masked and region hashes (`work/p47/artifact-validation.log`).
- Window: `port/p46_build_window.sh` **builds successfully**
  (`work/p47/window-build-final.log`). No GPU or desktop test was run.
- P4.6 speed patch reverse dry-run still applies, and its source files were not
  edited (`work/p47/speed-patch-preserved.log`, `speed-files.sha256`).
- All four P4.5 reference schedules pass with no FATAL, 56 valid PNGs and
  expected scene/credit-state coverage (`work/p47/regression-report-final.json`).
  No-coin reaches scenes 3..10 and returns to scene 1 at VB **8870**. The paid
  idle run reaches continue at **4033**, GAME OVER/state 0 at **5311**, attract
  scene 1 at **5613**, and the second paid game at **6120**. Combat and legacy
  multi-cart combat both spawn a paid player and reach director scene 3.
- These regressions are no longer frame/pixel-identical to P4.5: the initial
  memory and stack are deliberately corrected, and RNG/input timing changes the
  trajectory. For example the previous idle death VB 3615 becomes 4033, while
  the continue countdown is still exactly **1278** VBlanks. No attract-return or
  second-game progression target was weakened to hide this. The stricter
  **5,600-frame checker scenario now has an unmet port attract-return target**:
  its real return is at 5613, thirteen frames beyond that scenario. MAME meets
  the original target. This is retained as a coverage regression; the longer
  P4.5 reference passing does not make that checker result a pass.
- PNG evidence: `port/run/P46/p47-clock-nocoin/shot-9000.png` (FBI screen after a full
  loop), `port/run/P46/p47-clock-gameover/shot-5700.png` (returned attract),
  `port/run/P46/p47-clock-gameover/shot-6400.png` (second game intro), and
  `port/run/P46/p47-clock-combat/shot-2000.png` (fight with floor/ceiling rendering intact).

## Reproduction

Run from the repository root; existing local ROMs/MAME and generated recompile
are required. Use fresh capture output directories.

```bash
cmake --build port/run/P41/build-generated --target saturn_stv -j4
python3 tools/scenario/check_scenarios.py --version
python3 -m unittest discover -s tools/scenario -p 'test_*.py' -v
python3 tools/scenario/check_scenarios.py --port-hook --jobs 4 \
  --out work/scenario_p47_repro --ram-frames 1,7,9,11,13,314,316,702,1526 \
  --png 9,11,314,316,1526,2100,5000,7177,9300
# Expected exit 1: measured divergence, not a capture failure.
python3 tools/scenario/check_scenarios.py --reuse --out work/scenario_p47_verified
cmake -S tools/saturn-recomp/runtime/tests -B work/p47/runtime-tests
cmake --build work/p47/runtime-tests -j4
work/p47/runtime-tests/runtime_tests
port/p46_build_window.sh
bash port/run/P46/reg_runs.sh port/run/P41/build-generated/saturn_stv p47-repro
```

For the BIOS entry inventory, use an empty NVRAM/config destination:

```bash
LD_LIBRARY_PATH=port/run/P45/mamepkg/x/usr/lib/x86_64-linux-gnu \
  timeout 120 port/run/P45/mamepkg/x/usr/games/mame diehard -bios us \
  -rompath port/run/P45/mame/roms -video none -sound none -nothrottle \
  -skip_gameinfo -noreadconfig -nvram_directory work/p47/repro-nvram \
  -cfg_directory work/p47/repro-cfg -autoboot_delay 0 \
  -autoboot_script work/p47/probe_boot.lua
```

Original baseline: `work/p47/baseline.log` and
`work/scenario_p47_baseline/results.json`. Final: `work/p47/verified.log` and
`work/scenario_p47_verified/results.json`. Baseline source copies are
`work/p47/main.before.cpp`, `bios.before.cpp` and `video.before.cpp`; the first
RAM/register pass is `main.memory.cpp`. These make the intermediate changes
reviewable without reverting concurrent work. Nothing in `project/`,
`tools/decomp/` or `tools/glm/` was edited.
