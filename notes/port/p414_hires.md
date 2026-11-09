# P4.14 enhanced internal resolution

2026-11-09. CPU implementation and evidence are in this repository; no commit.
**The requested 2x/60 fps performance acceptance is not met on this loaded host.**
The option remains opt-in, 1x remains the default, and 3x is labelled CPU intensive.
This report distinguishes renderer correctness from that unmet performance target.

## Use

Launcher: **Display and sound → Internal resolution**, or **Player features /
training → Time / autofire**. Pause: **Player features / training → Time /
autofire → Internal resolution**. Choices are **1x (original)**, **2x**, and
**3x (CPU intensive)**. Changes apply on resume; the saved INI key is:

```ini
[display]
internal_resolution = 1
```

The headless and windowed harness also accepts `--internal-resolution 1|2|3`;
an explicit command-line value overrides the initial INI value. Ordinary Sharp,
Smooth, Auto and CRT presentation continue to operate on the composed frame.
Typical game dimensions are 704×240, 1408×480 and 2112×720, before 4:3 window
presentation. The game's existing 704-dot hardware mode is independent of this
new factor. There is no GPU rendering or new guest-resolution mode.

## Design as built

`port/runtime-patches/p414-hires.patch` is incremental against P4.13 and changes
11 runtime files, including tests. `port/main_stv.cpp` supplies the CLI;
`port/stv.cmake` includes raster `.inc` files in the exact-build state identity.
The Windows portability patch is separately rebased against these sources.

VDP1 still executes every original command against the original 512×256-word
hardware framebuffer, including its 1024×256-byte interpretation. Those buffers,
registers, clipping state, command walk/cutoff, CPU reads and draw-end time are
unchanged. Enhanced output cannot feed a guest read or decide how many original
commands execute. It uses two independent, 16-bit-per-dot companions, with room
for 1024×512 dots times the chosen factor on each axis; the full-height allowance
also accommodates the existing interlace path. This costs 8 MiB at 2x or 18 MiB
at 3x for the pair, before output/state working memory.

Commands that the original walk actually executes are streamed in batches to
up to four workers. Each has private raster/clip state, an immutable 512 KiB
texture/table snapshot and a disjoint horizontal band. They retain command
order within their bands; the guest waits for all workers before `draw()`
returns. No worker writes hardware buffers, registers or guest memory. The
original cutoff and draw accounting are independent of presentation work.

Polygons and distorted sprites scale their placed vertices and inclusive
coverage; clips cover complete original pixel cells. Texture dimensions remain
texels, and repeated enhanced samples of an end-code texel count once. Normal
and scaled sprites, polylines and lines replicate their original raster cells
with nearest sampling. Mesh parity uses original coordinates. RGB replacement,
MSB-on, shadow, half luminance/transparency and Gouraud use the existing colour
operations; the game's 8-bit mode retains its original replace-only behavior.

Framebuffer swaps use the original index. Erase and CPU word/byte/longword
writes update the corresponding companion, including both palette bytes.
Mode changes invalidate companions. Resolution changes resample the retained
presentation buffer, or seed it from authoritative hardware when enabling the
feature; the next draw adds new geometry detail. They never redraw an old
frame with a newer command list or overwritten textures. Mixed non-interlaced
VDP2 display of a full-height VDP1 draw falls back to authoritative hardware
sampling; CPU writes do not overwrite the separate interlace display copy.

VDP2 emits the selected output dimensions and samples normal planes with exact
integer rational coordinates. Division by three occurs after the coordinate
product, avoiding cumulative 3x stepping error. Rotation planes and sprite
rotation retain fractions through their transforms. Line-scroll records, back
and line-colour tables and inclusive windows remain in original hardware
coordinates. Priority/tie order, colour calculation, offsets and shadows retain
the original meanings. Enhanced-only caches decode CRAM and 8-bit sprite dots,
reuse untransformed normal rows, cache row rotation terms and skip lower layers
under an opaque top image. The general compositor handles colour-calculation
cases. An enhanced-only complete-frame cache compares VDP2 VRAM, CRAM and
render registers exactly and tracks revisions of the displayed VDP1 companion.
It reuses RGB output only when all those inputs match. TVSTAT/HCNT/VCNT are
excluded because this compositor does not read them. Draws, erase, CPU writes,
mode/resolution changes, swaps and restored state invalidate the relevant VDP1
revision. The cache owns its image; callers still receive a complete Frame,
and every requested field is presented. This is independent of frame skipping.
It adds up to one enhanced RGB frame plus about 0.5 MiB of video snapshots.
1x bypasses the cache. CRT blur/mask/scanline spacing stays anchored to original
dot/line cells.

Snapshots contain the actual companion scale and both completed buffers in the
VDP1 part. Restore does not depend on current VRAM contents. Rewind and embedded
input states use the same serializer. Resolution is a host preference; loading
a state at a different selected factor resamples the restored presentation
buffer. Existing exact-build state/input identity checks remain in force.

## Correctness evidence

Native runtime tests: **160 tests, 0 failed**, `work/p414/tests.log`. New checks
cover hardware-buffer equality across factors; CPU byte/word/longword writes;
8-bit packed dots and erase; system/user/inclusive clipping; end-code texel
counting; original mesh cells; RGB shadow/transparency/luminance/Gouraud;
adjacent sloping polygons; persistent hardware draw state shared between SH-2
host threads; restored buffers despite overwritten VRAM; interlace
copy authority; exact fractional NBG/RBG sampling; and priority/window/colour/
shadow behavior at original-coordinate anchors; and exact frame-cache
invalidation on VRAM/CRAM/window changes, CPU framebuffer writes and restore.
The cache test also verifies caller-owned output, ignored raster counters and
the 1x bypass.

The final default regression is `work/p414/regression-final.log` and
`work/p414/regression/manifest.json`: **56/56 reference PNGs, all four complete
traces, four guest traces and four WAVs are byte-identical**. This includes the
9,000-field no-coin attract schedule and game-over/new-game/combat schedules.
References are the unchanged P4.11-corrected/P4.13 manifests; nothing was
rebaselined to accept a pixel or logic change.

`work/p414/verify/equivalence.json` and `work/p414/equivalence.log` establish
**2,100/2,100 fields with identical, unmasked 2 MiB RAM and semantic records**
at 1x/2x/3x, identical WAVs and byte-identical hardware video dumps at VB1800
and VB2100. In addition, all **22 machine-state parts** at VB2000 compare
byte for byte after removing only the enhanced VDP1 companion payload. CPU
contexts, scheduler/interrupt/timer state, BIOS, SCSP/DSP, VRAM, hardware
framebuffers and device registers remain in that comparison.

The authority verifier records one physical input stream, then replays it at
all three factors. Replay supplies the same RTC origin and initial BIOS data;
replay-versus-replay snapshots also keep host autoplay-event bookkeeping
identical. No RAM mask or adjusted game-state comparison is used.

`work/p414/verify/state-equality.json` and `work/p414/state.log` pass both
factors: **120 repeated rewind fields**, **1,700 power-on replay fields** and
**200 cold-loaded/embedded-state replay fields**, with exact pixels, unmasked
RAM, semantics and PCM. These full-length runs precede the final SDL viewport and cross-thread device
state and exact-cache corrections; short tests and the authority/default regressions
are repeated on the final binary below. Final short checks also pass at both
factors: **60 repeated rewind fields, 850 power-on replay fields and 150
cold-loaded/embedded-state replay fields**, all with exact pixels, RAM,
semantics and PCM (`work/p414/verify-short/state-equality.json`). The final
2,100-field authority run again matches every one of the 22 state parts. All
three final fight PNGs are also byte-identical to the inspected comparison
images (`work/p414/final-visual-equality.json`).

The full rewind runs report 3.414 ms/capture at 2x and 7.106 ms/capture at 3x.
The 2x run retains 278 snapshots (about 9.3 s at 30 snapshots/s); the 3x run
retains 141 snapshots (about 4.7 s). Peak totals include the base image and
working storage in addition to the 512 MiB delta budget. Busy graphics therefore
shorten history; the 120-field reversal still passes at both factors. Exact
counts/bytes are in `work/p414/rewind-memory.json`.

## Visual inspection

The sheets use nearest resampling into equal 960×720, 4:3 panels, ordered
**1x / 2x / 3x**. Raw PNGs retain the actual internal pixels. Commands, settings,
environments and SHA-256s are in `work/p414/visual/manifest.json`.

![Attract comparison](p414/attract-comparison.png)

Attract VB1200, with no coin: helicopter skids, cockpit/body and rotor edges
show finer geometric steps. Original subtitle texels remain crisp and aligned.

![First fight comparison](p414/fight-comparison.png)

First fight VB1800, physical-input autoplay: railings, actor silhouettes,
clothing and floor geometry gain detail. The HUD stays in its original cells;
the bright rectangle and original authored overlaps are present at 1x too.
Mesh shadows preserve original cell size.

![Transformed floor comparison](p414/floor-comparison.png)

Transformed floor, Scene 9 VB1200: all three fresh runs use P4.13's explicitly
labelled next-power-on scene selector (`start_scene = 9`) and physical-input
autoplay. This is a scene-rendering fixture, not a full-route claim. Diagonal
floor motifs, wall edges and character contours resolve more finely without
moving the HUD or opening an obvious seam between surfaces.

![Ending comparison](p414/ending-comparison.png)

Ending VB154992: offline composition of the retained P4.11 exact device capture,
through the current production compositor. Its entire displayed VDP1 buffer is
zero (all 524,288 captured dots), so only VDP2/RBG artwork contributes. No stale
command-list reconstruction or sprite substitution is involved. The green GAME
OVER zoom is authored and preserved; fractional sampling improves its edges.
This is explicitly offline ending evidence, not a new 3x full-game execution.
A fresh 1x physical-autoplay route also completes **155,100 fields, zero FATAL**,
including the ending, and its VB154992 PNG is **byte-identical** to this offline
1x reconstruction (`work/p414/full-route.json`). That route began with the first
P4.14 binary before presentation polish; no final-build 3x full-route claim is
made. The existing full capture is therefore independently confirmed at the
exact displayed ending field, without migrating a save or bypassing identity.

All four sheets and individual raw images were opened and inspected. No obvious
new cracks, HUD displacement or clipping failure was found in these views.
This is scoped visual evidence, not a proof for every camera/command combination.
Launcher and real SDL software window/filter captures are under `work/p414/ui/`.
Sharp/Smooth/CRT captures after the viewport correction were also opened and
inspected at 960×720, along with the launcher and actual SDL-event pause panel.
The initial main-tab label clipped at the column edge; its width was corrected.
SDL filter inspection also exposed a smaller 2x viewport under integer scaling.
Viewport dimensions now use original TV lines, so changing internal resolution
preserves the same 4:3 area and HUD size in the window. All three filter captures
were repeated and inspected after that correction (`work/p414/ui-viewport.log`).

## Performance

Host: AMD Ryzen Threadripper PRO 5975WX, 32 cores / 64 logical CPUs. A sampled
interval found every allowed CPU more than 90% busy; load average also exceeded
64. Independent 1x software-window controls fell below real time. No unrelated
process was stopped or reprioritized. The measurements therefore describe this
loaded host, and do not establish idle-host or macOS/Windows throughput.

Final measurements: **3,600 fields per factor**, run sequentially after all
owned capture/regression jobs completed, frameskip **off**, Sharp, 960×720.
Combat samples are the 5-second intervals ending at VB900 or later; the table
includes both whole-run throughput and the complete combat range.

| Resolution | Whole run fields/s | Combat fields/s (mean; range) | Process CPU | Omitted pictures |
|---|---:|---:|---:|---:|
| 1x | 45.45 | 44.09; 40.29–48.36 | 244% | 0 |
| 2x | 33.98 | 33.45; 30.11–37.58 | 242% | 0 |
| 3x | 26.82 | 26.17; 22.14–31.04 | 275% | 0 |

**2x does not meet 60 fps; 3x does not either.** No picture skipping is hidden in
these measurements. The eligible-field cache counters near the end of the 2x
run show **1,098 reused / 2,423 composed**, and at 3x **1,079 / 2,377**; cached
fields are still presented. Load averages at completion were approximately
46.7 / 51.7 / 48.3 for 1x/2x/3x. The preserved **P4.13 executable** also misses
real time in a separate 1,800-field control: its combat samples span
**32.28–46.26 fields/s**, **214% CPU**, zero omitted pictures. These runs occur
at different moments on a shared host; they do not establish a causal speed
comparison or an idle-host guarantee.

The requested 2x performance acceptance therefore remains **unmet**, rather
than being inferred from original-resolution behavior or from guest fields
that were not presented. Per-interval samples, exact commands/settings, CPU
use, cache counts and load averages are in `work/p414/performance-summary.json`,
`work/p414/performance/final-*-paced.json` and their `run.log` files. The prior
uncached measurements are retained under `work/p414/performance-before-cache/`.

`port/p414_measure.py --fields 3600 --tag final` runs `--stats 5` with SDL dummy video/audio and the
**software** renderer, 960×720 output, unchanged physical autoplay and explicit
frameskip choice. `/usr/bin/time -v` records process CPU use (100% = one logical
CPU). Report JSONs retain every fields/s sample and omitted-picture count.
Offline composition measurements are diagnostic only, not gameplay fps.

## Build, patch and platform verification

```bash
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p414/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p414/runtime-tests -j8
work/p414/runtime-tests/runtime_tests
python3 port/p414_verify_regression.py
python3 port/p414_verify_hires.py --mode equivalence
python3 port/p414_verify_hires.py --mode state
bash port/p414_build_probes.sh
python3 port/p414_capture_scenes.py --part ending
python3 port/p414_capture_scenes.py --part scenes
python3 port/p414_capture_scenes.py --part sheets
port/p412_build_win.sh
python3 port/p414_verify_patches.py
```

The ending helper needs `work/p414/probes/p414_render_dump`, built from the
`port/p414_build_probes.sh` CMake project in `work/p414/probe-source`; it links the production
runtime without SDL or a recompiled game. `--part ending` documents the retained
capture inputs and checks that every displayed VDP1 dot is zero.

Linux window build passes. Windows cross-build passes, including a separately cross-compiled
`work/win/p414-tests/runtime_tests.exe` (not executed here), with the existing pinned
llvm-mingw/UCRT and SDL3 toolchain; the refreshed distribution is
`work/win/DieHardArcade-windows-x64.zip`. PE dependency inspection packages only
SDL3/libc++/libunwind and ordinary Windows imports; no ROMs are distributed. ZIP SHA-256:
`32bac4fbb574c57bfdfa1fed28d24b57958f5afec2f4ff3cc060c3d850639aa8`.
Full executable and patch hashes are in `work/p414/artifact-hashes.json`.
`work/p414/patch-validation.log` reconstructs **76 runtime files** through the
P4.13/P4.14 chain and **127 Windows files**, byte for byte, zero fuzz/offsets.

Runtime additions use standard C++20 and existing portable SDL/ImGui facilities.
No platform-specific renderer or graphics API was added. A macOS SDK/desktop is
not available here; the PM must rebuild/run `port/p48_build_mac.sh` and the app
wrapper. Wine/real Windows execution is left for the PM, as allowed by the task;
no Windows or macOS runtime-performance claim is made.

## Open issues

- **2x/60 fps acceptance is unmet on this host.** Default frame skipping can
  preserve more guest fields than pictures; those rates must not be confused.
- **3x is CPU intensive** and remains explicitly labelled. No sustained 60 fps
  claim is made.
- Original textures and palette precision still cap detail. Enhanced edges are
  an optional raster interpretation; they do not correct P4.11's remaining
  original-resolution VDP1 coverage/RBG sampling differences against MAME.
- Larger snapshots increase rewind working memory and may shorten retained
  history at the existing 512 MiB limit. Exact measurements are in state logs.
- The game's default non-GPL software raster is the validated build.
  `SATURN_VDP1_GPL=ON` is unsupported by this enhanced pass. Temporal
  interpolation is not wired into this ST-V harness. Full interlace-mode-switch
  coverage remains unverified beyond the specific buffer-authority tests.
- Physical controllers/speakers, real desktop filters, macOS and Windows play
  remain external checks. Existing unsupported VDP2 modes are not expanded here.

No convergence source/evidence, decompilation work, prohibited directories or
unrelated processes were modified. This P4.14 job made no commit and used no
sudo or GPU operation. The concurrent convergence job advanced HEAD from
`c60168b` to `adadacb` during the run; its commit contains convergence files and
its report. P4.14 files remain uncommitted, and its runtime patch base and tested
default sources are unaffected.


## Ten-line summary

1. Implemented opt-in 2x/3x CPU rendering; 1x remains the original default.
2. Original hardware buffers, device behavior and guest scheduling remain authoritative.
3. Scaled VDP1 geometry, fractional VDP2 sampling and exact field reuse are presentation-only.
4. All 160 runtime tests and 56 reference PNGs/four traces/four WAVs pass.
5. 2,100 fields and all 22 normalized machine-state parts match across 1x/2x/3x.
6. Enhanced rewind, quick load and both recording/replay origins pass pixel/RAM/PCM checks.
7. Four scene comparisons and SDL filters were inspected; viewport/HUD sizing was corrected.
8. 2x measures 33.98 fields/s, 242% CPU; the requested 60 fps acceptance is unmet.
9. 3x measures 26.82 fields/s, 275% CPU; it remains labelled CPU intensive.
10. Windows ZIP is refreshed; macOS/Windows play remain external checks; no commit or convergence edits.


## macOS verification (P4.14)

2026-11-09, Intel Mac (i9-9880H, 8 cores / 16 logical, macOS 26.5.1, Apple clang 21,
Homebrew SDL3). The Mac copy was reset to `1980322` (previous local P4.13 changes backed up
in `work/p414mac/local_uncommitted.diff` / `local_untracked.tgz`). `p414-hires.patch` applied
to the Mac's P4.13 runtime with zero fuzz/offset; the resulting runtime (`git diff` plus
untracked files, ignoring P620's stray `video.cpp.orig`) is content-identical to P620's.
No macOS-specific fix was needed. Evidence: `work/p414mac/` (Mac copy only).

| Check | Result |
|---|---|
| `port/p48_build_mac.sh`, `port/p411_build_app.sh` | Both build, zero warnings |
| Runtime unit tests (clang, `SATURN_NO_SDL`) | **160 tests, 0 failed** |
| Smoke 2000 fields, shot 1900, default and `--internal-resolution 1` | md5 **`9bd5386ea7439621c19674381086e8fd`**, P4.8 binary and bundled app binary. Matches Linux/Windows |
| Same, `--internal-resolution 2` | md5 **`8f1d9a1f01a2b77cd9d4f82b22b71b37`**, both binaries. Matches Linux/Windows |
| Same, `--internal-resolution 3` (no cross-platform reference yet) | `a48d6414e53a778c69cfe4fbe71ba423`, 0 FATAL |

App: `port/run/P411/Die Hard Arcade.app` is now **v1.03** (Info.plist 1.03/103, startup log
line `Die Hard Arcade 1.03`; ad-hoc signature verifies with `codesign -vv --deep`). The v1.02
app is kept unchanged as `port/run/P411/Die Hard Arcade v1.02.app`.

### Performance (macOS)

Method as `port/p414_measure.py` (3,600 fields, frameskip off, Sharp, scale 3, autoplay 100,
`--stats 5`), via the macOS port `work/p414mac/mac_measure.py` (BSD `time -p`; CPU% =
user+sys / wall, 100% = one logical CPU). Whole run = 3,600 / wall time including start-up;
combat = 5 s samples ending at VB900 or later. Host otherwise idle (load average 4.2-5.3
on 16 logical CPUs while running, mostly this process; no other heavy jobs).
"Real" = visible desktop window, SDL Metal renderer; "dummy" = SDL dummy video + software renderer.

| Resolution | Mode | Whole run fields/s | Combat fields/s (mean; range) | CPU | Omitted pictures |
|---|---|---:|---:|---:|---:|
| 1x | real, paced | 58.60 | 58.66; 55.35-60.59 | 290% | 0 |
| 2x | real, paced | 39.59 | 38.57; 32.20-44.53 | 293% | 0 |
| 3x | real, paced | 24.51 | 23.40; 18.54-28.90 | 310% | 0 |
| 1x | dummy, paced | 58.64 | 58.35; 55.69-59.95 | 285% | 0 |
| 2x | dummy, paced | 40.07 | 38.90; 33.85-44.91 | 289% | 0 |
| 3x | dummy, paced | 24.65 | 23.48; 19.37-28.52 | 307% | 0 |
| 1x | dummy, `--unpaced` | 60.51 | 58.98; 55.42-61.80 | 291% | 0 |
| 2x | dummy, `--unpaced` | 40.90 | 39.87; 34.33-46.52 | 292% | 0 |
| 3x | dummy, `--unpaced` | 24.94 | 23.89; 19.77-29.09 | 311% | 0 |

On an idle i9, **2x reaches about 40 fields/s and 3x about 24; neither meets 60 fps**. The
unpaced runs show the ceiling: even 1x is only just real time on this CPU (combat dips to
about 55), so there is no 1x headroom to absorb the enhanced raster cost. The window/Metal
path costs nothing measurable versus dummy, so the limit is CPU emulation + raster, not
presentation. 2x frame cache: 1,041 reused / 2,320 composed. The P620 conclusion (2x/60
unmet) therefore also holds on an unloaded host; it was not only load-induced.

### Launcher

`SATURN_LAUNCHER_SHOT` captures with `internal_resolution = 2` (`work/p414mac/ui/`):
dummy/software `launcher-main.png` and `launcher-features.png` (960x720) were opened and
inspected. **Internal resolution** shows `2x` with its label fully visible on the main tab
(Display and sound) and on Player features -> Time / autofire; nothing is clipped.

**Pre-existing Retina bug found** (also present in the v1.02 app, so not a P4.14 regression):
in a real window on a Retina display (`launcher-main-real.png`, `-real-f60.png`,
`-real-v102.png`, 1920x1440 backbuffer), the launcher draws at 1x pixel scale in the
top-left quarter, and the right-hand Arcade controls column is missing. Cause: ImGui 1.92's
`imgui_impl_sdlrenderer3` scales clip rects by `DisplayFramebufferScale` (2.0) when the
renderer's scale is 1, but the vertices are not scaled, so anything right of half the
logical width is clipped away. `launcher.cpp` lines 252 and 618 (launcher and in-game
overlay) call `RenderDrawData` without `SDL_SetRenderScale(ren, io.DisplayFramebufferScale.x,
io.DisplayFramebufferScale.y)` first (and resetting to 1 afterwards). Not fixed here. The
dummy captures and the P4.13 macOS checks never exercised a HiDPI framebuffer. A desktop
screenshot could not be taken (no Screen Recording permission), but `SDL_RenderReadPixels`
reads the same backbuffer that is presented.

PM addendum: Linux 3x shot-1900 md5 a48d6414e53a778c69cfe4fbe71ba423 matches macOS 3x; 1x/2x also match Linux, macOS and Windows (Wine).
