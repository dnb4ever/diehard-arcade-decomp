# P4.11 fidelity and polish

2026-11-08. Investigations complete; fixes verified, remaining differences and
internal-resolution deferral documented. No commit. Paths are repository-relative.
Other jobs' `project/`, `tools/decomp/`, non-port notes and macOS packaging files
are outside this work. The initial P4.10 executable is preserved as
`work/p411/baseline-stv` and renderer source copies as `work/p411/before/`.

## 1. Ending green field

The historical example is `port/run/P45/auto/shot-159000.png`, also displayed in
`port/run/P45/auto_end.png`. Visually inspected: this is a textured close-up,
including gray strips and saturated green areas, rather than a uniform fill.
49,104 of 168,960 pixels are `(0,248,0)`. That run predates the final P4.5
high-resolution RBG correction, so it cannot by itself establish a current renderer defect.

`port/runtime-patches/p411-scene-capture.patch` adds read-only, default-off
`STV_CAPTURE_SCENES=FIRST:LAST`. Every matching field retains the ordinary device
dump, PNG and a separate `shown-N.bin`: 1024 by 512 big-endian displayed VDP1
dots queried with double-density row coordinates. This avoids mistaking the
ordinary dump's **draw** framebuffer for the displayed one. It does not change
registers, memory, CPU scheduling or panel input. `port/p411_render_dump.cpp`
recomposes a field offline and can isolate sprites, planes or back screen.

Full current-route capture: `work/p411/full-capture/`, physical inputs from
`STV_AUTOPLAY=100`, unchanged original autoplay policy, 200,000-VBlank limit,
ending scenes 73..81 captured individually. MAME physical-input-only reference:
`port/p411_mame_play.lua`, output `work/p411/mame-final/`. No actor HP, scene,
RNG or other game-memory writes are used by either reference player.

**Verdict: the green transition is authored, and MAME shows it too.** It is the
large green **GAME OVER** artwork zooming into view on RBG0 in Scene 81, after
the credits. It is not a uniform back-screen flash. Removing it would change
the original game's animation. No green-frame suppression or timing workaround
is added.

The current native run enters ending Scene 73 at VB 154125, then 79 at 154178,
80 at 154293, 81 at 154894 and returns to attract Scene 2 at 155192. It retains
**1,067 consecutive fields**, VB 154125..155191. MAME reaches Scene 73 at frame
140391, 79 at 140452, 80 at 140487, 81 at 141088 and Scene 2 at 141383. Both
physical-input players can press START during the ending; no unskipped or
alternate 74..78 credits-route claim is made.

Diagnosis from native VB 154992:

- TVMD 8013 has display enabled; BGON 0018 enables RBG0 and NBG3. The isolated
  back and sprite images are black (`work/p411/green-back.png`,
  `work/p411/green-sprites.png`); the plane image retains the green artwork
  (`work/p411/green-planes.png`). This excludes stale VDP1 presentation and a green back
  screen as its source.
- The relevant RBG0 palette bank, entries 512..767, is **byte-identical in both
  engines**. Entry 556 is RGB555 03E0, saturated green. That bank does not change
  throughout the green interval, so this is not a partially uploaded palette.
  RAMCTL 0323 and the colour-offset registers are stable; offsets are zero.
- Native VB 154992 and MAME frame 141182 have **identical 128-byte rotation
  parameter blocks**. Each engine has exactly **22 consecutive fields above
  40% green**: native 154982..155003, MAME 141172..141193. Peaks are 57.30% and
  57.63%; the differing field numbers are not a gameplay timing correction.

Inspected the complete native/MAME ending contact sheets
(`work/p411/ending-native.png`, `work/p411/ending-mame.png`) and full-size same-pose images
(`work/p411/ending-same-pose.png`). The old coarse 500-VBlank capture made this fast zoom
look like an isolated unexplained green picture. The new field-by-field evidence
shows the text entering, crossing the near view and settling into GAME OVER.
Hashes/counts are in `work/p411/ending-root-cause.json` and both field inventories.

The true colour-range discrepancy is fixed separately in part 3. Applying that
correction to the captured field gives MAME's green 255 instead of 248 and the
same visual animation. **Pixel-perfect RBG0 equality is not claimed**: the
same-pose images still differ at 2,341/168,960 pixels, mostly sampling boundaries.
Rendering MAME's own captured VDP2 data through the corrected native compositor
also leaves 2,675 differing pixels, isolating a remaining renderer sampling
difference from game state. This remains ranked in part 3 rather than hidden by
a scene-specific adjustment.

An additional stable oracle imports that exact VDP2 state into MAME with both
CPUs parked and sprites disabled, then waits multiple fields before capture.
It leaves the same **2,675** differences, excluding a frame-notifier/presentation
phase mismatch as their explanation (`work/p411/rbg-stable-mame/comparison.json`,
`work/p411/rbg-stable-mame/comparison.png`). The green artwork persists unchanged
in that stable reference too.

Capture-only verification: `work/p411/capture-regression.log` reports **56/56
PNGs and all four deterministic trace hashes identical** to
`port/run/P48/p410-final/manifest.json`. The probe's first offline reconstruction
(`work/p411/capture-smoke/recomposed.png`, VB13) is byte-identical to its original
PNG. `port/p46_build_window.sh` passes (`work/p411/capture-build.log`).

## 2. `0601FC1C`

P4.6 already documented a real Scene 8 route and coverage-confirmed its seed.
P4.5's unverified wording is historical. Repeated on the current build:
`work/p411/fc1c-verification.log`; both 7,600-VBlank historical and 8,500-VBlank
retimed schedules finish with zero FATAL and all six requested PNGs.
`port/run/P48/p411-fc1c/fc1c-retimed/cov.txt` contains both **0601FB40 and
0601FC1C**. The historical hold alone no longer reaches the child on the P4.7
timing; the retimed hold starts at VB7850 and lasts 500 VBlanks.

The parent at `0601FB40` installs the child from state 2 after its countdown.
This is a real physical-input route, not speculative dead code or a second-game
special case. `port/seeds_extra.txt` already seeds it and the generated table
contains an active entry. No new fix is needed. The 12,000-VBlank two-player
run (`work/p411/two-player/`) also exits normally, has both paid players and
continues, and reaches the parent. Inspected `shot-2500.png`: both HUDs and
players are visible. A second two-player run, holding both raw 0040 directions
from VB 11220 for 700 fields, reaches **both parent and child**, holds Scene 8
from VB 11229 to 11526 and finishes 12,200 fields without FATAL
(`work/p411/two-player-hold/cov.txt`). Inspected its `shot-11500.png`.
Part 3 corrects that direction's historical UP label to physical DOWN; the raw
input is unchanged. These checks do not claim exhaustive alternate QTE/character
coverage. The compiled entry is registered for the entire active cart module,
so the old missing-entry FATAL cannot recur on another cart route merely because
it reaches the same child later.

The full **200,000-VBlank** run finishes with **zero FATAL**, covers the child,
clears all combat stages through Scene 72, traverses ending 73/79/80/81, starts a
second game and reaches Scene 32 again. It makes 98 autoplay coin/start requests
(including continues, initial start and ending/new-game requests). Attract and
game-over/new-game are also covered by the 56-field schedules. Detailed scene
edges and coverage: `work/p411/full-capture/report.json` and
`work/p411/full-capture/cov.txt`. The final corrected build independently repeats the retimed child
route with physical DOWN: `work/p411/final-fc1c/`, zero FATAL and both entries
in coverage. No new seed or decompilation change was required.

## 3. Remaining MAME differences

A fresh two-engine P4.7 checker capture completed in
`work/scenario_p411_baseline/`, with automatic first-attract-edge calibration,
unchanged RAM mask, full RAM/video capture and the original four scenarios.
Calibration remains +510. The first field boundary remains port 9/MAME 519,
first screen mismatch port 13/MAME 523, and whole masked RAM matches remain zero.
The four coverage outcomes reproduce P4.7 exactly:

| Scenario | Equal field records | Equal screen records | Coverage |
|---|---:|---:|---|
| attract | 312/9292 | 40/9292 | PASS |
| game_over | 308/5592 | 27/5592 | UNMET |
| scene1_combat | 308/2092 | 27/2092 | PASS |
| scene3_plus | 308/9992 | 37/9992 | UNMET |

All four are **DIVERGED**, with no unavailable port fields. The checker retains
its original goals and RAM mask. The existing game-over return beyond the
5,600-field scenario and the failed Scene 9 target were not waived.
`work/p411/p47-repeat-summary.json` retains the detailed boundaries.
The unchanged checker suite passes 12 tests (`work/p411/checker-tests.log`).

Four separately evidenced fixes are installed:

- Physical MAME panel inventory (`work/p411/mame-inputs/inputs.log`):
  Up/Down/Left/Right are **80/40/200/100**, A/B/C are **1/2/4**. Historical
  HLE direction names are reversed on both axes. The old MAME reference player
  consequently walked away from enemies; the corrected player uses the real
  physical field names. Autoplay's literal raw-word choices are unaffected.
- `work/p411/video-mame/`: the isolated VDP2 fixture parks the emulated CPUs,
  supplies explicit video data and compares identical data through both
  renderers. This is a device test, **not** full-game route evidence. The
  horizontal/vertical line scroll, line zoom and 2-line intervals, RGB555 back
  screen and direct-RGB bitmap corrections give **four pixel-identical images**
  against MAME 0.285 (`work/p411/video-mame/final-comparison.json`). The static FBI screen also becomes
  pixel-identical after full-range 5-bit expansion: original port VB 300 versus
  MAME frame 600 (`work/p411/fbi-expanded.png`, `work/p411/fbi-mame.png`).
- `work/p411/envelope-mame-final/envelope.log`: isolated SCSP release cases read
  **2,2,3,4,31** in MAME. The old native rates read **3,3,3,3,31**. The corrected
  64-step index uses twice the rate, signed octave, twice KRS and FNS bit 9,
  with KRS 15 disabling key scaling. The production native probe matches all five
  (`work/p411/envelope-final.txt`).
  The fixture parks/replaces sound-CPU execution; it does not count as a game
  route. Current real-game voices use KRS 0 and negative octaves, so the defect
  affects actual music/effects rather than only a synthetic register combination.

Current SCSP effects are **active**, contrary to the old P4.4/P4.5 observation:
native attract key-ons include EFSDL 7, and MAME register captures also contain
effect sends and DSP inputs (`work/p411/audio-mame/slots.json`). No missing-DSP
claim or speculative DSP rewrite is justified. Real-game PCM measurements are
in `work/p411/audio-summary.json`; no speaker audition is claimed.

Runtime changes are the four ordered `p411-*.patch` files listed in
`port/runtime-patches/README.md`. The final suite passes **142 tests, 0 failed**,
including both players' direction words, all 32 RGB555 levels, NBG0/NBG1 line
records/intervals and the MAME-measured envelope rates. All nine changed runtime
files reconstruct byte for byte with zero fuzz (`work/p411/patch-validation.log`).

`work/p411/panel/report.json` verifies the real SDL3 host path: four directions
from keyboard events and four from two virtual gamepads, **both players in
every case**, all matching the measured BIOS words. These are synthetic SDL
device/event checks, not a physical-controller or macOS desktop claim. Inspected
`work/p411/regression/physical/combat/shot-1440.png`: physical RIGHT now moves
toward the screen's right. Both current-label combat schedules finish normally.

Final P4.7 comparison: `work/scenario_p411_final_verified/`. Native captures are
fresh after the fixes; MAME captures are the fresh baseline from this milestone,
reused only after verifying executable/ROM/input/tool hashes and the unchanged
RAM mask. Port calibration is remeasured automatically and remains **+510**;
no later scene or RNG realignment is introduced by this checker.

| Scenario | Equal screens before → after | Equal field records after | Coverage after |
|---|---:|---:|---|
| attract | 40 → 619 | 312/9292 | PASS |
| game_over | 27 → 323 | 308/5592 | UNMET |
| scene1_combat | 27 → 323 | 308/2092 | PASS |
| scene3_plus | 37 → 335 | 308/9992 | UNMET |

All remain DIVERGED, with no unavailable fields, zero whole masked-RAM matches
and the same first field/screen boundaries. The following ranked list excludes
the previously known per-safe-point timing floor and its resulting camera,
actor and RNG trajectory differences:

| Rank | Remaining visual/audio difference | Evidence and decision |
|---:|---|---|
| 1 | RBG0 sampling at rotation/zoom boundaries | Same-pose ending differs at 2,341/168,960 pixels; importing MAME's own video data leaves 2,675 differences. Same artwork/pose/colour, differing sampling boundaries. A faithful fixed-point/rounding audit is needed; no fitted coordinate bias added. |
| 2 | VDP1 quad coverage and texture stepping | Identical, isolated 8-bit VDP1 command/texture fixture differs at **3,613/168,960 pixels**. Inspected `work/p411/video-mame-complete/geometry-comparison.png`; discrepancies include distorted texture boundaries and line coverage. Replacing raster rules needs broader shape/mesh/blending validation. |
| 3 | VDP2 colour-calculation ratios | Isolated ratio-zero fixture has at most **7 RGB levels** of error against MAME. Native retains 31:1 at ratio zero; MAME's endpoint is effectively opaque. Kept because Sega documents 31:1 through 0:32, rather than changing hardware behavior to imitate this reference approximation. |
| 4 | Output level and exact PCM | Native intentionally applies **27/32** output headroom. In the aligned music window, corrected native RMS is **441.18**, MAME **523.79**; recordings also use 44.1 versus 48 kHz containers. No automatic gain increase or claim of sample-exact audio is made. Full DSP/rounding waveform equivalence remains unverified. |

Ratio reference: [Sega VDP2 manual, colour calculation](https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p12_11.htm).
The measured SCSP correction also agrees with the
[MAME SCSP rate computation](https://github.com/mamedev/mame/blob/mame0285/src/devices/sound/scsp.cpp).
Device fixtures deliberately supply video/sound state with CPUs parked; they
do not substitute for the physical-input full-game route.

Other unimplemented renderer features remain RPMD 3/window switching,
screen-over pattern mode, coefficient line colour, vertical cell scroll,
mosaic/sprite windows and special functions by dot. No new visible fault from
those features was established here; they are unverified coverage, not invented
ranked defects. Real BIOS TEST/reload, backup bookkeeping, physical speakers,
controllers and macOS desktop execution retain the prior milestones' limits.

### Reference changes and validation

Original P4.5 timings/pixels already differed before P4.11 because of the
documented P4.7 boot/RAM/input-phase fixes. The exact current baseline is
`port/run/P48/p410-final/manifest.json`; the capture-only pass matches **56/56
PNGs and all four full traces** against it.

After the evidenced fixes, `work/p411/regression/report.json` audits every
field using the original captured VRAM/CRAM/registers/displayed VDP1 dots:

| Schedule | Original fields | Changed PNGs | Final pixels equal offline prediction | Guest trace |
|---|---:|---:|---:|---|
| no coin | 30 | 29 | 30/30 | identical |
| game over/new game | 16 | 15 | 16/16 | identical |
| combat | 5 | 5 | 5/5 | identical |
| legacy combat | 5 | 5 | 5/5 | identical |

The **54 changed PNGs are entirely explained by full-range RGB555 conversion**;
the two black pictures remain byte-identical. Disabling the new line-scroll
path in the offline probe changes none of these particular 56 fields. Thus no
camera, sprite order, geometry or unexplained pixel change is hidden in this
regression. The generic line-scroll correction is independently pixel-verified
against MAME for the tested compact records and intervals.

The after schedules swap old direction **names** to preserve original raw
panel words. Separate physical-label schedules verify the corrected behavior.
All four guest scene/coin/stop traces remain identical. Full trace hashes and
PCM change deliberately with corrected SCSP envelopes; total sample counts
remain fixed. Manifests, exact argv/environment, PNG hashes, WAVs and logs are
under `work/p411/regression/{before,after,physical}/`. Inspected all 56 final
images through `work/p411/reference-after-1.png` through `work/p411/reference-after-4.png`,
plus individual gameplay, ending and fixture PNGs.

The 200,000-field route was captured before the device corrections. Every one
of its **1,067 ending fields** is also recomposed through the final production
renderer (`work/p411/ending-fixed/manifest.json`); this is explicitly an offline
render verification, not a claim of a second post-fix 200,000-field execution.
No graphics diagnostic writes guest state, and the default execution scheduler
and generated SH-2 program are unchanged.

## 4. Internal resolution

**Decision: feasible as a separate renderer feature; not a contained launcher
change. No internal-resolution setting is exposed by P4.11.** Default resolution
and the existing Sharp/Smooth/CRT presentation remain intact.

The renderer is entirely CPU based. `tools/saturn-recomp/runtime/src/video/vdp1.cpp` walks commands
at draw start and `vdp1_raster.inc` fills quads using integer edge steps and
Bresenham scan lines. Its two hardware buffers are 512x256 16-bit words, also
interpreted as 1024x256 palette bytes in TVMR mode 1 (used by this game).
Clipping, erase, mesh, Gouraud, shadow and half-transparency act on these actual
buffers; CPU framebuffer reads/writes and save states use them. `g_hi[2]` is
specifically a full-height **interlace** companion, not a general 2x renderer.

`vdp2_compose` reads the displayed VDP1 dots and independently samples the
normal/rotation planes, CRAM, priority, windows and colour calculation. It emits
one RGB `Frame` at TVMD resolution, normally 704x240 during play. Bands of lines
are composed on up to eight CPU threads. `host_present` then scales that finished
picture into the 4:3 SDL texture/window. Enlarging that texture alone cannot add
geometry detail. The existing `--interp` command-history renderer interpolates
time at the original spatial resolution; it is not supersampling.

| Option | Result | Estimated work / main risks |
|---|---|---|
| Existing Sharp/Smooth presentation | Larger original image | Already implemented; no added geometric detail |
| VDP2-only 2x/3x sampling | Sharper transformed floors; original-resolution actors | About 1–2 days including tests; incomplete answer for polygons/sprites |
| Separate 2x/3x VDP1 buffers plus VDP2 fractional sampling | Sharper polygon edges and plane transforms, original textures | About 3–6 engineering days plus fidelity/performance validation |

The complete option should keep both original hardware buffers authoritative
and add two separate presentation buffers. Commands must rasterize into both:
the enhanced pass scales vertices, local/system/user clips and inclusive edge
rules while keeping texture sizes in texels and mesh cells in original pixels.
End-code counting must not stop early when several new pixels sample the same
texel. CPU framebuffer writes and erase must update the companion; mode changes
must invalidate it. A 3x, 1024x256, two-buffer, 16-bit-per-dot companion alone
uses about **9 MiB**, before output and working buffers.

VDP2 must sample at `(x/scale,y/scale)` throughout normal and rotation planes,
sprite rotation, windows, line tables and back screen. Rounding increments once
for 3x would accumulate sampling error; keep fractional coordinates through the
transform. The original game vertices/textures and palette precision still cap
detail. Preserve pixel HUD alignment and avoid interpreting the game's existing
704-dot hardware mode as the enhancement factor.

Save/load must restore or rebuild the enhanced displayed buffers without
replaying a newer, already overwritten command/texture list. The default 1x
path must bypass all new allocation/raster/sampling behavior and retain the
56-shot, trace and PCM reference checks. Pixel work rises approximately 4x/9x;
VDP1 currently rasterizes synchronously on the guest thread, so CPU throughput,
audio headroom and frameskip are material risks. Estimates are engineering
judgments, not measured implementation times or a performance guarantee. No GPU
operation was used for this investigation.

## Reproduction and open limits

The four runtime patches apply after P4.10, in README order. Production build
and tests, from the repository root:

```bash
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p411/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p411/runtime-tests -j8
work/p411/runtime-tests/runtime_tests
bash port/p411_build_probes.sh
work/p411/probes/p411_scsp_probe
```

Recheck the retained before-state reference data with the current executable:

```bash
python3 port/p411_verify_fidelity.py after
python3 port/p411_verify_fidelity.py compare
python3 port/p411_verify_fidelity.py physical --schedules combat,legacy-combat
python3 port/p411_recheck_scenarios.py --output work/scenario_p411_repro
```

The `before` phase was run before installing the device fixes; its captured
states and immutable reference hashes are retained under
`work/p411/regression/before/`. Running `before` against the corrected executable
is expected to reject the intentional colour changes. The final rechecker
reuses this milestone's verified MAME captures and reruns all native scenarios;
the ordinary checker can also recompare its output:

```bash
python3 tools/scenario/check_scenarios.py --reuse --out work/scenario_p411_final_verified
```

Fresh long capture, with no other inherited `STV_*` variables:

```bash
mkdir -p work/p411/full-repro
STV_AUTOPLAY=100 STV_SCENETRACE=1 STV_COVDUMP=1 STV_CAPTURE_SCENES=73:81 \
  port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --vblanks 200000 --stvinput 99999:COIN --out work/p411/full-repro
python3 port/p411_frame_report.py work/p411/full-repro --output work/p411/full-repro-inventory

# Final direction labels, same raw 0040 hold as the historical retimed child repro:
mkdir -p work/p411/fc-repro
STV_AUTOPLAY=100 STV_COVDUMP=1 \
  port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --vblanks 8500 --stvinput 99999:COIN,7104:P1DOWN:400,7850:P1DOWN:500 \
  --shots 7850,8100,8500 --out work/p411/fc-repro
```

MAME physical-only full-route reference, using a fresh NVRAM/config directory:

```bash
mkdir -p work/p411/mame-repro
P411_OUT="$PWD/work/p411/mame-repro" \
LD_LIBRARY_PATH="$PWD/port/run/P45/mamepkg/x/usr/lib/x86_64-linux-gnu" \
  port/run/P45/mamepkg/x/usr/games/mame diehard -bios us \
  -rompath port/run/P45/mame/roms -video none -sound none -nothrottle \
  -skip_gameinfo -noreadconfig -nvram_directory work/p411/mame-repro/nvram \
  -cfg_directory work/p411/mame-repro/cfg -autoboot_delay 0 \
  -autoboot_script port/p411_mame_play.lua
```

For isolated video, sound or switch oracles, select respectively
`port/p411_mame_video.lua`, `port/p411_mame_envelope.lua` or
`port/p411_mame_inputs.lua` with the same command and a fresh `P411_OUT` directory.
The video fixture also accepts `P411_VDP2_SOURCE` pointing at a captured MAME
VDP2 dump to reproduce the stable RBG0 comparison. Offline render example:

```bash
work/p411/probes/p411_render_dump \
  work/p411/full-capture/dump-154992.bin work/p411/full-capture/shown-154992.bin \
  work/p411/recomposed-green.png
```

Final build/test logs: `work/p411/final-window-build.log`,
`work/p411/final-tests.log`, `work/p411/final-pixel-audit.log`,
`work/p411/final-scenarios.log`. The full generated SH-2 program, scheduler,
timing constants, BIOS launch defaults and save-state format are unchanged.
Runtime source identity changes invalidate older quick slots as designed in
P4.10. No Mac build, desktop/speaker test or higher-resolution performance claim
is made. No git commit, GPU use, sudo, prohibited-directory access or unrelated
process termination was performed. Other jobs' source, notes and packaging
changes remain intact.
