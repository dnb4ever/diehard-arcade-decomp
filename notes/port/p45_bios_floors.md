# P4.5: real BIOS settings, the single-cart game loop, and RBG0 floor coverage

2026-11-06. Tested in this workspace on the build host. No commit made.
Canonical route: **no `STV_*` variables at all** (see section 8). No game variable,
actor, scene, credit or VDP register was forced. MAME 0.285 running the real US BIOS
(`epr-17952a`) was used as ground truth throughout.

Summary:

- **GAME OVER -> black and attract stuck on the title (defects 1, 2): one root cause.**
  The harness ran the multi-cart configuration (`STV_CFG798=40`: BIOS settings bit5
  clear) and entered at boot reason 0, the BIOS's "this game has just been selected"
  entry. On that path the BIOS owns the attract rotation and the game-over return
  (the cart calls `*0x0600060C` and waits). A real single-cart cabinet with factory
  settings has `0x06000798 = 0x000808FD` (bit5 set) and is entered at boot reason 1.
  There the cart runs its own attract (FBI screen -> intro -> title -> three demo
  stages -> loop). At game over it goes back to state 1 itself and never calls
  `0x0600060C`. MAME confirms this: across a game over, the boot reason
  (`0x06000656 = 1`), the boot mode (`0x060987CC = 1`) and the BIOS mode word
  (`0x06000824 = 0x20D`) do not change. Fix: seed the BIOS words from MAME and the
  BIOS disassembly (`port/main_stv.cpp`).
- **The harness's BIOS input and credit model was wrong in ways that the old route
  hid.** It forced `0x06098798 |= 0x34` every frame, which reports TEST, START1 and
  START2 as pressed. With TEST pressed the director preamble calls `0x0600060C` every
  frame. A coin also set the cart's START latch. Fix: publish the BIOS system-switch
  record `0x06000728`, model BIOS credits, and use the real 0x660/0x66C service bodies
  (`p45-bios-single-cart-input.patch`). Now, as in MAME, a coin adds a credit and the
  game starts only on START.
- **Interior floors (defect 3): two renderer bugs and a register decode bug**
  (`p45-rbg-param-b-hires.patch`):
  - RBG0 runs in rotation-parameter mode 2. Where parameter A's coefficient MSB is set,
    the hardware draws parameter B (the ceiling plane). The runtime drew nothing there.
  - In 704-dot high resolution, a display dot is half a rotation-screen dot. The runtime
    stepped a full dot, which sent the right and lower part of the screen off the
    populated floor and left it black.
  - KTCTL/KTAOF parameter-B fields were read from the wrong bits.

  Side by side with MAME, the lobby and the glass pavilion now match: ceiling present
  and floor edge to edge. Scenes 5, 9, 21, 25 and 32 have no undrawn regions.
- **Defect 4 (white top band, Scene 5) is authored.** MAME shows the same white back
  wall behind the fans (`port/run/P45/scene5_cmp.png`).
- **Defect 5 (Japanese `パンチ` prompt) is fixed** by the real area code
  (`0x06000248 = 0x4008`, USA). The QTE reads `PUSH ... KICK`, and the US-only FBI
  screen and English intro subtitles appear.
- **Four more data-reached entries** were seeded (`port/seeds_extra.txt`, P4.5 block).
  The USA path first hit `FATAL call to 06026EE2`; the second game loop hit `0601FC1C`.
- **Stretch: the game was played to the end.** With `STV_AUTOPLAY=100` from power-on and
  79 continues, P1 cleared the final rooftop/helicopter boss in Scene 72 (VB ~158031).
  The ending scenes 73 -> 79 -> 80 -> 81 followed, the game went back to the attract at
  VB 159214, and the scripted player started a second game. That second game stopped at
  VB 165861 (Scene 8) with `FATAL call to 0601FC1C`, one more data-reached task step.
  It is now seeded, but that is not re-verified by a run (section 7).
- Runtime tests: **128 tests, 0 failed** (two new). Regressions: section 9.

## 1. Ground truth used

- **BIOS disassembly.** `port/run/P45/bios_dis.txt` is `sh-elf-objdump` of the
  word-swapped `epr-17952a` (`~/decomp/bios_spike/bios_be.bin`, not in the repo). The boot
  code at ROM `0x29A` fills the `0x06000200+` vector slots from the offset table at
  `0x530` and the entries at `0x564`. The boot copies at `0x270..0x2A0` place ROM
  `0x0C00..0x0CFF` at `0x06000600`, ROM `0x30000..0x323FF` at `0x06000C00` (the per-frame
  services) and ROM `0x8000..` at `0x06010000` (the BIOS menu program). Cart vectors:

  | slot | target | what it does |
  |---|---|---|
  | `0x0600060C` | ROM `0x413A` | exit to the BIOS: resets SP to `0x06100000`, sets the `0x06000824` mode bits, switches on the argument (-3..2: next cart, menu, test, reboot same cart) and jumps into the BIOS main loop `0x32CE`. That loop reloads a cart (`0x4704`) and boots it through the cart's own table at `0x06004F10` (entry `0x06005000 + 4*boot reason`). Never returns. |
  | `0x06000610` | RAM `0x06000D14` (ROM `0x30114`) | per-frame service: port scan (`0x31498`), coins and credits, and, only when bit5 of `0x06000798` is clear, the multi-cart START/countdown service (`0x30222`) |
  | `0x06000660` | ROM `0x308E0` | copy the 20-byte panel report `0x06000740` to the caller |
  | `0x06000664` | ROM `0x30AB0` | take one credit if the start code is 2 |
  | `0x0600066C` | ROM `0x305C8` | copy 4 bytes from the caller to `0x06000828` |
  | `0x06000680` | ROM `0x3060E` | arg 0: clear mode bit 0x20; arg 1: unpause the BIOS service (`0x06000820 = 0`) |

  System-switch record `0x06000728` (ROM `0x31764`): `~PORTC` packed as COIN1 `0x01`,
  COIN2 `0x02`, TEST `0x04`, SERVICE `0x08`, START1 `0x10`, START2 `0x20`, PAUSE `0x4000`.
  The cart's `func_0220f9d4` turns it into the edge word `0x06098798`. The director
  preamble (`022016ac.c`) calls `0x0600060C` on a TEST edge (`0x06098798 & 4`).
- **MAME 0.285** (Ubuntu package extracted without root into `port/run/P45/mamepkg`, not
  committed) with `diehard.zip`, run headless with Lua probes `port/run/P45/mame/probe2.lua`
  (memory trace and snapshots), `dump.lua` (work-RAM dump) and `play.lua` (a scripted
  reference player; in MAME only, P1 HP is pinned so it walks the stage order). Measured at
  frame 600, factory NVRAM:

  | word | MAME | old harness | P4.5 |
  |---|---|---|---|
  | `0x06000798` settings | `0x000808FD` | `0x00000040` (STV_CFG798) | `0x000808FD` |
  | `0x06000248` area code | `0x00004008` (USA) | vector placeholder (nibble 1 = Japan) | `0x00004008` |
  | `0x060987CC` boot mode | 1 | 0 (STV_BOOTMODE) | 1 (default) |
  | `0x06000656` boot reason | 1 | sentinel | 1 |
  | `0x06000658` slot nibbles | `0xFFF0` | sentinel | `0xFFF0` |
  | `0x0600065A/B` output shadows | 0 | sentinel bytes | 0 |

  The `0x06000600` vector values in the MAME dump match the disassembly mapping above
  (`0x60C = 0x413A`, `0x660 = 0x060014E0`, ...).
- Note: `project/src_functional/02201590.c` inverts the bit5 test. The asm at
  `0x060055F2..0x06005612` sets `0x060987D0 = 1` and calls `0x06000680(1)` when bit5 is
  **clear**. Not edited (outside scope).

## 2. Defects 1 and 2: root cause and fix

Old route: boot reason 0 -> `func_022016ac` state 3 -> state 4 (the credit/join scene
"SELECT GAME WITH THE SELECT BUTTON", then play). With bit5 clear, the end of state 4
calls `*0x0600060C(0)` and parks in the undefined state 13. A real BIOS would now run
its multi-cart main loop and reboot a cart. The harness had no BIOS, so the screen
stayed black. The title never advanced because on this path the join countdown
(`0x06000654`) belongs to the BIOS multi-cart service.

Fix (`port/main_stv.cpp`, not a runtime patch): default `0x06000798 = 0x000808FD`, area
code `0x00004008`, and the BIOS variables at `0x06000650..0x0600065B` (they sat inside
the vector-sentinel window). The power-on boot reason 1 was already the default; the
old repros overrode it. With these values the cart takes its single-cart path:
`0x060987D0 = 0` (the cart's own START latch is live), and state 4 -> state 1 after a
game over. `0x0600060C` itself is not emulated: the BIOS menus and rotation are not
needed on a single-cart cabinet. The call is now logged (`ST-V BIOS: exit to BIOS ...`),
and it occurs 0 times in every P4.5 run.

Timing against MAME (no coin; then coin + START; frames are MAME, VBlanks are P4.5;
MAME's BIOS boot takes about 505 frames longer):

| event | MAME | P4.5 | difference |
|---|---:|---:|---:|
| attract scene 1 -> 2 | 520 -> 824 | 11 -> 316 | 304 / 305 |
| scene 3 / 4 / 6 / 7 / 8 / 9 / 10 | 2660/3160/4978/6814/7314/8554/9054 | 2154/2655/4476/6314/6815/8056/8557 | 505..498 |
| attract loops to scene 1 | 9367 | 8872 | 495 |
| START -> playing (`0x06098674 = 2`) | +21 | +20 | |
| START -> stage scene 3 | +827 | +826 | |
| CONTINUE? -> GAME OVER | 1278 | 1278 | |
| game over -> scene 1 | 299 | 302 | |

Evidence:
- Attract cycle, title -> demo -> title (`port/run/P45/final-nocoin`):
  `port/run/P45/a1_montage.png` (one PNG every 300 VBlanks). It shows the FBI screen,
  the intro with English subtitles, the title, demo 1 (lobby, `PUSH`/`KICK` QTE),
  demo 2 (glass pavilion), demo 3 (`CAUTION` ladders), the boss cut scene and the FBI
  screen again at VB 9000. The MAME reference is `port/run/P45/mame/mameB_montage.png`.
- Game over -> attract -> new game (`port/run/P45/final-gameover`):
  `port/run/P45/go1_montage.png`. It shows coin at 600 and START at 700, the
  `CONTINUE? 9 .. 0` countdown, `GAME OVER` (VB 5000), the FBI screen (VB 5300), the
  intro with `PRESS START BUTTON / CREDIT 1` after the second coin (VB 6050), and a
  fresh game from VB 6100 (helicopter intro, then stage 1 fight and continue).

## 3. The BIOS input and credit model (`p45-bios-single-cart-input.patch`)

These old harness behaviours were not faithful. With a real `0x0600060C` / TEST path
and the single-cart START latch they became visible:

1. `stv_system_bios_tick` and `stv_panel_read` forced `0x06098798 |= 0x34` every
   frame, as if TEST + START1 + START2 were always newly pressed. Removed. The BIOS
   system-switch record `0x06000728` (+2 previous, +4 press, +6 release) is now
   published every VBlank from the `--stvinput` events (COIN1/2, START1/2, SERVICE).
2. `deliver_coin` set the cart's START latch `0x0609880C` (P1) directly when a coin
   arrived, with a credit-state gate. On hardware the BIOS counts the coin, and the cart
   sets the latch itself on a START edge after `0x06000664` grants a credit. Now a coin
   only adds a BIOS credit (common chute, 1 coin = 1 credit, accepted at any time), and
   `0x06000664` grants and takes one. The old scripted-window fallback is gone.
3. The `0x06000660` report now uses the BIOS layout ([0] credits, [2] 1, [3]/[7]
   start code 2 when a credit exists, [0x10] `0x08`, [0x11] `0x11`) and copies **20
   bytes**. The old 32-byte clear overwrote the cart's config bytes `0x060987EC..F7`.
4. `0x0600066C` was treated as a second panel read, which overwrote `0x060987D4..` (the
   cart's per-player in-game bytes). It now copies 4 bytes to `0x06000828`, as ROM
   `0x305C8` does.
5. `STV_AUTOPLAY` (test harness, default off) also inserts COIN + START when P1 has no
   game (`0x06098674 == 0`). That gives a fully scripted run from power-on.

## 4. Missing recompiled entries (`port/seeds_extra.txt`, P4.5 block; see also section 7)

The USA attract stopped at VB ~13: `FATAL call to 06026EE2, not an entry (pr
06027D24)`. That is a draw callback that `02222d3c` installs into `g_06090298` through a
PIC pointer. A scan for prologues directly after `rts`/`braf`/`jmp` that are not table
entries found 4 candidates. 3 are real code (`06014298`, `06026EE2`, `0604E268`) and
were seeded; `0605382C` is data. Rebuilt with `python3 port/p41_build.py`.

## 5. Defect 3: interior floor coverage (`p45-rbg-param-b-hires.patch`)

Registers in every interior room (`port/run/P45/vdp2regs.py` on the dumps):
`TVMD = 0x8013` (704 dots), `RPMD = 2`, `KTCTL = 0x1111`, `KTAOF = 0x0101`,
`PLSZ = 0x884x` (A and B both 1x1-page planes, screen-over 2). The pixel classifier
`port/run/P45/rbgprobe.py` (a Python copy of the rotation maths) shows, at VB 3095 of
the lobby demo, that every line above ~70 has parameter A's coefficient MSB set. Those
dots belong to parameter B.

1. **RPMD = 2 was rendered as "A, MSB -> transparent".** MAME (`saturn.cpp`, RPMD case
   2) draws B, then A over it. Fix: `Rbg::alt` holds a parameter-B screen with B's map
   registers (`0x60..0x6E`), RBPLSZ, RBOVR, RBMPOF and B's coefficient table. A dot whose
   A coefficient has the MSB set is drawn from it. This restores the ceilings.
2. **High resolution.** With HRESO bit1 set, the rotation screen advances half a dot per
   display dot, and for LSMD 3 half a line per display line (MAME `hcnt_shift` and
   `vcnt_shift`). The runtime used full steps over 704 dots. That sampled the floor map
   twice as far out on the right and bottom, past the populated tiles, so those areas
   were black. Fix: `Rbg::xscale/yscale` applied in `rbg_pixel`.
3. **KTCTL/KTAOF decode.** Parameter B's fields are at bits 8-12 and 8-10, not 4-7.
   KDBS is bit 1 and KMD is bits 2-3. A 1-word table base steps in `0x20000`-byte banks.
   Coefficient mode 3 replaces Xp. RPMD 1 now also uses B's planes.

New unit tests in `runtime/tests/vdp2_test.cpp`:
`rotation_mode_2_draws_parameter_b_where_the_coefficient_msb_is_set` and
`rotation_screen_steps_half_a_dot_in_high_resolution`. Both fail on the old `vdp2.cpp`
and pass on the new one.

Evidence (left: old renderer, same route; middle: P4.5; right: MAME at the same scene time):
- Lobby demo: `port/run/P45/tri1.png` (VB 2795..3545) and `port/run/P45/tri2.png`.
- Glass pavilion demo: `port/run/P45/tri3.png` (VB 6850..7600).
- Gameplay, P4.4 against P4.5: Scene 9 and Scene 21 in `port/run/P45/floors_s9_s21.png`;
  Scene 25 and Scene 32 in `port/run/P45/floors_s25_s32.png` (P4.5 frames from
  `port/run/P45/auto2/shot-27000.png`, `shot-33000.png`, `shot-42000.png`,
  `shot-45000.png`, `shot-49000.png`, `shot-53000.png`).
- Scene 5 against MAME, white back wall authored: `port/run/P45/scene5_cmp.png`.

Remaining renderer gaps (not seen to matter here): RPMD 3 (window switching),
screen-over mode 1, and line colour carried in coefficient data (LCAE, set in
`KTCTL = 0x1111` but only for the line-colour screen, which this game leaves off,
`LNCLEN = 0`).

## 6. Defect 5

With area code USA the cart shows the FBI "Winners don't use drugs" screen, English intro
subtitles, and `PUSH ... KICK` QTE prompts (`port/run/P45/a1/shot-4200.png`). The old
placeholder area word decoded as nibble 1 (Japan).

## 7. Stretch: the furthest point (`port/run/P45/auto`)

`STV_AUTOPLAY=100 --stvinput 99999:COIN --vblanks 170000` from power-on, scripted player
only, with continues bought by COIN + START. 0 FATAL. Scene first-VBlanks:

| Scene | VB | | Scene | VB | | Scene | VB | | Scene | VB |
|---:|---:|---|---:|---:|---|---:|---:|---|---:|---:|
| 3 | 698 | | 21 | 25353 | | 38 | 77731 | | 62 | 103177 |
| 4 | 1897 | | 22 | 39596 | | 44 | 78141 | | 63 | 116094 |
| 5 | 1954 | | 24 | 40045 | | 45 | 86106 | | 64 | 116613 |
| 6 | 6527 | | 25 | 40102 | | 47 | 86789 | | 65 | 121102 |
| 8 | 7104 | | 26..31 | 46947..47242 | | 49 | 91495 | | 66 | 121529 |
| 9 | 7177 | | 32 | 47293 | | 51 | 91722 | | 67 | 134030 |
| 10..15 | 13070..13366 | | 33 | 57508 | | 52..56 | 101333..101671 | | 69..72 | 134125..148778 |
| 16 | 16863 | | 34 | 57565 | | 58 | 102924 | | 73 (ending) | 158223 |
| 17 | 17476 | | 35 | 66898 | | 60 | 103021 | | 79 / 80 / 81 | 158282 / 158315 / 158916 |
| 18 / 20 | 24799 / 25294 | | 36 / 37 | 67311 / 67370 | | 61 | 103102 | | attract (2) | 159214 |

Scenes 7, 19, 23, 46, 57, 59, 68 and 74..78 were not entered on this route (QTE or
alternate branches). The boss arm flag `0x06098670` dropped to 0 at VB 158031 in
Scene 72 (the rooftop fight against the helicopter, `port/run/P45/auto/shot-152000.png`,
`shot-156000.png`, `shot-158000.png`), and Scene 73 started the helicopter ending
(`shot-158500.png`). Caveat: the scripted player pressed COIN + START during the ending
(P1 state 0 counts as "no game"), so parts of the ending were probably skipped. The
credits sequence was not checked frame by frame (`port/run/P45/auto_end.png`).
The run then returned to the attract (`shot-159500.png`, `PRESS START BUTTON / CREDITS 4`)
and started a new game. In that game, at VB 165861 (Scene 8),
`FATAL call to 0601FC1C, not an entry (from pr 06013E0E)` stopped the run. This is a task
step after an `rts` that is referenced by pointer, the same class as P4.4's `060359B2`.
It is seeded (`port/seeds_extra.txt`, last P4.5 entry). A scan of all pointer literals
that point just past an `rts`/`braf`/`jmp` in code found no other real entry; the other
hits are mid-function labels or data. Re-running the 166000-VBlank route to confirm was
not done.

The renderer in this run predates the high-resolution fix (it has the RPMD fix). Its
logic is identical to `port/run/P45/auto2` (the final binary, same timeline to Scene 34
at VB 57565). For the final-renderer pictures use `auto2`.

## 8. Reproduce from the repository root

Runtime patch order: the P4.4 chain (`port/runtime-patches/README.md`), then
`p45-bios-single-cart-input.patch`, then `p45-rbg-param-b-hires.patch` (`patch -p1` in
`tools/saturn-recomp`). Reconstruction from the P4.4 files matches the installed
sources (`port/run/P45/patch-verification.log`).

```bash
python3 port/p41_build.py > port/run/P45/p41_build.log 2>&1      # seeds + runtime
cmake -S tools/saturn-recomp/runtime/tests -B port/run/P45/runtime-tests -DSATURN_NO_SDL=ON
cmake --build port/run/P45/runtime-tests -j8
port/run/P45/runtime-tests/runtime_tests > port/run/P45/tests.log 2>&1   # 128 tests, 0 failed

. port/run/P45/env.sh          # unsets every STV_*, STV_SCENETRACE=1, BIN=P41 build
port/run/P45/final_runs.sh     # final-nocoin, final-gameover, final-combat, final-legacy-combat

# furthest progression (scripted player from power-on)
STV_AUTOPLAY=100 STV_P41TRACE=1 $BIN --cart port/diehard_main.bin --cartrom cart.bin \
  --vblanks 170000 --stvinput 99999:COIN --shots $(seq -s, 500 500 170000) \
  --out port/run/P45/auto > port/run/P45/auto.log 2>&1

# MAME reference (binary extracted from the Ubuntu mame .deb into port/run/P45/mamepkg)
cd port/run/P45/mame && LD_LIBRARY_PATH=$PWD/../mamepkg/x/usr/lib/x86_64-linux-gnu \
  TAG=B COIN_F=1800 START_F=1900 ../mamepkg/x/usr/games/mame diehard -rompath roms \
  -snapshot_directory snapB -nvram_directory nvramB -video none -sound none -nothrottle \
  -seconds_to_run 600 -autoboot_script probe2.lua
```

## 9. Regression results

| Check | Result |
|---|---|
| Runtime unit tests | **PASS** `128 tests, 0 failed` (126 old + 2 new) |
| No-coin attract, 9000 VB (`final-nocoin`) | **PASS, changed on purpose.** Scenes 1 -> 10 -> 1 at VB 8872, 0 FATAL, 0 BIOS exits. P4.4 sat on scene 1. |
| Game over -> attract -> new game (`final-gameover`) | **PASS.** GAME OVER at 4893, scene 1 at 5195, second coin + START at 6000/6100, playing at 6120, stage at 6926 |
| P4.2 combat input, 2100 VB (`final-combat`) | **PASS.** Playing at VB 360, stage scene 3 at 1166 (P4.4: 1179), P1 fights the first group (`port/run/P45/combat_cmp.png`) |
| Legacy multi-cart route `STV_BOOTMODE=0 STV_CFG798=40`, same input | **PASS.** Scene 3 at 1179, as in P4.4 |
| Progression to Scene 9 / Scene 21 / Scene 51 / ending | **PASS** with the scripted player: Scene 9 at 7177, 21 at 25353, 51 at 91722, ending at 158223 (P4.4: 7020 / 36345 / 82937 / not reached) |

The P4.3 hand timeline (`port/run/P43/stage-clear-input.txt`) no longer clears Scene 3
on the new route: P1 dies at VB 3429 and the game returns to the attract
(`port/run/P45/reg-progression.log`). Two things changed. The config bytes at
`0x060987EC` are no longer zeroed every frame, and the area is now USA. A fixed button
timeline is fragile against either. The scripted player covers progression instead.
Byte-identical comparison with P4.4 is not possible by design: the boot path, the
attract and the input model all changed.

Why the no-coin output differs: P4.4 booted the post-START multi-cart entry, which shows
the "SELECT GAME" join screen and waits for a BIOS countdown that only a multi-cart BIOS
runs. P4.5 boots like a real single-cart cabinet, so with no coin you see the full
attract loop, as MAME shows.

## 10. Defect list after P4.5

1. `0x0600060C` (BIOS menus, test mode, multi-cart rotation) is not emulated. A single
   cart reaches it only via TEST. Emulating it would need the RAM-relocated BIOS program
   (`real_bios_spike.md`).
2. The multi-cart route (`STV_CFG798=40`) is still half-emulated: no BIOS START/countdown
   service (`0x30222`). It is kept only as a legacy repro.
3. `0x06000644` (backup-RAM access, 17 cart call sites), `0x06000640` (backup-RAM counter,
   modelled as a frame timer) and `0x06000628..0x0600063C` are still no-ops. Rankings and
   bookkeeping may differ from hardware.
4. Renderer: RPMD 3, screen-over mode 1 and coefficient line colour are not done.
5. The MAME reference player (`play.lua`) stalls in Scene 5, so MAME pictures beyond
   Scene 5 come only from the attract demos.
6. Audio is still not auditioned. The SCSP DSP effect path is unused.

## 11. What is next

1. Re-run the full `STV_AUTOPLAY` route (about 170000 VBlanks, 46 minutes) to confirm
   the `0601FC1C` seed, and stop autoplay from pressing START during the ending (treat
   scenes 73..81 as "watching"), so the credits can be checked against MAME.
2. Improve the MAME reference player (weapon pickup, QTE input), so that late-stage
   floors and effects get a frame-by-frame MAME comparison.
3. Replace the hand timelines in older notes with `STV_AUTOPLAY` repros on the faithful
   route, and retire `STV_BOOTMODE` / `STV_CFG798` from all docs.
4. Model `0x06000644` backup RAM (high-score table, bookkeeping) from the BIOS
   disassembly, checked against MAME's NVRAM.
5. Optionally fix `project/src_functional/02201590.c` (inverted bit5 test).
