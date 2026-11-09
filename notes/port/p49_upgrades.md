# P4.9 player-facing upgrades

2026-11-08. All commands and artifact paths are relative to the
repository root. No commit. Existing working-tree changes are preserved.

Build on Linux with `port/p46_build_window.sh`, then open the controls/settings
with `port/play.sh --launcher --stats 5`. On macOS keep using
`port/p48_build_mac.sh` and `port/play_mac.sh --stats 5` (launcher enabled by
that script). Changes are portable SDL3/C++ only; macOS was not built here.

## 1. Audio queue

The old host prefills 50 ms of silence while initializing, resumes the device
immediately, drops new samples above 250 ms, and refills 50 ms on empty.
`p49-audio-queue.patch` starts playback only when samples arrive, uses a 30 ms
lead, follows queue occupancy with a smoothed +/-2% SDL playback resampling
correction, and replaces stale audio on a 75 ms discontinuity. Normal playback
retains every chunk. Guest sample production, saved WAV and emulation timing
are unchanged. SDL3 APIs work on Linux and macOS; no device-specific API added.

Baseline: `work/p49/baseline/run.log`, SDL dummy audio/video with software
rendering, 3600 VB, `--stats 5`, no inputs. The earlier P4.6 250 ms boot backlog
was not reproduced on this run: first interval 10..81 ms, later 5..80 ms,
eight empty events by the end. These are stream-queue measurements, not measured
speaker-to-display latency. Physical audio still needs a desktop audition.

After: `work/p49/audio-after/run.log`: first interval 15..60 ms, 5..35 s
intervals 7..38 ms, later loaded intervals 0..52 ms. **Zero empty events,
zero dropped chunks, zero resyncs in 3600 VB / 60.107 guest seconds**;
identical 2,650,698 guest samples and 2,301,417 non-silent samples before/after.
Steady stream queue meets the <=60 ms target; device/OS buffering is additional.
Playback pitch can vary by at most 2% to follow the device clock. Sustained host
stalls still cannot guarantee uninterrupted audio.

`port/p46_build_window.sh` passes (`work/p49/audio-build.log`). Runtime suite:
**131 tests, 0 failed** (`work/p49/tests.log`). Reproduce with:

```bash
SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIO_DRIVER=dummy \
  port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --window --no-launcher --settings work/p49/audio-after/stv.ini --stats 5 \
  --vblanks 3600 --stvinput 99999:COIN --out work/p49/audio-after
```

## 2. Separate player gamepad layouts

`p49-player-pad-layouts.patch` adds P1 pad and P2 pad columns to the arcade
launcher. Choose a control, then press the corresponding player's controller;
Escape cancels. Coin also has a separate binding for each player. Hover a
binding to read its full SDL button name. Identical model/GUID controllers can
use different layouts. Assignments follow the existing connection-order player
slots and reconnect rules. Defaults remain unchanged. With no player layout,
the old model-specific `[gamepad GUID]` or shared `[gamepad]` mapping and
`[arcade] pad_coin` still apply. An edit snapshots that player's effective layout
into `[gamepad p1]` or `[gamepad p2]`, saved immediately to `--settings FILE`.
“Default arcade controls” restores the factory keyboard and both player layouts.

Verification: **132 tests, 0 failed** (`work/p49/pads-tests.log`), including a
save/reload case with shared overrides and an explicitly factory-valued player
layout. Window build passes (`work/p49/pads-build.log`). Inspected dummy SDL
launcher captures: `work/p49/pads-launcher.png`, `pads-edited.png` and
`pads-reloaded.png` (same directory). An external test driver
`work/p49/launcher_input.cpp` connects two identical virtual pads and clicks the
real UI. P1 A changes to rightshoulder and P2 B to leftshoulder; an intervening
P2 press while binding P1 is ignored. `work/p49/pads-edited.ini` retains both
layouts on reload. Physical presses through those mappings then give BIOS raw
P1 `0001`, P2 `0002` at VB60 (`work/p49/panel-test/run.log`). A second live-pad check binds P1 A to Back and coins to
P1 left-stick-click / P2 right-stick-click: raw P1 `0001`, system switches
`0003`, exactly two credits, and no pause menu (`work/p49/pad-coins.log`). Back
can be rebound freely in arcade mode; Guide is the menu button.

`SATURN_LAUNCHER_SHOT_FRAME=N` optionally delays the existing launcher capture
(default frame 3) for automated input checks. This only affects the capture
harness. Real controllers at a desktop remain a manual check.

## 3. Game settings: verified launcher fallback

The BIOS TEST dispatcher/renderer and non-returning cart-reload path are still
absent from the static recompile. The actual cart test program does run with
its real boot reason 2 (`work/p49/cart-test/shot-200.png`), but simply returning
from `0x0600060C` cannot enter it or reboot safely. P4.9 takes route **(b)**:
the launcher edits verified settings, saved in the same INI, then the harness
writes the BIOS work-copy bytes that the real EEPROM menu supplies. It does
not synthesize actor stats or patch cart instructions. Physical TEST still logs
an unimplemented BIOS exit; this is an open issue, not a completed TEST path.

In **Game settings (next start)** choose difficulty 1..8, initial vitality
171/146/128/114/102/93/85/64, initial players (lives) 1..4, and free play.
Changes apply on Start and persist across launches. Factory game settings
restores difficulty 4, vitality 114, one life and coin play. Free play grants
START/continue permission without creating fake credits. The real game menu
has no separate “allow continue” or continue-price switch; none was invented.
The launcher does not expose the real menu's pause-button and violence options.

`[stv]` keys are `difficulty = 4`, `vitality = 3` (table index), `lives = 1`,
`free_play = false`. Explicit `--settings FILE` also works headless for replay.
The existing macOS play script already supplies a persistent settings path;
no build-script or platform API change is needed.

MAME 0.285 / US BIOS was driven through Service Mode (physical TEST), Service 1
(move), and Service Mode (choose/change). Each setting was changed separately
in its real menu, all menus were exited to save, and the saved NVRAM was booted
again to play. Scripts/actions, screenshots, EEPROM files and raw backup-RAM
captures are under `work/p49/mame/`. `run.sh` runs `probe.lua` using
`port/run/P45/mamepkg`; `*-save.actions` retain the exact frame/input schedules.
**These settings live in the 128-byte EEPROM, not the 64 KiB backup-RAM image**;
the latter stayed unchanged. The EEPROM stores swapped 16-bit words and a
second record at +0x3C, with checksum/sequence changes on save.

| Menu change | Logical EEPROM data change | BIOS work copy | Saved-file offsets changed for data |
|---|---|---|---|
| Difficulty 4 -> 5 | active region byte `1B -> 1C` | `060007A0` | `21`, `5D` |
| Vitality 114 -> 102 | `1B -> 23` | `060007A0` | `21`, `5D` |
| Initial players 1 -> 2 | `1B -> 5B` | `060007A0` | `21`, `5D` |
| Coin preset 1 -> FREE PLAY | `00 00 -> 19 01` | `06000794..795` | `14`, `15`, `50`, `51` |

Offsets are hexadecimal. All four leave `06000798 = 000808FD` unchanged.
Difficulty is bits 0..2, vitality index bits 3..5, and lives-minus-one bits 6..7
in the selected region's soft DIP. Service `0x634` still copies the eight bytes
normally. Region-bank selection follows the cart's own region table, rather
than always writing the USA address. The INI persists these selected values;
it does not claim to write a full MAME EEPROM image, checksum or bookkeeping.

`p49-game-settings.patch` records launcher/default validation and BIOS free-play
report/credit permission. `port/main_stv.cpp` supplies settings bytes at boot.
The 20-byte free-play panel report matches MAME (`00 00 01 02 ... 02 ... 09 11
00 00`); service `0x664` repeatedly grants both players without charging a coin.
The default paid branch is unchanged. **134 runtime tests pass**
(`work/p49/settings-tests.log`). Window and portable headless builds pass.

`python3 port/p49_verify_settings.py` verifies both EEPROM record copies and
replays five native runs through the existing read-only scenario stream.
`work/p49/settings-effects/report.json` records all EEPROM byte differences,
SHA256s, commands and guest effects; `settings-effects.log` records success.
MAME frame 1650 and port VB1150 agree on these first-fight settings/effects:

- Difficulty 4 -> 5 raises the two enemies' action-level fields from 0/1 to 1/2.
  By native VB1800 their positions/attacks differ and P1 has 68 HP versus 72.
- Vitality 114 -> 102 gives the player 102 initial HP; at VB1800 it has 60 HP
  versus 72 under the identical paid-input schedule. The health bar changes.
- Lives 1 -> 2 gives `06099796 = 2`, active life count 2, and the HUD's extra-life
  indication, through the guest's normal decode.
- Free play reaches paid state 2 and scene 3 after START with **no COIN input**;
  the unjoined-player coin prompt disappears.

Inspected the game PNGs at VB1150 and VB1800 for all variants, and the MAME
assignment/menu PNGs. `work/p49/settings-edited.png` shows real UI edits to all
four fields via the external SDL event driver; `work/p49/settings-ui.ini`
retains difficulty 5, vitality index 4, two lives and free play after the UI
closes. No game RAM was forced by any reference or verification script.

The no-coin continue replay also passes (`work/p49/free-continue/run.log`):
P1 reaches CONTINUE at VB3344, START at VB4501 returns it to playing at VB4502,
without any coin event. Inspected `shot-4490.png` and `shot-4600.png` in that
run for the countdown and resumed fight.

## 4. CRT and scaling

In **Display and sound**, select Scaling **Auto / Sharp / Smooth** and optionally
check **CRT scanlines / phosphor mask**. Both choices save immediately. CRT is
off by default. Auto preserves the old integer-scaling behavior (sharp for
arcade, smooth when integer mode is disabled). Explicit Sharp/Smooth changes
sampling independently of `[display] integer`, which still controls geometry.
INI: `[display] crt = false`, `scaling = auto`.

`p49-crt-display.patch` filters only the uploaded display texture: a horizontal
1:6:1 blur, mild repeating RGB mask, and a 72%-brightness lower half-line.
It uses a reusable CPU buffer and ordinary SDL3 textures, with no shader,
OpenGL, Metal or Linux-specific API. The extra texture rows preserve scanlines
across source resolutions. Guest framebuffers, reference PNGs and F12's guest
screenshot are unchanged; UI overlays are drawn after the effect.

`SATURN_WINDOW_SHOT=FILE SATURN_WINDOW_SHOT_VB=N` captures the actual rendered
window (including filter/UI) before present. This opt-in check shares the
launcher's existing renderer-readback helper. Example:

```bash
SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIO_DRIVER=dummy \
  SATURN_WINDOW_SHOT=work/p49/display/crt-sharp.png SATURN_WINDOW_SHOT_VB=1800 \
  port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin \
  --window --no-launcher --settings work/p49/display/crt-sharp.ini \
  --vblanks 1810 --stvinput 100:COIN,200:START1 --out work/p49/display
```

Inspected all four full-window PNGs under `work/p49/display/`: `off-sharp.png`,
`off-smooth.png`, `crt-sharp.png`, `crt-smooth.png`. Sharp retains hard edges,
smooth interpolates them, and both CRT variants show the mask and scanlines.
The inherited Saturn touch controls appear under dummy video in these images;
the CRT is correctly confined to the game image. `work/p49/crt-launcher.png`
shows the real UI interaction selecting Sharp and enabling CRT; the saved
`work/p49/display-ui.ini` confirms both. All seven changed runtime source/test
files reconstruct byte for byte with the four ordered patches and zero fuzz
(`work/p49/patch-validation.log`).

Paced software-renderer performance (`work/p49/perf/report.json`), two sequential
3600-VB autoplay runs with identical input/default settings except CRT. Touch
controls were explicitly hidden in both benchmark INIs. Full-window captures
at VB1800 were inspected (`work/p49/perf/off/window.png`, `crt/window.png`).

| Display | Wall seconds | VB/s | Pictures skipped | Audio empty/drop/resync |
|---|---:|---:|---:|---:|
| CRT off, sharp | 60.112 | 59.888 | 0 | 0 / 0 / 0 |
| CRT on, sharp | 60.139 | 59.861 | 0 | 0 / 0 / 0 |

Both keep full raster speed on this host without relying on frameskip. Steady
five-second audio ranges are typically 6..39 ms with CRT; the device reports
1024 frames/block (23.2 ms), which is additional to the stream queue.
This is not an end-to-end speaker-latency measurement or a Mac performance claim.

## Final regressions, reproduction and remaining limits

- Runtime suite: **134 tests, 0 failed**, `work/p49/final-tests.log`.
- Linux SDL build: **PASS**, `work/p49/final-window-build.log`;
  portable null-host build and smoke: `work/p49/final-headless-build.log`,
  `final-headless-smoke.log` in the same directory. macOS Bash scripts pass
  `bash -n`; no changes to their build assumptions or compiler flags.
- All four P4.5 schedules (using current P4.7 boot state) **PASS**: 9000-VB
  attract, 9000-VB game-over/new-game, 2100-VB combat and legacy combat.
  **56/56 PNG hashes and all four deterministic trace hashes match the
  pre-P4.9 baseline**. Attract returns at VB8870, game over at VB5311, attract
  at VB5613, second paid game at VB6120. Zero FATAL. This preserves the known
  P4.7 differences from the original P4.5 run, rather than claiming those are
  fixed. Manifests: `port/run/P48/p49-baseline/manifest.json` and
  `port/run/P48/p49-final/manifest.json`; logs `work/p49/regression-before.log`
  and `work/p49/regression-after.log`.
- Four incremental runtime patches reconstruct all changed runtime files with
  zero fuzz. `git diff --check` passes for the touched source files.

```bash
cmake -S tools/saturn-recomp/runtime/tests -B work/p49/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p49/runtime-tests -j8
work/p49/runtime-tests/runtime_tests
port/p46_build_window.sh
cmake -S port/run/P41/recomp -B port/run/P49/build-headless \
  -DSATURN_RUNTIME="$PWD/tools/saturn-recomp/runtime" \
  -DSATURN_EXTRA="$PWD/port/stv.cmake" -DSATURN_NO_SDL=ON
cmake --build port/run/P49/build-headless --target saturn_stv -j8
python3 port/p49_verify_settings.py
python3 port/p48_verify.py --binary port/run/P49/build-headless/saturn_stv \
  --label p49-repro --schedules nocoin,gameover,combat,legacy-combat \
  --reference port/run/P48/p49-baseline/manifest.json
python3 work/p49/perf/run.py
```

To reproduce MAME menu changes from a fresh destination, for example:

```bash
P49_MAME_ACTIONS=work/p49/mame/difficulty-save.actions \
  P49_MAME_STOP=2900 P49_MAME_SHOTS=1480,1660,2100,2850 \
  work/p49/mame/run.sh difficulty-repro
```

Use `lives-save.actions`, `vitality-save.actions` and `free-save.actions` for the
other changes (free play exits at frame 2700). Do not reuse an already changed
NVRAM directory for the menu-changing pass. The gameplay pass intentionally
reuses the saved NVRAM, using `start-paid.actions` or `start-free.actions`.

Remaining limits: the physical TEST BIOS dispatcher/menu/cart reboot remains
unimplemented; ranks/bookkeeping and full backup RAM/EEPROM emulation are not
added. Desktop speakers, physical pads, macOS compilation/display/audio, high
DPI/retina rendering and OS compositor behavior still need hardware testing.
The existing P4.7 fidelity gaps remain. No GPU, prohibited directory or
unrelated process was used or changed; no commit was made.
