# P4.2 — faithful floor, palette DMA and first stage progression

2026-10-31, Codex. Tested in this workspace on the build host. No commit made.
All runs use `STV_BOOTMODE=0 STV_CFG798=40`; paid runs use physical coin/START input.
No actor, HP, scene, progress, descriptor or director variable was forced.

The rooftop floor now renders without `STV_RBG_PLANE`. Two runtime bugs were
fixed: incorrect RBG map-register offsets, and truncated SH-2 block DMA. The
first enemy pair can be defeated with scripted buttons; the game advances from
scene 3 to scene 4 and then scene 5. The top-right coin prompt is the original
cart's invitation for unjoined P2, so it is retained.

## Root cause 1: the renderer read the wrong plane registers

There was no missing BIOS stage loader and no requirement to populate all sixteen
planes with floor tiles. The game's real map intentionally points most planes
at the cleared page `0x5C000`. The visible floor is plane **M**, page **0x28**, at
VDP2 VRAM **0x50000**. It was already loaded before either P4.2 fix.

`runtime/src/video/vdp2.cpp` read parameter A's map from `0x48..0x56` instead of
`0x50..0x5E`, and parameter B's from `0x58..0x66` instead of `0x60..0x6E`.
Consequently A..D came from NBG2/NBG3 registers, and the last four real map
registers were omitted. In particular, slot M read `MPEFRA=0x2E2E` instead of
`MPMNRA=0x2E28`, sampling the empty page instead of the floor.
The register layout is independently recorded in
[Yabause's VDP2 register structure](https://github.com/Yabause/yabause/blob/master/yabause/src/vdp2.h).

Concrete pre-fix evidence, `port/run/P42/baseline/dump-1800.bin`:

| VDP2 offset | Value | Meaning |
|---|---|---|
| `0x48`, `0x4A` | `0000`, `0000` | NBG2 map, wrongly used as RBG A..D |
| `0x4C`, `0x4E` | `3838`, `3838` | NBG3 map, wrongly used as RBG E..H |
| `0x50..0x5A` | `2E2E` in every word | Real RBG A..L, deliberately blank |
| `0x5C` | `2E28` | Real RBG M=floor, N=blank |
| `0x5E` | `2E2E` | Real RBG O/P, blank |

The `0x50000` page contains 8064 nonzero bytes out of 8192; `0x5C000` is all zero.
The old claim that the full plane set had failed to load was based on reading
the wrong registers. The `0x70000` page used by earlier forced demos belongs to
a different map on this route. It is not the faithful rooftop floor selection.
The existing KTAOF shift-18 fix and game-generated rotation/coefficient tables
remain in use. No camera or coefficient override was needed.

Fix: change the two register bases, `0x48 -> 0x50` and `0x58 -> 0x60`.
Patch: `port/runtime-patches/p42-rbg-map-registers.patch`.
It also corrects the legacy override comment and adds a pixel test independently
sampling all sixteen planes in both A and B register banks. RBG1's register-bank
correction is tested synthetically; live RBG1 gameplay is not claimed.

## Root cause 2: SH-2 16-byte DMA divided the count twice

`runtime/src/machine/onchip.cpp::dma` already computes `n=tcr/4` for 16-byte
block transfers, because TCR counts longwords in this mode. The loop additionally
executed `i += 3`, effectively advancing four blocks after copying only one.
It then falsely published TCR=0 and TE=1, hiding the truncated transfer.

The faithful route actually exercises this: `func_02214678` programs
CHCR=`0x5E11`, TCR=126, destination `0x25F00008`, after its leading CPU copies.
The old runtime copies 8 blocks / 128 bytes; the corrected one copies 31 blocks /
496 bytes. The low two TCR bits are ignored in block mode, as also implemented by
[MAME's SH7604 DMA](https://github.com/mamedev/mame/blob/master/src/devices/cpu/sh/sh7604.cpp).
`port/run/P42/dma-trace.log` records this transfer during boot and both scene
setups. Its existing diagnostic format prints raw TCR as “126 x 16”; that label
is not the actual byte count.

Fix: remove the extra `i += 3`; the loop's normal increment now advances one
block. Patch: `port/runtime-patches/p42-dmac-block-count.patch`.
It includes an SH-2 DMAC test covering TCR 4, 8, 28, 64 and 126, untouched memory
past the transfer, SAR/DAR, TCR and TE.

The matched-input VB1800 dumps before and after both fixes have identical VDP1
VRAM, draw framebuffer, VDP1 registers, VDP2 VRAM and VDP2 registers. Exactly
**339 CRAM bytes** differ, within offsets `0x088..0x1F7`. Thus the newly visible
wall textures are a **palette DMA repair**, not a newly forced texture/stage
upload. The floor-map repair changes only how the renderer reads existing state.

## INSERT COIN(S): original unjoined-player behavior

Checked the original game logic, not just an old runtime screenshot:
`project/src_functional/02204868.c` reproduces the cart's attract/executive
handler. Its no-start path handles P1 and P2 separately. With P1 join=2 and
P2 join=0, it skips P1's prompt and executes P2's prompt branch. At HUD cursor
`(23,1)`, every 16 ticks it alternates the glyph selected through
`0x06090304[*(u8*)0x060987DF]` with the blank string at `0x06050D84`.
That is exactly the top-right location and blinking seen in these PNGs.
The active executive is `06008868`; its presence alone does not mean the paid
player is stuck in a noninteractive demo (combat and stage advancement below
prove otherwise). `02204afa.c` likewise preserves a separate unjoined P2 prompt.

Evidence: `port/run/P42/final-combat/shot-1320.png` has the prompt blank;
`shot-1800.png` has it visible, with P1 playing and P2 absent in the trace.
This is not an old framebuffer label left behind after P1 joined. Removing it
would change authored one-player behavior, so no prompt suppression was added.
This conclusion is based on the original cart code and its observed execution;
no new original-board video capture was available in this session.

## Retired bridges

None of these names remained in `port/main_stv.cpp`; the implementations were
in `tools/saturn-recomp/runtime/src/machine/bios.cpp`. All three implementations,
related parsing and the `e_liveround` early-return dependency were removed in
`port/runtime-patches/p42-remove-retired-bridges.patch`.

| Removed knob | What it did |
|---|---|
| `STV_LIVEROUND` | Set internal P1/P2 start-edge flags to force executive morphs; held the arm flag, scene/descriptor and director timeout to sustain a hybrid round. |
| `STV_SYNTHDEF` | Replaced a held attract descriptor's `+0x10` definition pointer with another scene's play definition, repeatedly. |
| `STV_STAGEDMA` | Built a scratch task, copied another descriptor's stage fields into it and the held descriptor, then called `scene81_stage_setup` out of band. |

They are unnecessary for the verified floor, player control, damage and first
stage progression. The 4200-VBlank actor/global/gate trace before versus after
removal is identical (33,600 trace records). Other pre-existing diagnostics and
bridges were not broadly removed; `STV_RBG_PLANE` remains a labelled diagnostic
but is never enabled by the P4.2 repros.

## Progression and the first actual wait

Script: coin VB200, START1 VB340, RIGHT VB1350 for 100 VBlanks, A every 12
VBlanks from VB1450, B every 24 from VB1456, each held two VBlanks. Inputs feed
only the existing P4.1 BIOS peripheral words; the cart handles combat.

| VB | Observation |
|---:|---|
| 2064 | First E3 reaches HP0; its slot clears at 2184. |
| 3734 | First E4 reaches HP0; P1 still has HP36. |
| 3862 | E4 slot clears; first pair is fully removed. |
| 3944 | Director advances scene 3 -> 4. |
| 4003 | Director advances scene 4 -> 5. |
| 4005 | Next combat group spawns, E3/E4 HP85, P1 HP36. |
| 6000 | Extended run remains in scene 5; another E3 is alive at HP82, P1 HP2. |

The first wait is ordinary enemy-wave completion, not missing emulation:
`022463b8.c` gates the next script record on the accumulated live-wave width
(`060973E0 <= 060973E8` and width+next-width <=40). Enemy teardown drains that
width (`0223e9d2.c` / `02249da8.c`). `022465fc.c` waits for active enemy count
`*(s32*)0x0609958C == 0`, then sets its parent content entity's `+0x3C` bit3.
`02232610.c` state 2 consumes that bit, performs its fade/delay, and ultimately
signals the director to advance. The trace's width falls 20 -> 10 -> 0 as the
first pair disappears. There is no forced completion flag or patched game code.

The final executable repeats the progression through VB4200 after bridge
removal. The earlier 6000-VBlank extension used the same two hardware fixes with
all three retired knobs unset; the entire shared trace prefix matches. No
emulation gate blocks the first clear/transition. The extension ends with a live
opponent, so it does not establish a later-stage emulation stall.

The stopping rule was not triggered: these two independently demonstrated
hardware fixes produced visible repairs and real stage progression, rather than
two successive deeper game-state gates. Work stops at the requested first-group
verification; no forcing was necessary.

## Reproduce from the repository root

Patch order against upstream `b22688d0bef52fba31cda79fd23150e8905cf3c2`:
`round-runtime-dh-portB.patch`, `p41-raw-panel-input.patch`,
`p41-player-trace.patch`, `p42-rbg-map-registers.patch`,
`p42-dmac-block-count.patch`, `p42-remove-retired-bridges.patch`.
Do not stack the older `saturn-recomp-stv.patch` on that baseline.
All paths above are under `port/runtime-patches/`. The P4.2 patches include every
new installed runtime/test change. Reconstruction of all twelve affected files
matches the installed sources byte-for-byte:
`port/run/P42/patch-verification.log`. Reverse `git apply --check` also passes.

Build and test:

```bash
python3 port/p41_build.py
cmake -S tools/saturn-recomp/runtime/tests -B port/run/P42/runtime-tests -DSATURN_NO_SDL=ON
cmake --build port/run/P42/runtime-tests -j4
port/run/P42/runtime-tests/runtime_tests
```

For the commands below, in Bash clear inherited ST-V overrides first:

```bash
for v in ${!STV_@}; do unset "$v"; done
export STV_BOOTMODE=0 STV_CFG798=40 STV_P41TRACE=1 STV_SCENETRACE=1
mkdir -p port/run/P42/final-combat port/run/P42/final-reference port/run/P42/final-nocoin

port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2100 \
  --stvinput '200:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2,1500:P1A:2,1550:P1A:2,1600:P1A:2,1650:P1A:2,1700:P1A:2,1750:P1A:2,1800:P1A:2,1850:P1A:2' \
  --shots 1320,1440,1560,1800,2000 --dump 1800 --out port/run/P42/final-combat \
  > port/run/P42/final-combat.log 2>&1
```

Paired reference: same command with `--stvinput '200:COIN,340:START1'`, output
`port/run/P42/final-reference`, log `port/run/P42/final-reference.log`.
No-coin: same command with
`--stvinput '99999:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2'`,
`--vblanks 1800 --shots 1700`, output `port/run/P42/final-nocoin`, log
`port/run/P42/final-nocoin.log`. The out-of-window coin suppresses the harness's
default automatic coin script.

Progression input and final bridge-free verification:

```bash
python3 - <<'PY'
from pathlib import Path
s = ['200:COIN', '340:START1', '1350:P1RIGHT:100']
s += [f'{v}:P1A:2' for v in range(1450, 5900, 12)]
s += [f'{v}:P1B:2' for v in range(1456, 5900, 24)]
Path('port/run/P42/progression-input.txt').write_text(','.join(s) + '\n')
PY
mkdir -p port/run/P42/final-progression
port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 4200 \
  --stvinput "$(cat port/run/P42/progression-input.txt)" \
  --shots 3734,3862,3944,3970,4003,4200 --dump 3862,4003 \
  --out port/run/P42/final-progression > port/run/P42/final-progression.log 2>&1
```

For the longer extension use `--vblanks 6000`,
`--shots 1320,1800,2400,3000,3600,4200,5000,5900`, output
`port/run/P42/progression` and log `port/run/P42/progression.log`.
The small pre-fix diagnostics `saturn_stv-before` and map-only diagnostic
`saturn_stv-map-only` are preserved under `port/run/P42/`; they are evidence
binaries, not the final build target.

## PNG evidence and regression results

Images were produced by `--shots`. The before/after floor, no-coin title,
first clear, transition and next-area frames were visually inspected.

| Evidence | Repository-relative PNG path |
|---|---|
| Original missing floor | `port/run/P42/baseline/shot-1800.png` |
| Map repair alone: floor visible, wall still black | `port/run/P42/map-only/shot-1320.png` |
| Both repairs: green floor and textured wall | `port/run/P42/final-combat/shot-1320.png` |
| Combat after movement / attacks | `port/run/P42/final-combat/shot-1440.png`, `shot-1560.png`, `shot-1800.png`, `shot-2000.png` |
| Paid reference | `port/run/P42/final-reference/shot-1320.png`, `shot-1800.png` |
| No-coin title / zero credits | `port/run/P42/final-nocoin/shot-1700.png` |
| First group cleared | `port/run/P42/final-progression/shot-3862.png` |
| Camera/scene transition | `port/run/P42/final-progression/shot-3970.png` |
| Next area / enemy group | `port/run/P42/final-progression/shot-4200.png` |

P4.1 combat regression **PASS**, 2100 VBlanks: P1 X changes from `0012966A`
(VB1320) to `00102D4C` (VB1440), while the paired reference stays at `0012966A`.
The same E3 task/entity goes HP64 ->54 at VB1462 ->48 at VB1508 ->42 at VB1858.
Every P41 actor/global/gate sample across the entire 2100-VBlank run is identical
to this session's pre-fix baseline. The reference and physical-input run also
match before input starts. Data: `port/run/P42/combat-comparison.csv`.

No-coin attract regression **PASS**, 1800 VBlanks: scene 0 ->1, stays in 1;
join=0/0 throughout; all six actor slots remain empty, despite START, direction
and attack events. Its PNG changes from P4.1 because the palette bug is repaired;
byte-identical preservation of broken colors is not claimed.

Runtime tests **PASS**, 125 tests, zero failures (`port/run/P42/tests.log`).
Against pre-P4.2 `vdp2.cpp`/`onchip.cpp`, exactly the two new hardware tests fail
(`port/run/P42/tests-before.log`); the other 123 pass. The fixed tests also cover
the live transfer's TCR=126 case. Builds: `final-build.log`, `tests-build.log`,
`tests-configure.log`, all under `port/run/P42/`. Run summaries and zero-FATAL
checks: `port/run/P42/verification.json`. All gameplay runs reached their stated
VBlank limits.

## Remaining work

The screenshots still contain stale VDP1 geometry, particularly during camera
cuts and in the next area. No `STV_VDP1ERASE` override was used or promoted:
manual frame-change/erase timing needs a separate hardware-backed diagnosis.
P4.2 establishes the floor and palette fixes, not complete video correctness.
Improve the physical-input script or interactive play to survive later waves
(the extension reaches P1 HP2), and test subsequent scene transitions and
post-death/continue. No later missing BIOS/runtime gate has been demonstrated.
Cold boot, other DIP settings, a second paid player and real-board pixel parity
are outside this verification. No edits were made to `project/`, `tools/decomp/`
or `tools/glm/` by this task; unrelated concurrent workspace changes were left alone.
