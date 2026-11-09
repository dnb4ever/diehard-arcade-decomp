# P4.13 player features

Implemented 2026-11-08/09; execution/performance limits are explicit below. Paths are repository-relative. No commit.
Changes are confined to `port/`, the shared runtime, `notes/port/` and `work/`;
`port/converge/` and `work/converge/` are excluded.

## 1. Rewind

Enable **Rewind history** (0..20 seconds) in the launcher or pause menu's
**Player features / training**. Default is **0 (off)**. Hold Backspace or a pad's
left shoulder; release resumes the saved continuation. `[player] rewind_key`
and `rewind_pad` accept SDL key/button names, editable in both menus (Enter
commits text). `--rewind-seconds 20` also enables history headlessly.

History uses P4.10's full-machine state parts at the same safe continuation
boundary, with one additional player-feature part. The ring keeps changed 256-byte XOR blocks, reverses them to restore
previous snapshots and drops the abandoned future on resume. It retains at most
600 snapshots, sampled every other field (about 20 s at 59.826 Hz), with a 512 MiB delta limit. Very busy
content can shorten history at that limit. The rewind position has two-field granularity; the headless `--rewind-frames`
probe rounds odd counts upward. No state files are written. Audio is
cleared during restore; rewind is silent. Ordinary quick load clears history.

Verification commands: `python3 port/p413_verify_features.py rewind` and
`python3 port/p413_verify_features.py speed`. `work/p413/verify/rewind-equality.json` confirms **300/300 fields, VB1501..1800,
bit-identical pixels, full 2 MiB RAM and semantic records** after rewinding from
VB1800 to VB1500. Two independent runs pass (56.810 and 74.238 s including
per-field composition/hashing). The latter includes allocation reuse. Capture
cost before the final poll-cache optimization is 1.259 ms/snapshot, about
0.075 of the 16.715 ms field budget. Final measurements are appended below.

The host became saturated during benchmarking (load average 66 on 64 logical
CPUs; sampled per-CPU use 78..100%). Even feature-off software presentation
fell to 45..57 fields/s. A final short paced run reached 59.77..60.18 fields/s,
but the longer scene-5 run fell below real time as host load rose again.
Sustained full-speed performance is consequently **not yet established**.
The implementation caches control
checks once per captured field and allows up to five omitted pictures when
history is enabled; ordinary feature-off frameskip remains unchanged.
The ring budget counts allocated delta capacity, rather than only used bytes.

`p413-rewind.patch` is incremental against the final P4.11 runtime. SDL key
events exercised a 30-field hold/release and the pause-menu feature dialog;
`work/p413/rewind-ui.log` exits normally with no audio empty/drop/resync events.
Inspected `work/p413/pause.png` and `player-menu.png`.

Launcher was rendered with SDL dummy/software and `SATURN_LAUNCHER_SHOT`;
`work/p413/rewind-launcher.png` was visually inspected. All rewind controls fit
at 960x720. Physical pad/speaker/desktop behavior is not verified here.

## 2. Fast-forward and turbo

Hold **Tab**, toggle with **F4**, or use **Fast forward (mute audio)** in the
launcher's **Player features / training** tab or the same pause-menu panel.
The key names are remappable as `[player] fast_forward_key` and
`fast_forward_toggle`. Fast-forward removes host pacing, presents one in four
fields, mutes host playback, and clears/rebases audio on both transitions.
Guest raster timing, synthesized samples and WAV recording remain unchanged.
Speed is limited by CPU headroom; there is no fixed multiple guarantee.

Each player's **A/B/C** has an independent 0..30 Hz autofire slider in both
menus. Zero disables it; positive values gate an already-held button with a
50% duty cycle at nominal 60 fields/s (actual frequency scales by 59.826/60).
It works for keyboard, pads and headless panel timelines. It leaves directions,
coins, start, service and unheld buttons alone. Phase uses emulated VBlank
number, so quick load/rewind reproduce it without a host-clock dependency.
Persistent keys are `turbo_p1_a` through `turbo_p2_c` in `[player]`.

`p413-speed-turbo.patch` follows `p413-rewind.patch`. Runtime tests pass:
**145 tests, 0 failed** (`work/p413/tests.log`). Tests count exactly **10 and
15 rising edges** for independently held P1 A / P2 C over 60 fields, each held
30 fields, reject out-of-range rates, and check both raw words after real
headless BIOS scans for 60 fields. Defaults preserve every panel bit for
120 fields. Real SDL events produce **two on/off pairs** (Tab hold/release,
then F4 toggle) in `work/p413/forward-ui.log`; the run reaches VB400 with
**zero audio empty/drop/resync events**. The enabled-from-start muted run
also exits normally. Inspected `work/p413/turbo-launcher.png`,
`turbo-player.png` and `ff-hud.png` (SDL dummy/software). The separate launcher
tab keeps all feature controls visible at 960x720.

Earlier every-field rewind prototype: **300/300** repeated fields remain identical after the
allocation/poll-cache changes (`work/p413/rewind-verify-final.log`, 67.982 s).
It retained all **1200 fields**, peak allocated delta/image capacity
**421,517,773 bytes (402.0 MiB)**, averaging **1.137 ms/capture**.
Paced 1800-field comparison during saturation: feature off **40.040 s**,
history on **40.920 s**, **2.2%** additional wall time; the feature-off run
itself misses real time. This prototype did not establish the requested full-speed
software presentation. The final implementation samples **every other field**
(30 snapshots/s), halving capture frequency to fit the measured frame budget.
Final cadence measurements, including the short real-time pass and longer
loaded run, are appended below. A less loaded CPU/desktop measurement remains
necessary to establish sustained full-speed presentation.

## 3. Training cheats

Launcher: **Player features / training → Training / cheats**. Pause: **Player
features / training → Training / cheats**. All defaults are off. Health, lives
and credits take effect on resume and affect both joined players. **Disable
all cheats** clears persistent training options. Scene selection applies at the
next power-on; it accepts 2..72, with 0 preserving the original route. Scene 2
is the intro, 3 the first fight. Selected scenes use the game's loaders and
scene task lifecycle; no live scene-index/pointer teleport is performed.

Active health/lives/credit cheats appear in orange as **CHEATS: ...** in the
HUD and in the actual SDL window title. A run started with scene select keeps
a SCENE mark even if its next-boot preference is subsequently disabled.
Headless `--shots` stay raw game pictures; host captures include the marker.
Keys are `[training] infinite_health`, `infinite_lives`, `infinite_credits`,
`start_scene`. The same INI works headlessly with `--settings FILE`.

RAM evidence, read from `project/src_functional/`, `notes/symbols.txt`,
`dh_names.h` and `dh_work.h` (no decomp files edited). Addresses and
unqualified actor/task offsets are hexadecimal; scene selector offsets are
explicitly labelled decimal:

| Variable | Address / layout | Evidence and implementation |
|---|---|---|
| Player task / actor | `060995B0[slot]`, task `+8`, actor slot byte `+34`; slots 1/2 | `02228168.c` (`player_join_spawn`) uses the same chain. `dh_work.h` gives the task/actor layouts. Require joined state `06098674+p == 2`, valid in-RAM task/actor and matching slot ID; enemies and attract actors are excluded. |
| Health | signed word actor `+56`, seed words `06099788/+2` | `0222ab18.c` (`actor_apply_damage`) subtracts/clamps this word. `02228168.c`, `02229c00.c` seed it from vitality `06099794`, doubled for pose set `0609979A == 1`. Hold that original maximum (114 at factory settings), preserving the selected vitality. |
| Lives | words `0609978C/+2`, initial setting `06099796` | `02228168.c` seeds the counters; `02229c00.c` decrements and selects re-arm versus final commit. Hold 9 while a joined player is active. |
| Credits | BIOS coin assignment `06000794 = 1901` | MAME-measured P4.9 free-play preset 25; existing runtime BIOS `660` reports it and `664` accepts starts without consuming credits. `0220f9d4.c` calls `664`; `022045f8.c` consumes the result and initializes the player counters. The BIOS owns the bank; there is no independent cart credit counter to freeze. The cheat writes this modeled BIOS RAM setting, restores the previous assignment on disable and snapshots that restoration state. |
| Scene / stage | index `06098668`, descriptor pointer `0609866C`; table `06072660`, stride `50` hex | `dh_names.h`, `02205b20.c` (`scene_advance_task`) establish the table and branch selector. On power-on, scene 0's data bytes `+43/+44` (decimal offsets) become selector 0 / target N. The original director constructs the index/pointer and runs cleanup/loaders. `02205c96.c` also exposes the original game's own 0..81 scene-check selector; ending/title scenes are intentionally excluded from this training range. |
| Timers (identified, unchanged) | frame tick `060987BC`; player accept timers `0609975C/+4`; BIOS countdown `06000654 & 7F` | `02223c74.c` increments `g_frame_tick`; `0220581A.c`, `022045f8.c`, `02228168.c` seed the accept timers to 30. P4.5/P4.7 notes distinguish BIOS countdown from these game counters. No timer freeze is exposed. |

**Invincibility is deferred because no verified universal RAM flag was found.**
The experiment holding actor `+4E = 90` reproduced the original respawn blink
and passed its held-value check for 1413 fields, but HP still fell **114 → 78**.
`022453DE.c` excludes nonzero `+4E` in one enemy targeting path;
`0222863E.c` decrements/blinks it, while other damage paths bypass it. The
candidate was removed from config/UI/runtime rather than offered as working
invincibility. Evidence remains in `work/p413/training/invulnerable/`.
The optional hitbox display is also deferred: spatial buckets in
`0222EEE4.c` are established, but a complete screen projection and attack/hurt
volume interpretation is not. No guessed rectangles are presented as hitboxes.

Verification: `python3 port/p413_verify_training.py`; logs, commands, samples
and PNGs in `work/p413/training/`. Health and lives each pass **1413 held-value
samples** through VB2100: HP **114**, lives **9**. Normal autoplay falls to
HP78; the health screenshot retains a full green bar. Credits start P1 with
**zero coins** and reach joined state 2; `06000794` remains `1901`.
Selected scene **9** starts at VB634 and supports joined combat through VB1200;
scenes 28 and 72 are also checked for entry/loading and screenshots (not a
full alternate-stage completion claim). A separate no-movement natural-death comparison runs **6000 fields** each:
normal paid play reaches GAME OVER at **VB4302**, while infinite lives keeps
P1 joined in scene 3 through **VB6000** (HP54, lives9). The VB4500 screenshots
show GAME OVER versus continuing combat. Logs/commands/timings (93.793 and
94.135 s) are in `work/p413/training/lives-effect.json` and `death-*`.
The final runtime suite passes **147 tests, 0 failed**; it covers RAM bounds,
joined-player exclusion, credit assignment restoration across a snapshot and
scene-selector data without a live index/handler change.

Inspected `work/p413/training/health/shot-1800.png`,
`lives/shot-1800.png`, `training-hud.png`, `training-launcher.png` and
`training-edited.png`. SDL mouse events toggle all three actual launcher
checkboxes and reload as true in `work/p413/training-edited.ini`. The pause
panel `work/p413/training-pause.png` was also inspected; `training-ui.log`
reads the actual SDL title back as `Die Hard Arcade [CHEATS: HP LIVES CREDITS SCENE]`. `p413-training.patch` follows the speed/turbo
patch; the quick snapshot adds a small `player` part for reversible BIOS
assignment and the scene-use marker. The P4.10 verifier understands that part.

A two-player, no-coin scene-9 check also passes: **555 P1** and **551 P2**
samples at HP114/lives9 through VB1200. Both HUDs/players are visible in
`work/p413/training/two-player/shot-1100.png`, visually inspected. The report
is `work/p413/training/two-player/report.json`.

## 4. Input recording/replay

Launcher: **Player features / training → Input files**. Enter a path and press
Enter; **Record from power-on**, **Record from quick save**, and **Play input
file** start play directly. The save option is enabled when the quick slot
exists. Default file is the quick-slot path plus `.inputs` (the same persistent
data folder). `[player] movie_path` remembers another path. Quit normally to
finish/write a recording. Pause shows current input-file status; the game HUD
also marks recording/replay. The launcher screenshot is
`work/p413/movie-launcher.png`, visually inspected under SDL dummy/software.
Real SDL mouse clicks start Record and Play, each to VB120, with **zero audio
empty/drop/resync events** (`movie-ui-record.log`, `movie-ui-play.log`).

Headless examples:

```bash
port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --record-inputs work/p413/run.inputs --vblanks 2100 --out work/p413
port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --replay-inputs work/p413/run.inputs --out work/p413
# Add --load-state --quick-state FILE when starting a recording from a save.
python3 port/p413_verify_inputs.py
```

**STVINP13 v1** has a 64-byte little-endian header: magic, version, build/ROM
identity, starting VBlank, embedded-state length, record count, FNV-1a payload
checksum and metadata length. Metadata contains training options, scene choice,
boot mode, a fixed virtual RTC origin and the initial 16 KiB BIOS work area.
Each field costs **6 bytes** (P1 raw, P2 raw, system switches). No ROM images,
host pointers or native stacks are stored. From a save, the file embeds the
P4.10 machine parts, so replay needs no external quick slot. Both variants
require the exact source/compiler/ABI/ROM fingerprint; cross-build input-file
compatibility is not claimed. Bounds, named parts and checksum are validated.

Replay overrides final physical panel pins after turbo and ignores live game
controls/autoplay. Host pause/fast-forward remain usable. Rewind truncates the
recording's abandoned future; quick load is blocked while either input mode is
active, preventing an unrecorded state jump. Training options are fixed while
recording/replaying and show the effective recorded flags; choose them before
starting. This prevents unrecorded RAM modifications. Real SDL mouse attempts
leave the INI and CHEATS title unchanged; inspected the disabled panel in
`work/p413/locked-training.png` (`locked-training.log`). Replay stops at its recorded end.
A recording interrupted by a crash or SIGINT has no final input stream; use
normal window Quit/close or a headless VBlank limit. Record paths are replaced
atomically on successful finish. An unavailable/unwritable/stale/corrupt file
is rejected (exit 2; a windowed run also shows an error dialog).

Initial full check: **2100/2100 power-on fields** and **600/600 embedded-state
fields** match bit-for-bit in pixels, unmasked 2 MiB RAM and semantic records.
The state origin is VB1500; replay matches VB1501..2100. Wrong version, identity,
truncation and checksum damage are **4/4 rejected**. Commands, hashes, timing and
file sizes are in `work/p413/inputs/report.json`; initial log
`work/p413/inputs-verify.log`. The fixed-RTC repeat also passes all 2700 field comparisons and 4 rejections.
The next repeat compares complete WAV files as well: power-on and state-origin
PCM are byte-identical (`work/p413/inputs-pcm-final.log`). Files are **29,080 bytes**
for 2100 power-on fields and **4,778,831 bytes** for the embedded state plus 600
fields. A replay without explicit `--vblanks` uses the full file length; a separate
**705-field** file/replay confirms it passes the old 600-field harness default
and keeps the VB700 PNG identical (`work/p413/default-limit-*`). Recording
from an unavailable/stale requested save is rejected rather than silently
recording a fresh boot. Runtime patch:
`p413-input-files.patch`, after training.


## 5. Windows parity

Rebuild/package: `JOBS=6 port/p412_build_win.sh`. It now refreshes the isolated
snapshot from current shared sources when inputs change, using
`port/p413_stage_win.py` and `work/win/p413-snapshot.json`. The historical
`work/win/baseline/` and original `snapshot.json` remain intact. The current
unpatched baseline is `work/win/p413-baseline/`; staging remains
`work/win/saturn-recomp/`. The build never patches the shared runtime.

The refreshed `p412-windows-portability.patch` is incremental against the
**current P4.13** runtime and harness, applied **only inside Windows staging**.
It still covers runtime resource/manifest/includes and the Windows harness/
CMake glue. `--force` on the reverse dry-run prevents GNU patch from auto-
reversing its direction and falsely reporting an unpatched snapshot as patched.

Double-click the extracted `DieHardArcade.exe` or `play.bat`. Supply the owner's
MAME **diehard (USA)** set as `diehard.zip`, five loose members, or a verified
prepared `cart.bin`. The program slice is derived automatically. The app
searches its AppData cache, a zip/loose set dropped in AppData, its executable
folder and a remembered source; a missing-set dialog explains the files and
opens the native Windows folder picker. `--choose-roms` chooses another source;
`--rom-dir FOLDER_OR_ZIP` selects one explicitly. Sources containing spaces and
Unicode use wide Windows file APIs in the shared converter.

Prepared images, `stv.ini`, the quick slot, input-file default, screenshots and
`app.log` live under **`%APPDATA%\Die Hard Arcade`**. A selected source folder
is remembered as `rom-folder.txt`. The distribution folder is read-only at play
time. Explicit `--cart` retains the original headless harness behavior.

Portable conversion lives in `port/p413_roms.c` / `.h`, using the exact P4.11
mapping and supported cart/program SHA-256 values. Both the Mac app wrapper
and Windows app call it. It contains a bounded stored/deflate ZIP reader,
SHA-256 verification and atomic output replacement; it launches no external
unzip/Python/PowerShell process. No new third-party DLL is required. The Mac
bundler compiles the shared C file beside `p411_app_mac.c`; its existing
Application Support path and dialogs remain. Linux C11 and Windows/UCRT C
compilation pass. Mac compilation is reserved for the other agent.

`python3 port/p413_verify_roms.py` passes **72 zlib-oracle inflater cases**,
**132 truncated/capacity cases**, **3 SHA-256 vectors**, and actual zip,
folder-of-zip, loose, prepared, overwrite, bad-member and truncated-archive
cases. Each valid conversion equals repository `cart.bin` and
`port/diehard_main.bin` byte for byte; corrupt input writes no prepared output.
Report: `work/p413/roms/report.json`. The Windows application and runtime-test
executables cross-build successfully with the pinned P4.12 toolchain/SDL.
One test-only C++ narrowing initializer was made explicit for Clang.

**Wine execution is blocked by this session's sandbox.** Portable Wine
`work/win/opt/wine-11.21-amd64-wow64/bin/wine` fails before loading the program:
`wineserver: bind: Operation not permitted` (`work/p413/wine-headless-init.log`).
Private Xvfb also cannot bind its Unix display socket
(`work/p413/xvfb-fixed.log`). `ptrace` is denied too. No policy change,
escalation, external process kill or reuse of another session's server was
attempted. Consequently **no current Windows/Linux smoke-MD5 equality,
Windows runtime-test execution, native picker execution, or Windows launcher
PNG is claimed**. P4.12 Wine results apply to the older build only.

`port/p413_verify_windows.py` contains the full pending workflow: private
prefix/Xvfb, in-app zip preparation from a Unicode source folder, launcher
captures, AppData settings/saves, fresh Linux versus Windows 2000-field smoke,
quick overwrite/cold load, input replay and Windows runtime tests. Run it in
an environment permitting local listener sockets. The refreshed owner package
is `work/win/DieHardArcade-windows-x64.zip`; final sizes/hashes follow below.


## 6. Original app icon

`port/p413_make_icons.py` draws an original geometric skyline inside a shield,
with a navy/teal palette and amber disk/stripe. It uses no Sega/Die Hard logos,
game images, lettering or third-party artwork. Generated assets:
`port/assets/p413-icon.png`, `p413-icon.icns`, `p413-icon.ico`.
The 1024px preview was visually inspected.

The ICNS has **11 representations** (16..1024px plus Retina sizes). The Mac
bundler copies it to `Contents/Resources/DieHardArcade.icns`, sets
`CFBundleIconFile`, and updates the app version to **1.02 / 102**. Checked-in
assets are copied directly; packaging does not require Pillow. Pillow is only
needed to regenerate/verify the art. The app wrapper's log version is 1.02.

The ICO has **7 sizes: 16, 24, 32, 48, 64, 128, 256px**. The shared harness CMake
adds the icon resource under `WIN32`; the staging copier includes the assets.
`python3 port/p413_verify_icons.py` parses the packaged PE resources and confirms
all seven embedded icon PNG payloads equal the ICO originals byte for byte,
with the UTF-8/DPI manifest also present. It validates every ICNS PNG and size.
Report: `work/p413/icon-resources.json`; ICNS **223,167 bytes**, ICO **45,136 bytes**
(use the report for exact generated sizes). Finder/taskbar appearance and a
macOS app build cannot be exercised in this Linux sandbox.


## Final validation and open issues

The implementation is complete, with two required validation items still
unresolved in this environment: sustained full-speed presentation and current
Wine execution. These are **not reported as passes**.

| Check | Result / evidence |
|---|---|
| Linux window build | `port/p46_build_window.sh` passes; `work/p413/final-linux-build.log` |
| Runtime unit tests | **147 tests, 0 failed**; `work/p413/tests-final.log` |
| P4.11 default route | **56/56 PNGs**, all **4 guest traces, 4 full traces and 4 WAVs identical** to `work/p411/regression/after/manifest.json`; `work/p413/regression/manifest.json` / `regression.log`. Includes 9000-field no-coin and game-over/new-game routes, combat and legacy combat; zero FATAL. |
| Ring rewind | **300/300** replayed fields, pixels/full RAM/semantics equal at VB1501..1800, with final 30 Hz sampling; `work/p413/rewind-30hz.log` / `verify/rewind-equality.json` |
| Existing quick-state feature | **300 warm + 500 cold** fields, full RAM/semantics and PCM identical; **4/4** invalid states rejected; `work/p413/quick-states-final.log` / `quick-states-final/report.json` |
| Input files | **2100 power-on + 600 embedded-state** fields, full RAM/semantics and complete WAVs identical; **4/4** invalid files rejected. `work/p413/inputs-final.log` / `inputs/report.json`; 705-field default-limit check and SDL launcher Record/Play checks also pass. |
| Patch reconstruction | **75 runtime files** and **126 Windows runtime/port files** reconstruct byte for byte, **zero fuzz and zero offset**; `work/p413/patch-validation.log` |
| Shared ROM converter | 72 inflater oracle cases, 132 error/capacity cases, 3 SHA vectors, actual ROM-set variants all pass; `roms-verify.log` |
| Windows build/package | Final app and runtime-test target cross-build; exact icon resources/imports/ZIP integrity checked. Runtime execution blocked before program load by sandbox socket policy. |
| macOS | Shared C compiles as C11 on Linux and Windows; Python/Bash syntax checks pass. A macOS build, ad-hoc bundle signature verification and desktop launch remain for the Mac agent. |

Final memory/frame-cost benchmark: `python3 port/p413_measure_rewind.py`,
`work/p413/performance/report.json`. Each run executes 1800 fields of the same
scripted combat. GNU time measures process peak RSS; capture timing measures
serialization plus delta construction. These sequential runs share a heavily
loaded CPU host and are not isolated performance claims.

| Mode | Wall s | Fields/s | Peak RSS MiB | Capture ms / snapshot |
| Headless, off | 37.65 | 47.81 | 37.1 | 0.000 |
| Headless, 20 s history | 39.21 | 45.91 | 197.4 | 1.198 |
| SDL dummy/software, off | 39.15 | 45.98 | 43.8 | 0.000 |
| SDL dummy/software, 20 s history | 36.12 | 49.83 | 205.5 | 1.210 |

The final history retains **600 snapshots / about 1200 fields (20 s)**.
Peak allocated delta/image capacity is **263,969,759 bytes (251.7 MiB)**;
resident overhead is **160.3 MiB headless**, **161.7 MiB windowed**. Capture
cost is **0.599–0.605 ms per emulated field** at alternate-field sampling.
The headless wall difference is +4.14%; windowed wall time is lower with
history in this trial because history allows more presentation skips and
host load varies. This is **not a claimed rewind speedup**.

Final paced confirmation (`work/p413/final-paced-report.json`, commands and
environment included): **1800 fields in 30.12 s**, all five-second samples
**59.77..60.18 fields/s**, **1.118 ms/capture**, **207.4 MiB** peak process RSS,
with zero audio empty/drop events and two resyncs. A longer **6500-field** run
through scene 5 took **138.28 s (47.01 fields/s)**; five-second samples were
**39.48..56.88 fields/s** while host load rose to about 65/64 CPUs. It retained
600 snapshots, used **274,187,999 bytes (261.5 MiB)** peak ring capacity and
**284.8 MiB** peak process RSS, captured at **1.146 ms/snapshot** and had zero
audio empty/drop/resync events. Logs are `work/p413/full-speed-final/` and
`work/p413/full-speed-scene5/`. Emulator log timestamps are virtual time;
these wall times come from GNU time.

**The sustained full-speed requirement is not established on this saturated
host:** the short run passes, but the longer run and even earlier feature-off
runs miss the 59.826 Hz raster rate. The measured capture cost is small and
bounded, but an unloaded CPU/real desktop check is still required.
No other user's work was stopped to manufacture a favorable benchmark.
Invincibility and hitbox display remain intentionally deferred as explained
above. Physical pads/speakers/fullscreen, native Windows dialogs and the
macOS app/icon at a desktop remain unverified.

Fresh Linux 2000-field smoke at VB1900 has MD5
`9bd5386ea7439621c19674381086e8fd` (`work/p413/linux-smoke/shot-1900.png`),
1,471,274 samples / 1,294,352 non-silent. The P4.12 MD5 is obsolete after
P4.11's color correction. **No corresponding current Wine MD5 exists** because
`wineserver` cannot bind its socket here.

Final owner ZIP: `work/win/DieHardArcade-windows-x64.zip`,
**5,532,933 bytes**, **16 entries**, integrity checked, no ROM/save/input files.
ZIP SHA-256: `7c9bc47fdade1d4ebc7e167b8bc2e416356bca04fa25f6fefe89a0c277c36ac4`.
Executable SHA-256: `23bce528ffbcf54656b7732a2622a51a565a64996749e92040692a89afe9d3f6`.
`work/p413/package-final.json` and the package's import/hash manifest retain the
inventory. The distribution still needs `SDL3.dll`, `libc++.dll` and
`libunwind.dll` beside the executable; no new DLL was added.

Workspace usage at close: approximately **508 MiB** in `work/p413`, **2.6 GiB**
in `work/win`, with **84 GiB free** on the workspace filesystem. No commit.
The convergence trees and unrelated edits are preserved.

## Ten-line summary

1. P4.13 implements rewind, fast-forward, autofire, training, input files and app packaging/icon changes.
2. Rewind keeps about 20 s at 30 snapshots/s; 300 repeated fields match pixels/RAM/semantics exactly.
3. Final rewind cost is about 0.60 ms/field and 162 MiB additional peak RSS in the measured window run.
4. Tab/F4 fast-forward mutes host audio; six independent A/B/C autofire rates persist in both menus.
5. Health/lives/free credits and original scene select are verified; CHEATS is visible; invincibility/hitboxes are deferred.
6. Input files pass 2100 power-on and 600 embedded-state fields plus full PCM equality and four rejection cases.
7. All 147 runtime tests and 56 P4.11 PNGs/four traces/four WAVs pass; patches reconstruct exactly.
8. Shared in-app ZIP preparation and the refreshed Windows ZIP build; current Wine execution is sandbox-blocked.
9. Original ICNS/ICO assets are wired into macOS/Windows; all seven PE icon payloads match the source ICO.
10. Full-speed presentation, real Windows and the Mac app still need external verification; no commit or convergence edits.

## Addendum (PM, outside the Codex sandbox)

Wine smoke of the refreshed Windows package, run outside the sandbox with the
P4.12 command: rc 0, 0 FATAL, `shot-1900.png` md5 `9bd5386ea7439621c19674381086e8fd`,
identical to the current Linux SDL build (`port/run/P46/build-sdl/saturn_stv`) for the same
command. Evidence: `work/p413/wine-me/`, `work/p413/linux-me/`.

## macOS verification

2026-11-09, Intel i9 Mac (macOS 26.5.1, Apple clang 21, Homebrew SDL3 3.4.12).
The Mac copy was fast-forwarded to `32a9323`; the P4.13 files and the
`tools/saturn-recomp` runtime edits were copied from p620 (runtime `git diff` hash
identical, untracked runtime files identical). **No macOS-specific fix was needed**,
so `port/runtime-patches/p413-macos-fixes.patch` does not exist. Evidence is in
`work/p413mac/` (Mac copy only); verifier outputs are in the usual `work/p413/` paths.

| Check | Result |
|---|---|
| `bash port/p48_build_mac.sh`, `bash port/p411_build_app.sh` | Both build, **zero warnings**. The wrapper plus `port/p413_roms.c` also compile with `-Wall -Wextra -Werror` |
| Smoke, 2000 fields, shot 1900 | rc 0, 0 FATAL, md5 **`9bd5386ea7439621c19674381086e8fd`** for the P4.8 binary *and* the bundled `Contents/MacOS/saturn_stv`. Matches Linux and Windows |
| Runtime unit tests (`runtime/tests`, clang++) | **147 tests, 0 failed** |
| `p413_verify_regression.py` (P48 binary at the P46 path) | **PASS**: 56/56 PNGs, all four guest/full traces and WAVs identical to p620's P4.11 manifest |
| `p413_verify_features.py rewind` | **PASS**: 300/300 rewound fields bit-identical (pixels, RAM, semantics) |
| `p413_verify_inputs.py` | **PASS**: 2100 power-on + 600 embedded-state fields; 4/4 bad files rejected |
| `p413_verify_training.py` | **PASS**: health/lives 1413 held samples each, no-coin start, scenes 28/72 |
| `p413_verify_roms.py` | **PASS**: 72 zlib-oracle cases, 132 error cases, 3 SHA vectors, every set variant |
| `p410_verify_states.py` | **PASS**: 300 warm + 500 cold, RAM/semantics/PCM identical, 4/4 rejected |
| `p413_verify_patches.py` | **PASS**: the four P4.13 patches rebuild the Mac runtime's 75 files exactly (BSD patch, zero fuzz/offset); Windows 126 files also match |
| `p413_verify_icons.py` (Pillow in a temporary venv; PE copied from p620) | **PASS**: 11 ICNS / 7 ICO sizes, all PE payloads exact. `iconutil` also unpacks all 11 ICNS images |
| `p413_verify_features.py speed` (paced, SDL dummy, idle Mac) | 1800 fields in 30.1 s off and 30.4 s with 20 s history. 5 s samples were **59.8–60.2 fields/s**, with one history sample at 56.0. Zero audio empty/drop events; 1.59 ms/capture, 600 snapshots, 251.8 MiB ring |
| `p413_verify_windows.py` | Skipped: Linux only (Wine/Xvfb) |

App bundle (`port/run/P411/Die Hard Arcade.app`, 11 MB): `Contents/Resources/DieHardArcade.icns`
is byte-identical to `port/assets/p413-icon.icns`. Info.plist has `CFBundleIconFile`
`DieHardArcade.icns` and version 1.02/102. The ad-hoc signature verifies (`codesign -vv --deep`).
Every Mach-O has minos 14.0, and SDL3 is the only non-system dylib (`@rpath`). The
NSWorkspace (Finder) icon of an app copy outside the repo shows the shield/skyline art
(`work/p413mac/finder-icon.png`, inspected).

Launcher captures (`SATURN_LAUNCHER_SHOT`, SDL dummy/software) were inspected:
`work/p413mac/launcher-{main,features,training,movie}.png`. They show the Game/controls
tab, the Player features tab with its Time/autofire, Training/cheats and Input files
sub-tabs, and all controls within 960x720. An app copy outside the repo with the real
data folder logs `Die Hard Arcade 1.02` and opens the launcher (`app-launcher.png`).
With a throwaway `HOME` whose path contains a space and `ü`, and only `diehard.zip`
present, first launch runs the in-process converter: no `/usr/bin/unzip`, about 0.75 s in
total, `cart.bin`/`diehard_main.bin` byte-identical to the repository images. The test's
default `stv.ini`/`app.log` were then removed from the real data folder.

Still not checked on macOS: a real desktop window, physical pads and speakers, fullscreen,
the missing-ROM dialog, and a long full-speed run through scene 5 with the visible SDL
renderer.
