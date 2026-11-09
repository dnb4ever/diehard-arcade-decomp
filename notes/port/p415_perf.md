# P4.15 performance and Retina launcher

2026-11-09. No commit. Engineering changes and Linux verification are complete;
**idle i9/macOS performance acceptance still requires the PM's rebuild/run**.
All paths below are repository-relative. Initial HEAD was `b8eb944`; the
concurrent decomp job advanced it through `593f044` to `b1529ae`. Its sources, tools and notes
were left alone. Baseline executable: `work/p415/baseline-stv`; original runtime:
`work/p415/before/runtime/`. Evidence is under `work/p415/`.

## Retina / Windows DPI

`port/runtime-patches/p415-retina-launcher.patch` fixes both ImGui draw sites in
`tools/saturn-recomp/runtime/src/host/launcher.cpp`, including the in-game player-feature overlay.
The shared `tools/saturn-recomp/runtime/src/host/imgui_scale.h` wrapper applies **draw-data FramebufferScale** to
SDL geometry before the SDLRenderer3 backend renders. This backend already
uses logical clip coordinates when SDL's render scale is non-unit. The wrapper
restores the exact previous x/y renderer scale afterward. A normal 1x draw
makes no scale-setting calls. Scale is read each frame, so moving a window
between displays does not retain a stale ratio.

`port/p415_test_hidpi.cpp` renders actual ImGui geometry through the production
wrapper into SDL software targets at 1x, 1.5x and 2x, then 1x again. It checks
lower-right geometry, both clip boundaries and restoration of a deliberately
non-unit previous renderer scale. All pass (`work/p415/hidpi.log`). This directly
exercises the geometry/clip mismatch without pretending Linux dummy video is a
real Retina window. Both production sites call the same tested wrapper.

The fresh Linux launcher main-tab capture is **byte-identical** to the initial
P4.14 executable: SHA-256
`43a286bd39c12694ee182f1a59d1c4891ad5f5bc33491ea218fba78edcae2b48`.
Images: `work/p415/ui/before.png` and
`work/p415/ui/launcher-main/window.png`. All six launcher/features/Sharp/Smooth/CRT/actual SDL-event pause captures are
byte-identical to P4.14 (`work/p415/ui-equality.json`). The launcher was also
opened and inspected.

Windows keeps the existing embedded **PerMonitorV2**, `true/pm`, UTF-8 manifest.
Its ImGui SDL3 platform backend supplies the current window/pixel-size ratio;
the same wrapper handles 1x and fractional framebuffer scales. PE resource
verification passes (`work/p415/windows-resources.log`). Real mixed-DPI monitor
moves on Windows and the original Retina macOS window remain PM checks. No
real Mac/Windows desktop execution is claimed here.

## Profile first: method and limits

Host: Threadripper PRO 5975WX, 32 cores / 64 logical CPUs. Other work remained
running; no process was stopped, reprioritized or pinned. No GPU was used.
The final benchmark waits for this job's correctness/build jobs to finish,
then runs each resolution sequentially with before/after order reversed at 2x.
All runs use identical power-on input, `STV_AUTOPLAY=100`, 3,600 fields,
960x720 SDL dummy/software output, Sharp, frameskip **off**, `--unpaced`, and
`--stats 5`. Every requested field is composed or exactly reused and presented.
Pacing, field rate and guest scheduling are unchanged.

`perf stat -e instructions,cycles,task-clock true` and
`perf record -e cpu-clock -- true` **both fail** under
`perf_event_paranoid=4`, including software events. Errors are retained in
`work/p415/perf-unavailable.log`. Valgrind is absent, and its no-root package
download failed on DNS. No privilege/policy change was attempted. Consequently
**hardware instruction/cycle counts are unavailable**; this report explicitly
uses thread CPU milliseconds, not estimated cycles or invented instructions.
A permitted hardware-counter run remains an open verification item.

`SATURN_PROFILE=1` adds exclusive thread CPU-time and elapsed-time counters
around SH-2 execution, each side's synchronization, authoritative VDP1 draw,
enhanced VDP1 workers, VDP2 caller/workers, 68000+SCSP/DSP audio, and SDL
presentation. CPU time uses the thread clock, which excludes descheduling.
The category switches preserve attribution when the SH-2s share a coroutine
thread. Counters and clocks are host-only, outside guest snapshots. Optional
profiling still has overhead; the throughput table uses profiling **off**.

Before profiles were taken before optimisation at every factor. The baseline
already used **`-O3 -DNDEBUG`**; changing -O2 to -O3 was not a valid explanation.
First profile: 46.69 / 30.63 / 22.73 fields/s, with load rising approximately
46 to 65. By field 1200 at 1x, the master had used 14.60 CPU seconds waiting
for the slave's 14.02 seconds of work. VDP2 had used another 18.46 CPU seconds.
That evidence selected synchronization, the idle slave loop, and composition.
Audio and presentation were substantially smaller, and their arithmetic was
left unchanged.

CPU cost tables below are sums divided by identical field counts. A worker
row is the **sum** of its worker threads, not a wall-time critical path.
Before: separate master/slave host threads, up to seven extra compositor
threads and four enhanced raster workers. After: one host thread runs both
SH-2 contexts and caller work; compositor/raster workers remain separate.
The profiler aggregates band workers; individual worker instruction counts
and individual-band CPU imbalance were not obtainable from this profile.

### Unpaced software-window throughput

| Factor | Before fields/s | Final fields/s | Final combat range | CPU before → final | 1-minute load at end, before → final |
|---|---:|---:|---:|---:|---:|
| 1x | 38.48 | **168.01** | 160.67–171.53 | 228% → 180% | 74.3 → 41.0 |
| 2x | 29.66 | **82.59** | 71.24–89.72 | 239% → 283% | 64.5 → 45.5 |
| 3x | 27.10 | **40.52** | 31.17–46.13 | 277% → 303% | 45.4 → 48.6 |

All runs show **zero omitted pictures**. CPU 100% means one logical CPU.
Final 1x has substantial Linux headroom; every final 2x combat sample exceeds
60 fields/s. Final 3x remains below real time. These are **Linux** results,
not a claim of 60 at 2x on the i9 Mac.

Changing shared-host load materially affects wall time. The earlier paired
optimized controls were **97.63 / 51.13 / 48.25 fields/s**, at end loads
**85.6 / 77.3 / 39.3**. They use the same accepted execution/rendering code:
comparing the retained control and final ELF `.text` finds only **eight
consecutive differing bytes**, verified as the exact-build identity immediate
(`work/p415/accepted-code-equivalence.json`). Final load start/end triples
and every sample are in `work/p415/performance-summary.json`. These controls
expose variance; no entire wall-time ratio is claimed as a load-independent
causal speedup.

### Exclusive CPU milliseconds per field, 3600 identical fields

Each cell is **before → final**. Values are thread CPU time, not elapsed
critical-path time; parallel worker sums can exceed 16.7 ms while the picture
finishes faster. These separate runs use the same optional profiler.

| Component / host placement | 1x ms/field | 2x ms/field | 3x ms/field |
|---|---:|---:|---:|
| Master SH-2, main thread | 0.74 → 0.59 | 1.47 → 1.23 | 2.22 → 1.99 |
| Slave SH-2, slave thread → main-thread coroutine | 11.86 → 0.87 | 13.56 → 0.93 | 13.20 → 0.96 |
| Master side of handoff | 12.44 → 0.14 | 14.57 → 0.15 | 14.37 → 0.15 |
| Slave side of handoff | 3.47 → 0.14 | 4.25 → 0.15 | 4.36 → 0.15 |
| VDP1 draw + enhanced setup, caller | 1.66 → 1.62 | 2.25 → 1.87 | 2.13 → 1.93 |
| VDP1 enhanced raster, four workers summed | 0.00 → 0.00 | 12.23 → 10.03 | 24.37 → 22.10 |
| VDP2 caller / band 0 | 2.76 → 1.00 | 4.49 → 3.27 | 9.23 → 7.13 |
| VDP2 seven workers summed | 16.76 → 5.51 | 20.43 → 17.07 | 42.63 → 35.86 |
| 68000 + SCSP/DSP audio, caller | 1.70 → 1.58 | 1.92 → 1.65 | 1.90 → 1.68 |
| SDL presentation/events, caller | 0.74 → 0.67 | 0.96 → 0.84 | 1.26 → 1.13 |
| Other / pool bookkeeping | 0.00 → 0.08 | 0.00 → 0.09 | 0.00 → 0.10 |
| Total attributed CPU | **52.13 → 12.20** | **76.14 → 37.28** | **115.67 → 73.19** |

Total attributed CPU falls **76.6% / 51.0% / 36.7% at 1x / 2x / 3x**.
Before/after handoff counts are **586,331 in each direction**, and both runs
execute **1,934 authoritative draws** at every factor. Boot/reporting and
OS thread-management costs are not fully attributed; GNU time retains whole-
process user/system totals. CPU clocks remove descheduling, but CPU frequency
and memory contention still vary with host load. The audio/presentation rows
provide unchanged-work controls rather than invented optimisation gains.

Profiler-off throughput, profiler-on throughput, CPU/elapsed component costs,
call counts, environments and load triples are retained in
`work/p415/performance/{fair-before,accepted,accepted-profile}-*x/report.json`.
The rejected texture candidate is excluded from these tables.

## Optimisations and isolated evidence

1. **SH-2 coroutine handoff.** ST-V already executed only one CPU at a time.
   The existing portable minicoro implementation now preserves the suspended
   native slave stack on the master's host thread, at every original give/yield
   boundary. Saturn configurations keep their threaded path. There is no new
   CPU parallelism, changed slice size, early yield or interrupt delivery.
   In the matched first 1800 fields, synchronization CPU cost falls from
   **15.628 to 0.298 ms/field**, removing **98.1%** of that category. The slave's
   actual work remains about 10.7 ms/field before idle batching. The sequential
   short wall runs improved from 41.94/36.79/27.01 to 52.71/42.48/29.79 fields/s,
   but changing host load prevents assigning that entire wall difference to
   the handoff alone. All three factors pass the full before/after authority
   comparison (`work/p415/authority/before-vs-coroutine.json`).

2. **Empty slave loop, between polls only.** `port/p415_idle_loops.py` checks
   the SHA-256 of the entire generated `f_060051B0` function (cart 022011b0)
   before adding a helper at 060051EE. Unknown generated code fails the build;
   it is not patched heuristically. Linux, Mac and Windows wrappers run this
   pass after P4.10's continuation pass. The helper requires the original
   register constants, aligned known command-buffer bounds, equal producer /
   consumer pointers and a zero command. It coalesces full empty iterations
   strictly **before** the next original poll: each has four inner backedges,
   one outer backedge, a wrapping 32-bit counter increment and flag store.
   At least one original safe point remains; the original instructions then
   materialize the exact registers/locals/continuation at the next poll.
   Store-watch and trace modes disable batching. No polls, slave slices,
   master handoffs, safe-point time, commands or guest-visible counter ticks
   are removed. Master VBlank waits are left alone.
   Exhaustive tests compare all budgets 0..257 through the next poll, including
   counter wrap, exact context bytes, memory and poll phase. Work/trace/watch
   guards are tested. Slave CPU cost falls **10.729 to 1.247 ms/field (88.4%)**
   in matched short profiles. The full 2100-field before/after comparison
   passes at each factor, including all 22 state parts
   (`work/p415/authority/before-vs-idle.json`).

3. **Exact original-resolution composition reuse and pruning.** Existing
   P4.14 palette lookup, opaque-layer pruning, offset tables, row rotation
   terms and exact unchanged-frame caching now apply at 1x too. General
   colour-calculation/special-priority paths remain available. Hardware VDP1
   draw/erase/CPU writes, mode changes and state restore advance presentation
   revisions at 1x as well; full VRAM/CRAM/render-register comparisons remain
   exact. TVSTAT/HCNT/VCNT are still excluded because composition does not
   read them. Interpolation bypasses the whole-frame cache. Output is copied
   into the caller's Frame; every requested field is still presented.
   The first-1800-field VDP2 caller+workers CPU sum falls **19.757 to
   7.585 ms/field (61.6%)**. All existing renderer tests and a new 1x cache
   invalidation/restore/interpolation test pass. Full regression and fresh
   scene fixtures confirm unchanged original and enhanced pixels.

4. **Reuse sleeping compositor workers.** Seven native thread creations per
   composed picture are replaced by a synchronous reusable band pool. Each
   band gets its own mutable callback/line-colour state, as in the old copied
   thread callback. All finish before guest execution resumes. Tests exposed
   an initial shared-callback mistake; it was corrected before final evidence,
   with a repeated private-state test and the line-back-screen colour test.
   The short profile lowers VDP2 caller CPU from **1.527 to 1.177 ms/field**,
   but worker CPU/host-load variation cancels that in the total: **no isolated
   net-throughput gain is established on Linux** (122.49 versus 121.58 fields/s
   under overlapping correctness load). It is retained to eliminate native
   thread churn; its Mac benefit must be measured, not assumed.

5. **Exact power-of-two map arithmetic.** NBG/RBG maps have 1/2-page plane
   dimensions, 8/16-dot cells and power-of-two bitmap dimensions. Wrapped
   signed remainders plus negative correction become unsigned masks; plane /
   cell division becomes shifts. Negative floor coordinates retain the same
   Euclidean wrap. Rotation floating-point operations and sample locations
   are unchanged. Final renderer tests, all scene fixtures and exact authority
   comparisons pass. Matched first-1800-field VDP2 CPU sums change from
   **7.471 to 6.234 ms/field (16.6%)** after map masks. Host frequency/load
   also changes (audio is an unchanged-work control), so this is a measured
   component difference rather than an isolated instruction-count claim.

6. **Portable release ISA.** Linux/Windows x64 Release adds
   `-march=x86-64-v2`, with `SATURN_STV_X86_V2=OFF` for older x64 CPUs.
   macOS retains plain x86_64. There is **no AVX/AVX2 requirement**, fast-math,
   FMA-policy change or new LTO dependency. Short 1x profile CPU totals change
   only **13.69 to 13.42 ms/field**; this small gain is not isolated from load
   and frequency variation. The substantial gains come from the earlier
   changes. Build flags and build/pass sources join the state fingerprint.

**Rejected experiment: VDP1 texture-coordinate recurrence.** An exact
quotient/remainder stepper replaced per-pixel texture division and passed its
closed-form arithmetic oracle, all PNGs and the full guest-state comparison.
It did not produce a measured gain. At 2x, VDP1 caller+workers increased from
10.704 to 13.441 CPU ms/field; unchanged audio rose from 1.530 to 1.743 under
the changed host conditions. Even relative to that control, the raster cost
increased about 10%. The experiment was **removed**; final raster stepping and
Gouraud arithmetic remain original. Code/binary evidence is retained under
`work/p415/texture-experiment/`, and its rejected profiles are the
`final-profile-*` reports. They must not be confused with accepted final costs.

All runtime changes are in `port/runtime-patches/p415-*.patch`. No functional
SH-2 decompilation source or `tools/decomp` was changed. The generated change
is in the port's own recompile. No optimisation with a timing-visible result
was accepted.

## Exactness evidence

- **164 tests, zero failed**, repeated: `work/p415/tests.log` and
  `work/p415/tests-repeat.log`. This includes all P4.13/P4.14 tests and the
  idle-loop, original-cache and private-band-state tests.
- **56/56 reference PNGs**, all four guest/full traces and all four WAVs are
  byte-identical to the unchanged P4.11-corrected/P4.13/P4.14 oracle:
  `work/p415/regression-final.log`, `work/p415/regression/manifest.json`.
  Includes the **9000-field no-coin attract** and game-over/new-game schedules.
- **2100/2100 unmasked 2 MiB RAM and semantic records per factor** match the
  original executable; WAVs, hardware dumps at 1800/2100 and PNGs at
  1800/1900/2100 match. **All 22 machine-state payloads at VB2000 match byte
  for byte at each same factor**, including enhanced VDP1 buffers. Only the
  state-file header's build identity/checksum is outside that comparison;
  no RAM, device or payload normalization is used. Evidence:
  `work/p415/accepted-equality.log`,
  `work/p415/authority/before-vs-accepted-final.json`.
- Those before/after runs use a test-only `time()` interposer with a fixed
  RTC origin and independent power-on recordings. It is never linked/shipped,
  and build/input/state identity rejection is not bypassed. The helper is
  `port/p415_fixed_time.c`; metadata and initial BIOS state therefore match
  without editing saved payloads.
- Fresh P4.14 attract/fight/scene-9 floor PNGs at **all three factors**, plus
  offline ending composition at all three factors, are byte-identical:
  `work/p415/visual.log`, `work/p415/visual/manifest.json`. All four comparison
  sheets are also byte-identical (`work/p415/visual-sheet-equality.json`). Ending evidence
  remains the retained original device capture, not a new full-game route.
- Enhanced state checks at both factors pass **60 repeated rewind fields,
  850 power-on replay fields and 150 cold/embedded-state replay fields**,
  with exact pixels, unmasked RAM, semantic records and WAVs:
  `work/p415/state-accepted.log`, `work/p415/verify-short/state-equality.json`.
  Longer P4.13/P4.14 state histories remain historical evidence; no new full
  ending traversal or 200,000-field execution is claimed.
- The PM's **default paid-input** 2000-field smoke (no autoplay), shot 1900,
  retains the specified hashes below. This differs deliberately from the
  autoplay authority/performance schedule; their shot-1900 hashes are not
  interchangeable.

| Factor | shot-1900 MD5 |
|---|---|
| 1x | `9bd5386ea7439621c19674381086e8fd` |
| 2x | `8f1d9a1f01a2b77cd9d4f82b22b71b37` |
| 3x | `a48d6414e53a778c69cfe4fbe71ba423` |

## Build, packaging and reproduction

```bash
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p415/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p415/runtime-tests -j6
work/p415/runtime-tests/runtime_tests
port/p415_build_probes.sh
SDL_VIDEO_DRIVER=dummy work/p415/probes/p415_test_hidpi
python3 port/p415_verify_regression.py
python3 port/p415_verify_hires.py --mode state --short
python3 port/p415_verify_smoke.py
python3 port/p415_verify_visuals.py --part scenes
python3 port/p415_verify_visuals.py --part ending
python3 port/p415_verify_ui.py
port/p412_build_win.sh
python3 port/p415_verify_patches.py
```

`port/p415_capture_authority.py --binary BINARY --tag TAG` captures a retained
before/final binary pair; compare using
`port/p415_compare_authority.py BEFORE_TAG AFTER_TAG`. This needs the retained
baseline/evidence fixtures. `port/p415_measure.py --binary BINARY --tag TAG`
runs unpaced throughput; add `--profile` for host component counters. Exact
commands, loads, environments and every stats sample are retained in JSON.

Linux window build and Windows cross-build pass. The Windows-only P4.12 patch
was re-exported against P4.15, then applied only in isolated staging.
`work/p415/patch-validation.log` reconstructs **80 runtime files** and
**132 Windows runtime/port files**, byte for byte, zero fuzz/offsets.
Windows runtime tests also cross-compile (`work/win/p415-tests/runtime_tests.exe`);
execution is reserved for the PM. The package still has only SDL3/libc++/
libunwind plus ordinary Windows imports, and contains no ROMs. Icon/UTF-8/
PerMonitorV2 resource checks pass.

Final owner package: **`work/win/DieHardArcade-windows-x64.zip`**,
**5,551,643 bytes**, **16 entries**, ZIP integrity checked.
ZIP SHA-256: `7a9f023e5ffa5d0e98b610b2b8866d87162849e1df08e967758bd734ff8c12d3`.
Executable SHA-256: `371c5a2377154d8f181ea6c6b71607414314df4b2647b22e354f738e1d22006a`.
Linux executable, patches and package hashes are in
`work/p415/artifact-hashes.json`. `selftest` also links successfully with the
new hardware-free idle-helper stub (`work/p415/selftest-link.log`).

## Open issues and next steps

1. Rebuild with `port/p48_build_mac.sh` and the app bundler on the idle i9.
   Repeat the same 3600-field 1x/2x/3x real-window and dummy runs, frameskip off.
   Linux's increased headroom does not establish **Mac 2x/60**. Check real Retina
   launcher and in-game feature overlay, including a display-scale transition.
2. PM: run current Windows tests/smoke/UI under Wine and check real Windows
   mixed-DPI monitors. Current cross-build/resource/probe evidence cannot
   establish native picker/controller/speaker/display behavior.
3. Obtain permitted hardware instruction/cycle samples and individual worker
   breakdown on an idle host. The explicit counter-policy limitation here is
   not a successful `perf` profile. Use Mac Instruments for the i9 critical path.
4. Enhanced VDP1 raster plus VDP2 sampling still dominate larger factors.
   A separately validated GPU **presentation** renderer is the logical next
   larger project: retain CPU hardware buffers/guest timing, reproduce integer
   coverage, mesh, colour operations and nearest sampling, compare all output
   hashes, and never substitute a faster inexact image for this oracle. No GPU
   was accessed or implemented in this milestone. Profile enhanced per-pixel
   division/command traversal before another CPU-only rewrite.
5. Audio (68000/SCSP/DSP) is unchanged and measurable in the remaining budget;
   predecoding stable DSP instructions is a possible future exact optimisation.
   No cycle/timing, mixer gain, sample ordering or audio-queue shortcut was added.
6. Existing MAME fidelity gaps and unsupported renderer modes remain those of
   P4.11/P4.14. This milestone preserves their reference output; it does not
   claim improved emulation accuracy. Exact-build state/input files from older
   source/flag fingerprints remain intentionally incompatible.

## Ten-line summary

1. Fixed both launcher and in-game ImGui Retina draws; prior renderer scale is restored.
2. Linux UI is byte-identical; actual SDL target probes pass 1x/1.5x/2x geometry and clipping.
3. ST-V coroutine handoff removes spinning while preserving every guest scheduling boundary.
4. Fingerprinted empty-slave batching retains counter, flag, context, budget and poll phase.
5. Exact VDP2 caches/pruning and power-of-two arithmetic reduce composition cost.
6. Final Linux unpaced 1x/2x/3x is 168.01/82.59/40.52 fields/s, zero omitted pictures.
7. All 164 tests, 56 regression PNGs/four traces/four WAVs and all scene fixtures pass.
8. All requested smoke MD5s, 2100-field unmasked guest records and 22 state payloads match.
9. Windows cross-build/ZIP and zero-fuzz patch reconstruction pass; no commit or decomp edits.
10. Hardware counters are blocked; idle-i9 Mac, Retina desktop and Wine execution remain PM checks.


## macOS verification (P4.15)

2026-11-09, Intel Mac (i9-9880H, 8 cores / 16 logical, macOS 26.5.1, Apple clang, Homebrew
SDL3, built-in Retina LCD 3072x1920). The Mac copy was reset to `8951396` (previous local
changes backed up in `work/p415mac/local_changes_vs_1980322.diff` plus copies of the touched
files; the P4.14 runtime as `work/p415mac/saturn-recomp-runtime-p414.tgz`).
`p415-retina-launcher.patch` then `p415-performance.patch` applied to the Mac's P4.14 runtime
with zero fuzz/offset. The resulting `runtime/` tree is content-identical to P620's (121 files,
md5 per file, ignoring the old stray `video.cpp.orig`). No macOS-specific fix was needed.
Evidence: `work/p415mac/` (Mac copy only).

| Check | Result |
|---|---|
| `port/p48_build_mac.sh`, `port/p411_build_app.sh` | Both build, zero warnings; idle-loop pass applied (idempotent rerun = no change) |
| Runtime unit tests (clang, `SATURN_NO_SDL`) | **164 tests, 0 failed** |
| Smoke 2000 fields, shot 1900, dummy video/audio, 1x | **`9bd5386ea7439621c19674381086e8fd`**, P4.8 binary and bundled app binary |
| Same, `--internal-resolution 2` | **`8f1d9a1f01a2b77cd9d4f82b22b71b37`**, both binaries |
| Same, `--internal-resolution 3` | **`a48d6414e53a778c69cfe4fbe71ba423`**, both binaries, 0 FATAL |

App: `port/run/P411/Die Hard Arcade.app` is now **v1.04** (Info.plist 1.04/104, startup log
string `Die Hard Arcade 1.04`; ad-hoc signature verifies with `codesign -vv --deep`). The v1.03
app is kept unchanged as `port/run/P411/Die Hard Arcade v1.03.app` (v1.02 also still present).
The version bump is a local, uncommitted edit of `port/p411_app_mac.c` / `port/p411_bundle_mac.py`
(upstream still says 1.02).

### Performance (macOS)

Same method as P4.14 (`work/p415mac/mac_measure.py`: 3,600 fields, frameskip off, Sharp,
scale 3, autoplay 100, `--stats 5`; CPU% = user+sys / wall, 100% = one logical CPU; whole run
= 3,600 / wall including start-up; combat = 5 s samples ending at VB900 or later). Host idle
apart from this process (load average 2.4-4.9). "Real" = visible desktop window, SDL Metal.

| Resolution | Mode | Whole run fields/s | Combat fields/s (mean; range) | CPU | Omitted | P4.14 whole |
|---|---|---:|---:|---:|---:|---:|
| 1x | real, `--unpaced` | 95.54 | 93.49; 88.29-98.29 | 140% | 0 | - |
| 2x | real, `--unpaced` | 51.97 | 50.32; 43.03-57.68 | 276% | 0 | - |
| 3x | real, `--unpaced` | 29.24 | 27.73; 22.20-35.42 | 305% | 0 | - |
| 1x | real, paced | **59.69** | **59.91; 59.80-60.20** | 90% | 0 | 58.60 |
| 2x | real, paced | 50.41 | 49.45; 41.34-57.75 | 267% | 0 | 39.59 |
| 3x | real, paced | 28.69 | 27.05; 21.46-34.79 | 297% | 0 | 24.51 |
| 1x | dummy, `--unpaced` | 134.33 | 130.34; 123.96-137.87 | 185% | 0 | 60.51 |
| 2x | dummy, `--unpaced` | 58.40 | 56.05; 46.57-66.17 | 298% | 0 | 40.90 |
| 3x | dummy, `--unpaced` | 30.94 | 29.13; 22.62-37.36 | 314% | 0 | 24.94 |

**1x now holds a steady 60** in a real window (every 5 s sample 59.8-60.4, zero omitted
pictures) at 90% CPU, versus 290% and dips to 55 before; unpaced headroom is about 1.6x
(real) / 2.2x (dummy). **2x improves ~27% but still does not hold 60** (41-58 in combat,
dipping where the scene is busiest); 3x improves ~17% to ~28. Zero omitted pictures
everywhere (frameskip off, so slow fields are late, not dropped). New observation: at 1x the
real Metal window costs measurably more than dummy when unpaced (95 vs 134 fields/s, ~3 ms
per presented field); this does not matter paced (60 is held), and at 2x/3x the CPU raster
dominates (real is within ~5-10% of dummy). Enhanced resolution on this CPU remains
"CPU intensive"; the GPU presentation renderer (P4.15 next step 4) is what 2x/60 needs here.

### Retina launcher / in-game overlay

Real desktop window on the built-in Retina display (1920x1440 backbuffer for a 960x720
window, Metal), captured via `SATURN_LAUNCHER_SHOT` (frame 60) and, for the in-game overlay,
`SATURN_PLAYER_SHOT` driven by real SDL Escape/Down/Enter events (macOS port of the P4.13
driver using DYLD `__interpose`: `work/p415mac/ui-driver-mac.cpp`). Images in
`work/p415mac/ui/` were opened and inspected:

- `launcher-main-real-new.png` / `launcher-features-real-new.png`: **fixed**. The launcher fills
  the whole window at proper size; the right-hand Arcade controls column (P1/P2 key + pad) is
  fully visible; the Player features tab fills the width. Nothing is clipped.
  `launcher-*-real-v103.png` (v1.03 app) reproduce the old bug for comparison (half size,
  top-left quarter, right column missing).
- The bundled v1.04 app binary's launcher capture is byte-identical to the P4.8 binary's.
- `pause-real-new.png` (in-game Player features / training overlay at 3x): **fixed**, fills the
  window, all controls and labels visible, nothing clipped.

Cosmetic only: glyphs are rasterized at logical size and upscaled 2x, so text is slightly soft
compared to native Retina text (correct size, readable). Crisp text would need the font
atlas rasterized at the framebuffer scale. The overlay uses default (smaller) font sizing,
unlike the launcher's `wh/600` scaling; this is pre-existing and also true on Linux.
A desktop `screencapture` was attempted but refused ("could not create image from display",
no Screen Recording permission for this session); `SDL_RenderReadPixels` reads the same
backbuffer that is presented. A live display-scale transition (moving between a Retina and
a non-Retina display) was not tested; no external display is attached.

### Notes

- `port/p411_build_app.sh` runs `p410_continuations.py` but **not** `port/p415_idle_loops.py`.
  It shares `port/run/P41/recomp` with the P4.8 build, so it is fine after
  `p48_build_mac.sh` (as here), but an app built alone after regeneration would silently
  lack the idle-loop batching (correct, just slower). Suggested one-line fix: add
  `python3 port/p415_idle_loops.py port/run/P41/recomp` after the continuations line.
