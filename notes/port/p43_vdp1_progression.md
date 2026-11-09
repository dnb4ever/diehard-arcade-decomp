# P4.3 — faithful VDP1 V-Blank erase (`TVMR.VBE`) and second-area progression

2026-10-31. Tested in this workspace on the build host. No commit made.
All runs use `STV_BOOTMODE=0 STV_CFG798=40`; paid runs use physical coin/START input.
No actor, HP, scene, progress, descriptor, director variable, or VDP1 command was forced.

The stale VDP1 geometry seen across `y = 0..99` during camera cuts and in the
second area (`port/run/P42/final-progression/shot-3970.png`, `shot-4200.png`)
is eliminated by emulating VDP1 `TVMR` bit 3 (`VBE`, V-Blank Erase/Write Enable)
in `tools/saturn-recomp/runtime/src/video/vdp1.cpp`. With scripted input
extended past the second area (`port/run/P43/stage-clear-input.txt`), P1 clears
Scene 5, passes the Scene 6 hallway QTE (`"SUCCESS"`), transitions through
Scene 8, and engages the next enemy wave in Scene 9 (third combat area) with no
runtime or BIOS gate blocking progression.

## Root cause: `vdp1.cpp` ignored `TVMR` bit 3 (`VBE`) in manual frame-change mode

### 1. Why only `y = 0..99` accumulated stale VDP1 pixels

At the start of every VDP1 command list (`project/src_functional/02215000.c`,
`vdp1_cmdlist_header_init`), the game writes four setup commands into VDP1 VRAM
at `0x25C00000`:

| Command index | `CTRL` | Meaning | Coordinates |
|---:|---|---|---|
| `cmd[0]` | `0x0009` | System clipping | `(0, 0) .. (704, 240)` |
| `cmd[1]` | `0x0008` | User clipping | `(0, 0) .. (704, 240)` |
| `cmd[2]` | `0x000A` | Local coordinates | `(352, 120)` |
| `cmd[3]` | `0x0004` | Non-textured black polygon (`PMOD = 0x00C0`, `COLR = 0x0000`) | `(-352, -20) .. (352, 120)` -> screen `(0, 100) .. (704, 240)` |

Because `cmd[3]` uses local origin `(352, 120)` with top vertices `y = -20` and
bottom vertices `y = +120`, the software clear polygon only covers scanlines
`y = 100..240`. The top 100 scanlines (`y = 0..99`) rely entirely on the VDP1
hardware framebuffer erase engine, which `vdp1_ clear_framebuffers`
(`project/src_functional/02214b24.c`) programs at boot and scene init with:

- `EWDR = 0x0000` (transparent clear value)
- `EWLR = 0x0000` (`x1 = 0, y1 = 0`)
- `EWRR = 0x58F0` (`x3 = 703, y3 = 240`)

### 2. How *Die Hard Arcade* drives VDP1 framebuffer swap and erase at 30 fps

During gameplay, *Die Hard Arcade* runs at 30 fps (2 VBlanks per game frame) in
manual frame-change mode (`FBCR = 0x0003`: `FCM = 1, FCT = 1`, `PTMR = 0x0002`).
Rather than using `FBCR = 0x0002` (`FCM = 1, FCT = 0`) to erase across an entire
display field, the game's VBlank-IN and VBlank-OUT ISRs use **V-Blank Erase**
(`TVMR` bit 3, `VBE = 0x0008`) on the swapping VBlank:

1. In `isr_vblank_in` (`project/src_functional/02223cf0.c`), when a new game
   frame is ready to swap (`g_0609869C == 1` and `g_060E0BC8 == 1`, every second
   VBlank):
   - Writes `TVMR = g_060E0E30 | 8` (`0x0009`: `VBE = 1, TVM = 1`) at
     `0x25D00000`.
   - Writes `FBCR = g_060E0E32 | 3` (`0x0003`: `FCM = 1, FCT = 1`) at
     `0x25D00002`.
2. In `isr_vblank_out` (`project/src_functional/02223d98.c`), at the end of the
   same VBlank when `g_0609869C == 1` and `g_06098694 == 1`:
   - Writes `TVMR = g_060E0E30` (`0x0001`: `VBE = 0, TVM = 1`) at `0x25D00000`.
   - Kicks the next VDP1 command list draw (`PTMR = 2`).

Per the Sega Saturn VDP1 User's Manual (Section 4.1, *TV Mode Selection Register
(TVMR)*, bit 3 `VBE`) and MAME (`src/mame/sega/saturn.cpp`
`saturn_state::video_update`, checking `STV_VDP1_VBE && STV_VDP1_FCM`), setting
`VBE = 1` in manual mode (`FCM = 1`) erases the display framebuffer during the
VBlank period just before `FCT = 1` swaps it into the draw buffer at
`VBlank-OUT`. On the non-swapping VBlank of each 30 fps pair, `VBE` is `0` and
`FBCR` is not written, so the newly swapped display buffer stays intact across
both VBlanks.

`tools/saturn-recomp/runtime/src/video/vdp1.cpp` previously checked only
`g_erase_field` (set by `one_cycle` or `FBCR = 2`) in `vdp1_vblank_out()` and
ignored `g_reg[TVMR] & 8`. As a result, `erase()` never executed during
gameplay, and `y = 0..99` retained every sprite and polygon ever drawn into each
framebuffer:
- At VB 1320 (`shot-1320.png`), the red/grey intro helicopter from Scene 2
  remained stuck in `y = 0..99`.
- At VB 3970 (`shot-3970.png`), the Scene 3 rooftop wall and the previous camera
  cut's helicopter remained stuck in `y = 0..99`.
- At VB 4200 (`shot-4200.png`), Scene 4's brown door (`y = 0..99`) and prior
  frames' rotated blades from Scene 5's two foreground ceiling/exhaust fans
  (`models[1034]` / `cmd[80,81,83,84,86,88]`, 24x24 8-bpp curved fan blade
  sprites at `srca = 0x77B0`, and `models[1004]` / `cmd[51..77]` Y-brackets)
  smeared across `y = 0..99`. (The live current-frame blades of those two
  foreground fans remain present in Scene 5 as authored 3D stage props.)

### 3. Fix

In `tools/saturn-recomp/runtime/src/video/vdp1.cpp::vdp1_vblank_out()`:

```cpp
if (g_erase_field || (g_reg[TVMR] & 8)) { erase(g_draw_fb ^ 1); g_erase_field = false; }
```

The retired `STV_VDP1ERASE` override in `vdp1_vblank_out()` (which forced
`g_erase_field = true` on every manual swap and blanked odd VBlanks at 30 fps)
is also removed.

Patch: `port/runtime-patches/p43-vdp1-vbe-erase.patch`.
Unit test: `vbe_erases_display_buffer_during_vblank_in_manual_mode` in
`tools/saturn-recomp/runtime/tests/frame_change_test.cpp` (`126 tests, 0 failed`).

## Stretch: clearing the second area (Scene 5) and advancing to Scene 9

### 1. Why the P4.2 6000-VBlank run stopped in Scene 5

Scene 5's enemy wave script starts at `0x0606143C` (`project/src_functional/022463b8.c`):

| Record address | Spawn `(X, Z)` | Type / Width | Limit (`g_060973e8`) | Flags | HP | Delay | Meaning |
|---|---|---|---|---|---:|---:|---|
| `0x0606143C` | `(160, 96)` | `0` (`w=10`) | `40` | `0x00` | `85` | `0` | Wave 1 `slot=3` (`col: 0 -> 10`) |
| `0x06061450` | `(160, 112)` | `1` (`w=10`) | `40` | `0x41` | `85` | `0` | Wave 1 `slot=4` (`col: 10 -> 20`) |
| `0x06061464` | `(240, 96)` | `4` (`w=15`) | `0` | `0x01` | `113` | `2` | Waits for Wave 1 clear (`col == 0`), spawns Wave 2 `slot=3` (`col: 0 -> 15`) |
| `0x06061478` | `(240, 96)` | `1` (`w=10`) | `15` | `0x62` | `85` | `60` | 2P-only record (`flags & 0x20`); skipped in 1P mode (`g_06098564 == 1`) |
| `0x0606148C` | `(0, 0)` | `0` | `0` | `0x00` | `0` | `0` | End of timeline (`ptr == 0`) -> installs `func_022465fc` |

In the P4.2 6000-VBlank run:
- Wave 1 (`slot=3` `hp=85`, `slot=4` `hp=85`) was defeated by VB 5556.
- Wave 2 (`slot=3` `hp=113`) spawned at VB 5688, and 60 ticks later the wave
  task reached `0x06061478`, skipped the 2P-only marker (`flags & 0x20`), saw the
  terminator at `0x0606148C`, and installed `func_022465fc`
  (`project/src_functional/022465fc.c`) to wait for `*(s32*)0x0609958C == 0`.
- By VB 6044 (when attacks continue past VB 5900), `slot=3` was down to `hp=22`,
  but P1 had advanced to the `+X` boundary (`x = 0x000B2666`, facing `+X` into
  the left wall) while `slot=3` walked to `x = 0x00094ED0` (`-X`, to the right on
  screen, behind P1). Because spamming `P1A`/`P1B` every 12 VBlanks kept P1 in
  attack animations facing `+X`, `slot=3` hit P1 from behind at VB 6198.

### 2. Scripted clear of Scene 5 and subsequent scene progression

Pausing `P1A`/`P1B` during `VB 6070..6125` and holding `6090:P1RIGHT:30` turns
P1 toward `-X` (`x = 0x00097B9B`) as `slot=3` approaches, while keeping the
entire input prefix through VB 6060 identical to P4.2:

| VB | Observation |
|---:|---|
| 4003 | Director advances `scene 4 -> 5`. |
| 4005 | Scene 5 Wave 1 spawns (`slot=3` HP 85, `slot=4` HP 85, P1 HP 36). |
| 5340 | Wave 1 `slot=3` reaches HP 0 (`col: 20 -> 10`). |
| 5556 | Wave 1 `slot=4` reaches HP 0 (`col: 10 -> 0`). |
| 5688 | Scene 5 Wave 2 `slot=3` spawns at HP 113 (`timeline=06061478`, `col=15`). |
| 6044 | Wave 2 `slot=3` reaches HP 22 (`5754: 98`, `5772: 82`, `5954: 67`, `5972: 52`, `6026: 37`, `6044: 22`). |
| 6090 | Script holds `P1RIGHT:30` (with `P1A`/`P1B` paused `6070..6125`); P1 turns toward `-X`. |
| 6144 | Wave 2 `slot=3` reaches HP 7. |
| 6162 | Wave 2 `slot=3` reaches HP 0; Scene 5 enemies are defeated. |
| 6288 | `slot=3` slot clears (`*(s32*)0x0609958C == 0`, `arm(0x670) = 0`); `func_022465fc` sets content entity `+0x3C` bit 3. |
| 6368 | Director advances `scene 5 -> 6` (`gate(0x86e) = 2`, hallway QTE cutscene). |
| 6691 | Scripted `P1A`/`P1B` presses pass the Scene 6 QTE (`"CAUTION"` -> `"パンチ"` -> `"SUCCESS"`); P1 HP increases `2 -> 12`. |
| 6945 | Director advances `scene 6 -> 8` (corridor entry cutscene). |
| 7020 | Director advances `scene 8 -> 9` (`gate(0x86e) = 1`, third combat area: interior lobby). |
| 7022 | Scene 9 Wave 1 spawns (`slot=3` HP 113, `slot=4` HP 113; `slot=5` HP 92 joins at VB 7083). |
| 7333 | P1 damages Scene 9 `slot=4` (`HP 113 -> 93`). |

**No runtime or BIOS emulation gate stops progress after clearing the second
area.** Scene 5 completion, the Scene 6 interactive QTE, the Scene 8 transition,
and Scene 9 enemy wave spawning and combat all execute faithfully on the
existing runtime.

## Reproduce from the repository root

Patch order against upstream `b22688d0bef52fba31cda79fd23150e8905cf3c2`:
`round-runtime-dh-portB.patch`, `p41-raw-panel-input.patch`,
`p41-player-trace.patch`, `p42-rbg-map-registers.patch`,
`p42-dmac-block-count.patch`, `p42-remove-retired-bridges.patch`,
`p43-vdp1-vbe-erase.patch` (all under `port/runtime-patches/`).
Reconstruction of all thirteen affected files matches the installed sources
byte-for-byte (`port/run/P43/patch-verification.log`), and reverse
`git apply --check -R` passes.

Build and test:

```bash
python3 port/p41_build.py > port/run/P43/final-build.log 2>&1
cmake -S tools/saturn-recomp/runtime/tests -B port/run/P43/runtime-tests -DSATURN_NO_SDL=ON > port/run/P43/tests-configure.log 2>&1
cmake --build port/run/P43/runtime-tests -j4 > port/run/P43/tests-build.log 2>&1
port/run/P43/runtime-tests/runtime_tests > port/run/P43/tests.log 2>&1
```

Regression and progression runs:

```bash
for v in ${!STV_@}; do unset "$v"; done
export STV_BOOTMODE=0 STV_CFG798=40 STV_P41TRACE=1 STV_SCENETRACE=1
mkdir -p port/run/P43/final-combat port/run/P43/final-reference port/run/P43/final-nocoin port/run/P43/final-progression port/run/P43/final-stage-clear

# 1. P4.2 combat check (2100 VBlanks)
port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2100 \
  --stvinput '200:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2,1500:P1A:2,1550:P1A:2,1600:P1A:2,1650:P1A:2,1700:P1A:2,1750:P1A:2,1800:P1A:2,1850:P1A:2' \
  --shots 1320,1440,1560,1800,2000 --dump 1800 --out port/run/P43/final-combat \
  > port/run/P43/final-combat.log 2>&1

# 2. Paired paid reference (2100 VBlanks)
port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2100 \
  --stvinput '200:COIN,340:START1' \
  --shots 1320,1440,1560,1800,2000 --dump 1800 --out port/run/P43/final-reference \
  > port/run/P43/final-reference.log 2>&1

# 3. No-coin attract regression (1800 VBlanks)
port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 1800 \
  --stvinput '99999:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2' \
  --shots 1700 --out port/run/P43/final-nocoin \
  > port/run/P43/final-nocoin.log 2>&1

# 4. P4.2 progression check (4200 VBlanks)
python3 - <<'PY'
from pathlib import Path
s = ['200:COIN', '340:START1', '1350:P1RIGHT:100']
s += [f'{v}:P1A:2' for v in range(1450, 5900, 12)]
s += [f'{v}:P1B:2' for v in range(1456, 5900, 24)]
Path('port/run/P43/progression-input.txt').write_text(','.join(s) + '\n')

s2 = ['200:COIN', '340:START1', '1350:P1RIGHT:100', '6090:P1RIGHT:30']
s2 += [f'{v}:P1A:2' for v in range(1450, 7500, 12) if not (6070 <= v <= 6125)]
s2 += [f'{v}:P1B:2' for v in range(1456, 7500, 24) if not (6070 <= v <= 6125)]
Path('port/run/P43/stage-clear-input.txt').write_text(','.join(s2) + '\n')
PY

port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 4200 \
  --stvinput "$(cat port/run/P43/progression-input.txt)" \
  --shots 3734,3862,3944,3970,4003,4200 --dump 3862,4003,4200 \
  --out port/run/P43/final-progression > port/run/P43/final-progression.log 2>&1

# 5. Extended second-area clear -> Scene 6 QTE -> Scene 8 -> Scene 9 progression (7500 VBlanks)
port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 7500 \
  --stvinput "$(cat port/run/P43/stage-clear-input.txt)" \
  --shots 5800,6200,6400,6600,6800,7000,7400 \
  --out port/run/P43/final-stage-clear > port/run/P43/final-stage-clear.log 2>&1
```

## PNG evidence and regression results

| Evidence | Repository-relative PNG path |
|---|---|
| Scene 3 initial frame (stale intro helicopter in `y = 0..99` gone) | `port/run/P43/final-combat/shot-1320.png` |
| Scene 3 combat after movement / attacks | `port/run/P43/final-combat/shot-1440.png`, `shot-1560.png`, `shot-1800.png`, `shot-2000.png` |
| Paired paid reference | `port/run/P43/final-reference/shot-1320.png`, `shot-1800.png` |
| No-coin attract title / zero credits | `port/run/P43/final-nocoin/shot-1700.png` |
| Scene 3 cleared | `port/run/P43/final-progression/shot-3862.png` |
| Scene 4 helicopter camera cut (stale rooftop wall/helicopter in `y = 0..99` gone) | `port/run/P43/final-progression/shot-3970.png` |
| Scene 5 entry and Wave 1 combat (stale door/blade smears in `y = 0..99` gone) | `port/run/P43/final-progression/shot-4003.png`, `shot-4200.png` |
| Scene 5 Wave 2 combat (P1 vs final Scene 5 enemy) | `port/run/P43/final-stage-clear/shot-5800.png` |
| Scene 5 cleared (final enemy defeated at VB 6162) | `port/run/P43/final-stage-clear/shot-6200.png` |
| Scene 6 hallway QTE (`"CAUTION"`, `"パンチ"`, `"SUCCESS"`) | `port/run/P43/final-stage-clear/shot-6400.png`, `shot-6600.png`, `shot-6800.png` |
| Scene 8 corridor transition cutscene | `port/run/P43/final-stage-clear/shot-7000.png` |
| Scene 9 third combat area (3 live enemies, active combat) | `port/run/P43/final-stage-clear/shot-7400.png` |

- **Runtime unit tests**: **PASS**, `126 tests, 0 failed` (`port/run/P43/tests.log`),
  including the new `vbe_erases_display_buffer_during_vblank_in_manual_mode` test.
- **P4.2 combat & reference regression**: **PASS**, 2100 VBlanks, zero `FATAL`s;
  all `16,810` `P41`/`SCENETRACE` records in both `port/run/P43/final-combat.log`
  and `port/run/P43/final-reference.log` are 100% identical to P4.2
  (`port/run/P43/verification.json`).
- **No-coin attract regression**: **PASS**, 1800 VBlanks, zero `FATAL`s; all
  `14,404` trace records in `port/run/P43/final-nocoin.log` are 100% identical to
  P4.2 (`scene=1`, `join=0/0`, all six actor slots empty).
- **P4.2 4200-VBlank progression regression**: **PASS**, 4200 VBlanks, zero
  `FATAL`s; all `33,617` trace records in `port/run/P43/final-progression.log`
  are 100% identical to P4.2, with `shot-3970.png` and `shot-4200.png` now free
  of stale VDP1 geometry across `y = 0..99`.
- **P4.3 7500-VBlank second-area clear progression**: **PASS**, 7500 VBlanks,
  zero `FATAL`s (`port/run/P43/final-stage-clear.log`, `60,027` trace records);
  clears Scene 5 at VB 6162, completes Scene 6 QTE (`scene 5 -> 6` at VB 6368,
  HP `2 -> 12` at VB 6691), transitions through Scene 8 (`VB 6945`), and enters
  Scene 9 (`VB 7020`) with live 3-enemy combat (`slot=4` HP `113 -> 93` at VB 7333).

## What is next

1. **RBG0 rotation screen-over / background fills in interior rooms**: In Scene 5
   (`shot-4200.png`, `shot-6200.png`), Scene 6 (`shot-6600.png`, `shot-6800.png`),
   and Scene 9 (`shot-7400.png`), the RBG0 floor map intentionally populates only
   the active floor footprint with nonzero pattern names, leaving surrounding
   tiles transparent so VDP1 3D wall/floor polygons or the VDP2 back screen show
   through. Verify whether any interior room uses RBG0 screen-over mode 1
   (over-pattern name `RBG_OVPNRA`, currently noted as unimplemented in
   `vdp2.cpp`) or NBG/back-screen clipping where black/white floor margins appear
   before VDP1 wall polygons cover them.
2. **Further stage progression past Scene 9**: Extend scripted or interactive
   play through Scene 9's multi-enemy waves (`timeline=060614F0..0606152C`) and
   subsequent elevator/interior scenes to test boss encounters, weapon pickups,
   and continue/game-over flows.
