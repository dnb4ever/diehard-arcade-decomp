# P4.6: real-time play in a window (picture, sound, keyboard and gamepads)

2026-11-06. Tested on the build host over ssh. No commit made. Emulation behaviour is
unchanged: every P4.5 regression PNG and trace is byte-identical (section 6).

## 0. The P4.5 seed `0601FC1C`: verified

`0601FC1C` is not reached through a pointer literal. It is the child task step that
`func_0221bb40` (RAM `0x0601FB40`, a 6-state Scene 8 task) installs in its state 2 with
`mova` + `func_0220ff72(task, 0x0601FC1C)`. State 1 counts down a timer first, so the step only
runs if Scene 8 (the corridor before Scene 9) lasts long enough. In the first game of the P4.5
run the scripted player left Scene 8 after 73 VBlanks, before the countdown ended. In the second
game it stayed longer and hit the FATAL.

Shortest repro: the scripted player from power-on, and P1 holds UP in Scene 8 so it stays there
(`port/run/P46/fc1c_repro.sh`, 7600 VBlanks, about 90 s):

```bash
STV_AUTOPLAY=100 STV_COVDUMP=1 port/run/P41/build-generated/saturn_stv --cart port/diehard_main.bin \
  --cartrom cart.bin --vblanks 7600 --stvinput 99999:COIN,7104:P1UP:400 --out port/run/P46/fc1c-final
```

Result with the final P4.6 binary: Scene 8 from VB 7104 to 7401, **0 FATAL**, and `cov.txt` lists both
`0601FB40` and `0601FC1C` as run. Without the hold (`port/run/P46/fc1c-a`) the scene ends first:
`0601FB40` runs and `0601FC1C` does not. That matches the first game of the P4.5 run. Picture:
`port/run/P46/fc1c-b/shot-7350.png`.

## 1. How to play

At the P620 desktop (log in at the monitor; the box shows only the login screen now), in a terminal:

```bash
cd ~/decomp/diehard-stv
port/play.sh                  # window, 3x (960x720), sound, keyboard + gamepads
port/play.sh --fullscreen     # or F11 / Alt+Enter in the game
```

Build it once first if `port/run/P46/build-sdl/saturn_stv` is missing (no root needed):
`port/p46_build_window.sh`. `port/play.sh` runs, with no `STV_*` variables:

```bash
port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin --window --out port/run/P46/shots
```

Options: `--fullscreen`, `--scale N` (window 320x240 times N), `--settings FILE`, `--stats N`
(speed and audio queue every N s, to the log), `--unpaced` (no pacing, for measuring).

Over ssh from the Mac this does not work well. X forwarding sends every frame over the network with
software rendering. Play at the P620's own screen, or build for the Mac (section 7).

## 2. Key map (defaults)

| Control | Player 1 | Player 2 |
|---|---|---|
| Lever | arrow keys | R F D G (up down left right) |
| A / B / C | Z / X / C | A / S / Q |
| START | 1 | 2 |
| COIN | 5 | 6 |

| System and window | Key |
|---|---|
| TEST / SERVICE | F2 / 9 |
| Pause menu (Resume, Quit) | Esc, P or Pause |
| Fullscreen | F11 or Alt+Enter |
| Screenshot (`port/run/P46/shots/shot-VB.png`) | F12 |
| Quit | close the window, or Quit in the menu |

Gamepads (SDL GameController, any Xbox/PlayStation/Switch-style pad): the first pad is P1 and the
second is P2. D-pad or left stick = lever. A, B and C are the left, bottom and right face buttons
(X, A, B on an Xbox pad). Start = START, Back/Select = COIN, Guide = pause menu.

Remapping: the first windowed run writes `~/.local/share/saturn-recomp/stv.ini` with every key and
an explanation. `[arcade]` holds the panel keys by SDL key name (e.g. `p1_a = Z, LeftCtrl`), and
`pad_coin` holds the gamepad coin button. `[gamepad]` holds the pad's lever and A/B/C/START buttons
(also per pad model, as `[gamepad GUID]`). `[display] scale`, `integer` and `frameskip`, and
`[audio] volume`, are there too. Delete a line to get its default back.

Notes: one coin = one credit, common chute (factory BIOS settings, P4.5). TEST is wired as on the
cabinet, but the BIOS test menu (`0x0600060C`) is not emulated (P4.5 defect 1). The cart then logs
`ST-V BIOS: exit to BIOS`, so do not press it during play.

## 3. What was there, what was added

The saturn-recomp runtime already had a complete SDL3 host (`runtime/src/host/host.cpp`). It
had a window with a 4:3 picture, frame pacing of each VBlank-IN against the host clock, an SDL
audio stream fed from the SCSP (50 ms lead, 250 ms cap, so latency cannot run away), Saturn-pad
keyboard and gamepad input with settings and per-model bindings, a pause menu, fullscreen and an
F12 screenshot. The ST-V harness never used it: it forced `headless`, and the build had no SDL3
(only the runtime library `libsdl3-0` is installed; `libsdl3-dev` is not).

Added (front end):

- Build: `port/p46_build_window.sh`. It unpacks Ubuntu's `libsdl3-dev` with `apt-get download`
  (no root, headers and CMake files only) and links the system `libSDL3.so.0` (3.4.2). It builds
  `port/run/P46/build-sdl/saturn_stv` from the same generated recompile as the headless binary.
- `port/main_stv.cpp --window`: turns on the SDL host. The default coin/start timeline and the
  600-VBlank limit apply only headless. The window title is "Die Hard Arcade". The settings file is
  `stv.ini`.
- Control panel (`p46-realtime-window.patch`): `host_stv_panel()` turns the `[arcade]` keys and
  the first two gamepads into P1/P2 raw panel words and the system switches. `bios.cpp` ORs them
  into the same BIOS words `--stvinput` writes (`0x06000708`, `0x06000710`, `0x06000728`), so the
  cart sees exactly what a timeline press gives it. A coin key adds one BIOS credit on the press.
  A key tapped and let go between two frames still counts for one frame. `STV_AUTOPLAY` plays P1
  only while the player leaves P1 alone.
- Picture: `[display] integer = auto`. Each line is a whole number of pixels tall (3x = 720 lines
  for 240), the width is 4:3 of that (704 dots -> 960), and scaling is sharp (`SDL_SCALEMODE_PIXELART`).
  `false` gives the old smoothed fit-to-window.
- Pacing: unchanged. One VBlank per field of the runtime's NTSC raster, **263 lines x 63.556 us
  = 59.826 Hz** (Mednafen's Saturn timing). The ST-V target given was ~59.94 Hz. Changing the raster
  would change emulation timing, so it is left alone. VSync is off in the host so that a hidden
  window cannot stall the machine; expect occasional tearing on a 60 Hz monitor.
- `[display] frameskip = true`: when the host is more than a field behind, up to two pictures in a
  row are left out. Game speed and sound hold, and the picture drops toward 20-30 fps in the
  heaviest moments.
- Save states: not built. The runtime's `state.cpp` saves only at a `--tasks` task switch, which
  Die Hard does not have, and `main_stv` does not use `saturn_main`. Not cheap.

Speed (`p46-speed-handoff-compose.patch`, output byte-identical):

- The runtime runs the master and slave SH-2s on two threads that take strict turns. Die Hard
  hands over about 326 times per frame. Each handoff was a mutex/condvar sleep and wake. Measured:
  the run stalled about 6 ms a frame in futex wake-ups on this busy host. The waiting thread now
  spins on an atomic turn for about 1 ms before it sleeps. Whose turn it is, and so the run, does not
  change. Headless: **55 s -> 33 s** for the same 3000 VBlanks.
- `vdp2_compose` took **16 ms** a field single-threaded (704x240, RBG0 rotation floor). Lines are
  independent, so bands go to up to 8 threads: **3.7-4.1 ms**.

## 4. Speed (build host, load average 45-53 from other jobs during every measurement)

CPU: Threadripper PRO 5975WX. Both GPUs were at 100% (training), and the load average was 45-53
on 64 threads throughout, so these numbers are pessimistic. Per frame, measured with temporary
instrumentation (removed again):

| Part | Before | P4.6 |
|---|---:|---:|
| Slave SH-2 (Die Hard's geometry CPU), its turns | ~7.5 ms | ~7.5 ms (9-11 ms while the window composes on 8 threads) |
| Master SH-2 + SCSP/68000 + VDP1 | ~3.1 ms | ~3.1 ms |
| CPU handoffs | ~6 ms | ~0 |
| VDP2 compose (window only) | 16 ms | ~4 ms |
| Present, SDL software renderer (offscreen) | 3.7-4.4 ms | same (a GPU renderer at the desktop: well under 1 ms) |

Uncapped, scripted player from power-on through Scene 5 (the busiest fight so far; 6500 VBlanks;
`port/run/P46/perf_final.sh`):

| Mode | VBlanks/s | vs 59.83 Hz |
|---|---:|---:|
| Headless, no picture (emulation alone) | **84.7** (6500 in 76.7 s) | 1.42x |
| Window, every field composed and shown, software renderer | **47-56** (Scene 5: 47-55) | 0.78-0.93x |
| Window, paced, frameskip on, software renderer | **59.7-60.2** held; 7-34% of fields left out (shows 40-56 fps) | 1.00x |

So on this loaded box the emulation has 42% headroom. Composing and showing every field does not
fit in 16.7 ms, mainly because of the software renderer and the contended cores. Frameskip keeps the
game at full speed. Under Xvfb with the lavapipe (CPU) Vulkan renderer, the paced window held
56-59.6 VBlanks/s and showed about a third of the fields (`port/run/P46/win2/run.log`). At the
real desktop the renderer is the GPU, so present drops out of the budget.

Audio (paced window, SDL disk driver capturing the device stream, `port/run/P46/win2/sdl-audio-out.wav`):
66 s of non-silent 44.1 kHz stereo, silent only while the pause menu was open. The queue stayed
at 8-108 ms. A few underruns occurred (each padded with 50 ms of silence) while the box was behind.
In the 6500-VBlank paced run there were 9 underruns in 109 s. Startup caveat: while the window opens,
the boot runs ahead and fills the queue to its 250 ms cap (the "chunks dropped" counter only grows then).
The queue then drains to about 80 ms over the first ~30 s, so sound lags the picture by up to a
quarter second at first. Fix next: drop the stream back to the 50 ms lead once pacing has settled.

## 5. Verified (and how)

There is no logged-in desktop on the box (only the GDM greeter), and Xvfb/xdotool are not
installed. I unpacked Ubuntu's `xvfb`, `xdotool` and `x11-apps` without root into
`port/run/P46/xdeb` and ran a private Xvfb on `:77`. `port/run/P46/wintest.sh` drives a real X11
window with XTEST key presses, grabs it with `xwd`, and records the audio device through SDL's
disk driver.

- Window opens, 960x720, sharp 4:3 picture: `port/run/P46/win1/win-09.png`, `port/run/P46/win2/win-30.png`.
- Keyboard: `5` (tapped) -> `ST-V coin: COIN1 -> BIOS credits=1 at VB 402`; `1` -> game starts
  (scene 0 -> 2 -> stage scene 3 at VB 1294). P1 then fought and reached CONTINUE? (`win2/run.log`).
- Pause (`P`): menu over a dimmed, frozen game (`win2/win-35p.png`); the audio is silent meanwhile.
- F12 screenshot: `win2/shot-1686.png`. F11 toggled; bare Xvfb has no window manager, so the
  window only moved (`win2/win-41f.png`). Real fullscreen needs the desktop to check.
- Gamepads: two SDL virtual gamepads (`--virtual-input`, through the same SDL gamepad and binding
  path as real pads, `port/run/P46/pad/run.log`): both connect as players 1 and 2, both START
  presses join (two player actors), and P1 RIGHT / P2 LEFT move them the right way
  (x `0x120000 -> 0x11E355`, `0x130000 -> 0x132666`). The Back = coin button cannot be pressed on a
  virtual pad; it uses the same code path as the coin keys.
- Not verified here: real keyboards and pads at a desktop, real fullscreen, GPU renderer timing,
  audio on real speakers.

## 6. Regressions

| Check | Result |
|---|---|
| Runtime unit tests | **PASS** `128 tests, 0 failed` (`port/run/P46/tests.log`) |
| P4.5 reference runs: no-coin attract 9000 VB, game over -> attract -> new game, combat 2100 VB, legacy multi-cart combat (`port/run/P46/reg_runs.sh`) | **PASS, byte-identical.** All 56 PNGs match `port/run/P45/final-*` md5 for md5, and the scene trace, coin, FATAL, stop and sound sample-count lines match. Run with the headless binary (`regH-*`) and the SDL binary headless (`regS-*`). |
| `--shots`, `--stvinput`, `STV_AUTOPLAY` headless | unchanged (above, and the item-0 repro hits the same scene VBlanks as before the change) |

## 7. What is next

1. **Play-test at the P620 desktop** (`port/play.sh`) with a real pad: feel of input latency (one
   field of pacing plus the audio lead), tearing, fullscreen. Tune `[display]`/`[arcade]` defaults.
2. **Mac build**: feasible. The runtime already targets macOS, Windows and Android
   (`sat_data_dir`, settings path, `uinput` is Linux-only and guarded). SDL3 comes from Homebrew
   (`brew install sdl3`). The generated recompile is plain C++ (`port/run/P41/recomp`, 14 MB) and
   can be copied or regenerated with Python. You also need `cart.bin` and
   `port/diehard_main.bin`. An Intel i9 should have more per-core speed than this loaded box. Not
   built or tested yet.
3. **GUI launcher**: the runtime has an ImGui launcher (`--launcher`: players, controls, display,
   volume) that `main_stv` does not wire up yet. A small step: reuse it for key remapping and
   display options, plus a `.desktop` entry or an app bundle on the Mac.
4. More speed if needed: overlap VDP2 compose with the next field (snapshot the video memories, one
   field of latency). The slave SH-2's ~7.5 ms is now the floor, and it is emulation work.
5. Cosmetic: the pause menu still shows the Saturn touch-pad column. Hide it in arcade mode.
6. P4.5 leftovers still open: BIOS test menu (`0x0600060C`), backup RAM (`0x06000644`), and the
   credits check against MAME.
