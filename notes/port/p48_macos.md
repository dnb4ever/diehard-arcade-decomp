# P4.8: native Intel macOS v1.00

2026-11-08. Built on the supplied Intel i9 Mac: x86_64, eight cores,
2.3 GHz, 32 GB RAM, Darwin 25.5.0. Apple clang 21.0.0, CMake 4.4.5,
Ninja 1.13.2 and the **already installed Homebrew SDL3 3.4.12**.
The sandbox hides the CPU model string. No sudo, package download or
`brew install` was used. No git commit was made.

**Headless verification passes; actual desktop launch remains unverified.**
Cocoa cannot enumerate displays from this command environment and LaunchServices
rejects `open`. The launcher and window host do run with SDL's dummy video and
software renderer. The measured GPU/desktop result is therefore still open.

## Changes and build

The supplied P4.7 runtime and generated recompile compile with Apple clang
and libc++ without changes to guest execution, compiler flags, affinity or
thread synchronization. `_mm_pause` already has the right Intel header and
architecture guards; Linux uinput is already guarded. CMake's SDL3 imported
target selects the Homebrew dylib and Threads selects native pthreads.
No GCC extension or filename-case failure occurred in either target.

- `port/p41_build.py`: `--generate-only` emits the 1,152-function, 137,075-
  instruction recompile without first building a different host; `--headless`
  explicitly selects the null host even when SDL3 is installed. Existing
  default Linux invocation remains available.
- `port/p48_build_mac.sh`: native Release/Ninja x86_64 builds, separate
  headless/window directories, Homebrew prefix discovery (or `SDL3_PREFIX`),
  SDL required for the window build. Uses macOS's Bash 3.2; no `readlink -f`,
  Ubuntu package extraction, hardcoded Linux library name or user path.
- `port/main_stv.cpp`: `--launcher` and `--no-launcher`, reusing the runtime's
  existing ImGui/SDL window before any guest starts.
- `port/runtime-patches/p48-macos-arcade-launcher.patch`: optional media-path
  text plus an arcade launcher editing the actual P1/P2 keyboard, common
  gamepad, coin, TEST, SERVICE and pause bindings. Hides Saturn multitaps in
  arcade mode. The Saturn disc launcher retains its original layout.
- `port/play_mac.sh`: clears inherited `STV_*` harness variables, preselects
  both ROM paths and opens the launcher. Settings and screenshots stay in
  `port/run/P48/stv.ini` and `port/run/P48/shots`.
- `port/p48_bundle_mac.py` / `port/p48_app_mac.c`: build generates a native
  executable in `port/run/P48/Die Hard Arcade.app`,
  version 1.00. This is an in-tree development launcher: it uses the repository
  ROMs, executable and external Homebrew SDL3. It is neither standalone nor
  signed/notarized; keep it at its generated location. App log:
  `port/run/P48/app.log`.

From the repository root, with `cart.bin` and `port/diehard_main.bin` present
and the runtime patched through P4.8 (order in `port/runtime-patches/README.md`):

```bash
port/p48_build_mac.sh --headless
python3 port/p48_verify.py
port/p48_build_mac.sh
port/play_mac.sh
port/play_mac.sh --no-launcher --stats 5
port/play_mac.sh --fullscreen
open "port/run/P48/Die Hard Arcade.app"
```

`JOBS=4 port/p48_build_mac.sh` lowers compilation concurrency. To regenerate
C++ explicitly: `python3 port/p41_build.py --generate-only`, then rebuild.
The original Linux `port/p46_build_window.sh` and `port/play.sh` are unchanged.
Linux execution was unavailable here; portability of the added launcher was
checked by keeping platform APIs out of the patch and building both host modes.

Controls: arrows/Z/X/C, 1 START, 5 COIN for P1; R/F/D/G, A/S/Q,
2 START, 6 COIN for P2. Escape/P pauses; F11 or Alt+Enter toggles fullscreen;
F12 saves a screenshot. Gamepad defaults and runtime limits are described in
`notes/port/p46_realtime.md`. TEST still exits toward the unimplemented BIOS
menu. The launcher displays the selected ROM paths; it does not browse for ROMs.
Model-specific gamepad overrides still require editing the settings file.

## Verification method and reference limits

`port/p48_verify.py` reconstructs the documented no-coin 9,000 VB, paid
game-over/second-game 9,000 VB, combat 2,100 VB, legacy multi-cart combat
2,100 VB and P4.6 `0601FC1C` repro 7,600 VB schedules. It passes `--shots`,
clears unrelated `STV_*` variables, retains logs and writes PNG MD5/SHA256 and
a deterministic scene/coin/stop/sound trace SHA256 manifest. The four P4.5
routes request 56 PNGs; the seed repro requests another three.

**Cross-host PNG/trace hash equality with P4.5/P4.6 cannot be established
from this copy.** Neither requested note contains literal Linux hashes;
they refer to excluded `port/run/P45` and `port/run/P46` artifacts and scripts.
The 16 game-over capture times are explicitly reconstructed in the new
runner, rather than claimed as a recovered historical shot list. The combat
input and shots come from `notes/port/p42_floor_progression.md`.

This copy already contains P4.7, whose boot-memory, stack and input-phase
corrections intentionally changed P4.5/P4.6 timings and pixels. See
`notes/port/p47_mame_fidelity.md`, final regressions. The first no-coin scene
1/2 entries are VB 9/314 here versus 11/316 in P4.5. Before changing the
harness, the initial binary was built as
`port/run/P41/build-generated/saturn_stv`. Its no-coin PNGs at VB 300/600/900
match P4.8 byte for byte, and it already reports VB 9/314. These differences
therefore predate P4.8.

To compare another host, copy its compatible manifest into the output tree
and pass `--reference port/run/P48/linux-manifest.json` to the runner. Full
commands, overrides, hashes and durations are retained in each manifest.
Do not describe P4.7-vs-P4.5 differences as an OS regression.

## Headless results

All five schedules exited 0 with **59/59 PNGs and zero FATAL** in the null-host
build. Results, exact commands, every PNG MD5/SHA256 and trace SHA256 are
retained in `notes/port/p48_macos_hashes.json`; raw artifacts are under
`port/run/P48/headless`. Wall times include capture and startup, and are
validation timings, not isolated performance benchmarks.

| Route | VB | PNGs | Wall seconds |
|---|---:|---:|---:|
| No-coin attract | 9000 | 30 | 100.203 |
| Game over and second paid game | 9000 | 16 | 97.651 |
| Combat | 2100 | 5 | 23.297 |
| Legacy multi-cart combat | 2100 | 5 | 22.759 |
| Original P4.6 seed repro | 7600 | 3 | 88.719 |

| Event | Published P4.5/P4.6 VB | Mac P4.7-based build VB |
|---|---:|---:|
| No-coin scene 1 | 11 | 9 |
| No-coin scene 2 | 316 | 314 |
| No-coin scene 3 | 2154 | 2152 |
| No-coin attract returns | 8872 | 8870 |
| Paid idle CONTINUE | 3615 | 4033 |
| Paid idle GAME OVER | 4893 | 5311 |
| Paid idle attract returns | 5195 | 5613 |
| Second game playing / scene 3 | 6120 / 6926 | 6120 / 6926 |
| Combat playing / scene 3 | 360 / 1166 | 360 / 1166 |
| Legacy combat scene 3 | 1179 | 1179 |

The P4.7 Linux note explicitly records the new 8870, 4033, 5311 and 5613
values; all four match here. The first three pre-P4.8/P4.8 PNG MD5s are
`6c9a79530a32671a6795236090053b3e` (300),
`0f052098cec0918e31572c4e94abfa2f` (600), and
`48c8e9ccba4129f8b18759be29e218b3` (900).

The original 7,600-VB P4.6 seed schedule now stops in scene 6 and does not
exercise `0601FC1C`, because P4.7 changes the scripted fight trajectory.
An additional **8,500-VB retimed run** retains the original hold and adds
UP at 7850 for 500 fields. It reaches scene 8 at VB **7867**, scene 9 at
**8164**, exits 0, produces all three shots, and `cov.txt` contains both
**0601FB40 and 0601FC1C**. No game state was forced. Reproduce with:

```bash
python3 port/p48_verify.py --label retimed --schedules fc1c-retimed
```

Runtime unit tests: **131 tests, 0 failed**:

```bash
cmake -S tools/saturn-recomp/runtime/tests -B port/run/P48/runtime-tests -G Ninja -DSATURN_NO_SDL=ON
cmake --build port/run/P48/runtime-tests -j8
port/run/P48/runtime-tests/runtime_tests
```

The P4.8 patch also reconstructs its two runtime source files byte for byte
from the pre-P4.8 snapshots; reverse dry-run succeeds. Shell syntax, Python
compilation, plist validation and repository whitespace checks pass.

The **SDL binary run headless also exits 0 on all five schedules**. All
**59 PNG MD5/SHA256 values and five trace SHA256 values match the null host**.
Log: `port/run/P48/verify-sdl-headless.log` (`Comparison: all hashes match`).
Reproduce after the null-host run:

```bash
python3 port/p48_verify.py --binary port/run/P48/build-window/saturn_stv \
  --label sdl-headless --reference port/run/P48/headless/manifest.json
```

## Launcher and window checks

The ImGui launcher was rendered and visually inspected at 960x720, with both
ROM paths, P1/P2 keys, gamepad bindings, coin/system keys and display/volume
controls visible (`port/run/P48/launcher-dummy.png`). The native app executable
also ran the game for 180 fields and exited 0 when invoked directly under
SDL's dummy video/audio drivers. Its stats settled to 59.84 VBlanks/s with
zero pictures skipped (`port/run/P48/app.log`).

**A real desktop window could not be confirmed in this command environment.**
The default Cocoa driver logs unavailable `com.apple.hiservices-xpcservice`
connections and `SDL: The video driver did not add any displays`; a direct
`--window` run exits **3** (`port/run/P48/window-cocoa.log`). LaunchServices
`open` rejects the bundle with **-10827 / kLSNoExecutableErr**, even though
`file` confirms its executable is x86_64 Mach-O, the plist passes `plutil`
and direct invocation works. The rejection persisted with a native wrapper.
These are observed environment failures; double-click launch remains unverified.
No approval escalation or OS permission change was attempted.

SDL's `offscreen` driver also fails here while initializing OpenGL; the
`dummy` driver with the software renderer successfully exercises window,
launcher, composition, pacing and audio-stream code without desktop access:

```bash
SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIO_DRIVER=dummy \
  SATURN_LAUNCHER_SHOT=port/run/P48/launcher-dummy.png \
  port/play_mac.sh --settings port/run/P48/dummy.ini
```

This captures four launcher frames and closes using the runtime's existing
test option. Physical keyboard/gamepad remapping, real fullscreen, actual
speaker output and the GPU renderer require a normal desktop terminal.
The launcher's Start interaction was not automated here.

## Speed measurement commands

The performance runs execute sequentially, after the regressions and builds
have finished. They use the scripted player from VB 100, reaching scene 3
at VB 687 and scene 5 at VB 2171 with this P4.7 boot. The uncapped window
composes every field; the paced run retains the default frameskip setting.
Reference raster speed is **59.826 VBlanks/s** (263 x 63,556 ns).
Other `STV_*` overrides were cleared before these commands:

```bash
mkdir -p port/run/P48/perf-headless port/run/P48/perf-unpaced port/run/P48/perf-paced
STV_AUTOPLAY=100 port/run/P48/build-headless/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 6500 \
  --stvinput 99999:COIN --settings port/run/P48/benchmark.ini --out port/run/P48/perf-headless

SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIO_DRIVER=dummy STV_AUTOPLAY=100 \
  port/run/P48/build-window/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --window --unpaced --stats 5 --vblanks 6500 --stvinput 99999:COIN \
  --settings port/run/P48/benchmark.ini --out port/run/P48/perf-unpaced

SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIO_DRIVER=dummy STV_AUTOPLAY=100 \
  port/run/P48/build-window/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --window --stats 5 --vblanks 3000 --stvinput 99999:COIN \
  --settings port/run/P48/benchmark.ini --out port/run/P48/perf-paced
```

Host wall times and exact argv are in `port/run/P48/performance.json`; each
mode's `run.log` contains the sound report and the window modes' `--stats 5`
samples. Dummy audio exercises the queue, but does not measure speaker latency.

| Mode | VB | Wall seconds | VB/s | Relative to real time |
|---|---:|---:|---:|---:|
| Headless, emulator only | 6500 | 72.383 | **89.80** | **1.501x** |
| SDL dummy video, software renderer, every field, unpaced | 6500 | 101.753 | **63.88** | **1.068x** |
| Same software host, paced, default frameskip | 3000 | 50.155 | **59.814** | **1.000x** |

The emulator has about **50% real-time headroom** on this i9. Composing and
presenting every field through SDL software leaves about **7% average
headroom** in this test. These are CPU/software-host results, not a measured
Cocoa/Metal desktop result. Unpaced Scene 5 `--stats 5` intervals range from
**59.10 to 66.16 VB/s**. The two 6,500-field runs have identical guest stop
state and sound counts: **4,786,934 samples, 4,526,098 non-silent**, final
68000 PC `000192`.

Paced `--stats 5`: **59.81–60.21 VB/s**, with **two pictures skipped through
VB 2706** (the last complete interval); the remainder has no periodic summary.
The audio queue ranges from **34 to 250 ms**, and stays mostly 217–250 ms
after startup. There are **zero underruns**, but **16,063 chunks dropped** at
the cap by the end of the paced run. The unpaced run reports one underrun
and 101,544 dropped chunks. This dummy-device behavior does not establish
correct audible synchronization. It must be checked on CoreAudio at a real
desktop; the P4.6 audio lead/cap concern remains open.

## Remaining issues

- Confirm actual desktop window, launcher Start/remapping, fullscreen, physical
  pads and speaker audio from a normal desktop terminal with
  `port/play_mac.sh --no-launcher --stats 5`; then try `port/play_mac.sh` and
  double-click the app. Cocoa and LaunchServices could not be verified here.
- Obtain the Linux PNG/trace artifacts or hash manifest for a literal cross-OS
  comparison. The recorded historical event differences match P4.7; the
  missing P4.5/P4.6 hashes cannot be reconstructed from prose.
- The app requires this repository layout and the installed Homebrew SDL3.
  A distributable app would need ROM selection, copied dylibs/rpaths,
  deployment-target testing and signing/notarization.
- Existing BIOS menu/backup/save-state and MAME fidelity limitations remain;
  this milestone changes no emulation behavior. The original deleted runtime
  patches and unrelated worktree changes were preserved; only the required
  patch README was restored and updated. Historical patches can be recovered
  from repository history for a fresh runtime reconstruction.
