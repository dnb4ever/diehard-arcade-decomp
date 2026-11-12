# saturn-recomp runtime patches (ST-V / Die Hard Arcade)

The native port is built on a nested clone of the saturn-recomp runtime, which
is gitignored out of this repo (it is a large third-party tree). The custom
edits that make Die Hard Arcade boot and render are captured here as a patch so
the port is fully reconstructable from this repository alone.

- Upstream: https://github.com/callumjenkins/saturn-recomp
- Base commit: b22688d0bef52fba31cda79fd23150e8905cf3c2
- Patch: saturn-recomp-stv.patch (6 files, STV BIOS input/coin service, VDP2
  rotation over-mode + RBG fixes, mmio/machine/video hooks)

## Apply
    git clone https://github.com/callumjenkins/saturn-recomp
    cd saturn-recomp && git checkout b22688d0bef52fba31cda79fd23150e8905cf3c2
    git apply /path/to/port/runtime-patches/saturn-recomp-stv.patch

Then build per upstream instructions; the ST-V boot harness is port/main_stv.cpp.

## P4.1 faithful combat (2026-10-30)

The recorded `round-runtime-dh-portB.patch` is a complete patch against the base
above, including the panel control-word fix described in the round-entry note.
Use it instead of stacking `saturn-recomp-stv.patch` on the same base.
Then apply `p41-raw-panel-input.patch` for real A/B/C/direction input through the
BIOS raw peripheral words. Optional `p41-player-trace.patch` adds read-only,
per-VBlank `STV_P41TRACE=1` diagnostics. These two patches apply in that order.

`python3 port/p41_build.py` rebuilds under `port/run/P41`, preserving old generated
entry seeds as well as `seeds_extra.txt`. Details, commands, limitations and PNG
paths: [P4.1 player spawn](../../notes/port/p41_player_spawn.md).

## P4.2 floor and progression (2026-10-31)

Apply these after the P4.1 chain above, in this order:

1. `p42-rbg-map-registers.patch`: read parameter A's map at VDP2 offsets
   `0x50..0x5E`, and parameter B's at `0x60..0x6E`. The old offsets read NBG2/3
   registers and omitted the real near-floor plane. Includes an A..P pixel test
   for both parameter sets. Updates the legacy plane-override comment.
2. `p42-dmac-block-count.patch`: remove the second divide-by-four in SH-2 DMAC
   16-byte transfers. Restores the missing CRAM palette entries; includes full
   transfer, boundary sentinel, end-address and completion tests.
3. `p42-remove-retired-bridges.patch`: delete `STV_LIVEROUND`, `STV_SYNTHDEF` and
   `STV_STAGEDMA` from `runtime/src/machine/bios.cpp` (their actual location;
   none remained in `port/main_stv.cpp`). Historical purpose is recorded in the
   milestone note. Other legacy diagnostics are outside this removal.

All three patches are required to reconstruct the installed runtime. No BIOS
stage synthesis or game-state forcing was added. The stage floor renders with
`STV_BOOTMODE=0 STV_CFG798=40`, coin + START, and no renderer/bridge overrides.
The unjoined P2 coin prompt is authored cart behavior and remains intact.
Scripted combat clears the first group and reaches scenes 4 and 5.

Build with `python3 port/p41_build.py`. Runtime test commands, exact repros,
patch reconstruction results, PNG paths and remaining limits are in
[P4.2 floor and progression](../../notes/port/p42_floor_progression.md).

## P4.3 VDP1 V-Blank erase and second-area progression (2026-10-31)

Apply after the P4.2 chain above:

1. `p43-vdp1-vbe-erase.patch`: implement `TVMR` bit 3 (`VBE`, `0x0008`, V-Blank
   Erase/Write Enable) in `runtime/src/video/vdp1.cpp::vdp1_vblank_out()`,
   remove the retired `STV_VDP1ERASE` override, and add a unit test in
   `runtime/tests/frame_change_test.cpp` (`126 tests, 0 failed`).

During gameplay in 30 fps manual frame-change mode (`FBCR = 3`, `PTMR = 2`),
`vdp1_cmdlist_header_init` (`02215000.c`) draws a black clear polygon (`cmd[3]`)
only across `y = 100..240` (local coordinates `(352, 120)` with vertices
`(-352, -20)` to `(352, 120)`), relying on VDP1 hardware erase (`EWDR = 0`,
`EWLR = 0`, `EWRR = 0x58F0`, covering `x = 0..703, y = 0..240`) for the top 100
scanlines (`y = 0..99`). On every swapping VBlank (`g_0609869C == 1` and
`g_060E0BC8 == 1`), `isr_vblank_in` (`02223cf0.c`) writes `TVMR = 0x0009`
(`VBE = 1, TVM = 1`) and `FBCR = 3`, and `isr_vblank_out` (`02223d98.c`) clears
`TVMR = 0x0001` (`VBE = 0`). Emulating `TVMR.VBE` in `vdp1_vblank_out()` erases
the display framebuffer during VBlank before swapping it into the draw buffer,
eliminating all stale VDP1 geometry across `y = 0..99` during camera cuts and in
Scene 5.

With scripted input extended past Scene 5 (`port/run/P43/stage-clear-input.txt`),
P1 defeats all enemies in the second area (Scene 5) and advances through Scene 6
(hallway QTE, `"SUCCESS"`), Scene 8 (corridor transition), and into Scene 9 (the
third combat area) with no runtime or BIOS gate blocking progression. Details,
repros, and PNG paths are in
[P4.3 VDP1 erase and progression](../../notes/port/p43_vdp1_progression.md).

## P4.4 Missing recompiled entries and scripted-player harness (2026-11-06)

Apply after the P4.3 chain above:

1. `p44-autoplay-harness.patch`: adds `STV_AUTOPLAY=VB` to `runtime/src/machine/bios.cpp`,
   a test harness that is off by default. From VBlank `VB`, whenever the `--stvinput`
   timeline holds no P1 control, it picks the P1 raw panel word `0x06000708`. It reads
   only the actor slots the P41 trace prints, walks toward the nearest enemy, faces it,
   and alternates A/B. While `CONTINUE?` is up (P1 credit state `0x06098674 == 3`) it
   queues the same COIN/START1 events a timeline uses. It never writes game state.
   With the variable unset, every P4.3 regression trace, PNG and dump is
   byte-identical.

The two runtime gates found past Scene 9 were missing static-recompile entries, not
emulation bugs. They are fixed in `port/seeds_extra.txt` (P4.4 blocks), not in the
runtime:

- `060359B2` (`func_022319b2`): a task step installed by data pointer. The 51 other
  `project/src_functional` functions absent from the generated table were seeded with it.
- `0604E928` (cart `0x0224a928`): a soft-float helper reached only through
  `mov.l @(pool),r0 … bsrf r0`, which the recompiler's bsrf resolver misses.
  `port/run/P44/tools/scan_unresolved_bsrf.py` checks for other cases of this pattern.

Rebuild with `python3 port/p41_build.py`. With the P4.3 input up to Scene 9 and
`STV_AUTOPLAY=7020`, P1 reaches Scene 51 in 90000 VBlanks with 0 FATAL. Details are in
[P4.4 progression](../../notes/port/p44_progression.md).

## P4.5 Real BIOS settings, BIOS input model, RBG0 parameter B and high resolution (2026-11-06)

Apply after the P4.4 chain above, in this order (both apply with `patch -p1`):

1. `p45-bios-single-cart-input.patch` (`runtime/src/machine/bios.cpp`): the BIOS input and
   credit model, decoded from the US BIOS (`epr-17952a`) and checked against MAME 0.285.
   - Every VBlank, publish the BIOS system-switch record at `0x06000728` (COIN1 `0x01`,
     COIN2 `0x02`, TEST `0x04`, SERVICE `0x08`, START1 `0x10`, START2 `0x20`, plus previous,
     press and release words at +2/+4/+6). The cart derives its own edge word `0x06098798`
     from it.
   - Remove the old per-frame force of `0x06098798 |= 0x34`. It reported TEST, START1 and
     START2 as pressed on every frame.
   - A coin only adds a BIOS credit (common chute, 1 coin = 1 credit). `0x06000664` takes a
     credit when the cart asks for one. A coin no longer sets the cart's START latch `0x0609880C`.
   - The `0x06000660` report uses the BIOS layout and copies 20 bytes, not 32. The extra
     12 bytes used to overwrite the cart's config bytes at `0x060987EC`.
   - `0x0600066C` copies the caller's 4 in-game bytes to the BIOS at `0x06000828`. It used to
     be treated as a panel read and overwrote `0x060987D4..`.
   - `0x0600060C` (exit to the BIOS menu) is logged. A single-cart cabinet reaches it only
     from the TEST switch.
   - `STV_AUTOPLAY` also inserts a coin and presses START when P1 has no game.
2. `p45-rbg-param-b-hires.patch` (`runtime/src/video/vdp2.cpp`, `runtime/tests/vdp2_test.cpp`):
   - RBG0 `RPMD = 2`: where parameter A's coefficient has its MSB set, the dot is drawn with
     parameter B, using B's plane map, plane size, screen-over and coefficient table. The old
     code made those dots transparent.
   - In high resolution (704 dots), a display dot advances half a rotation-screen dot. In
     double-density interlace, a display line advances half a rotation-screen line.
   - KTCTL/KTAOF fields: parameter B's fields are at bits 8-12 and 8-10, not 4-7. KDBS is
     bit 1, KMD is bits 2-3, and coefficient mode 3 replaces Xp.
   - Two new tests: `128 tests, 0 failed`.

Outside the runtime (main repository, not patches):
- `port/main_stv.cpp`: seed the BIOS values that MAME shows for a single cart in slot 0.
  These are `0x06000798 = 0x000808FD` (factory settings, bit5 = single-cart),
  `0x06000248 = 0x00004008` (USA) and the BIOS variables `0x06000650..0x0600065B`. The
  power-on boot reason 1 was already the default. `STV_CFG798` / `STV_CFG248` still override.
- `port/seeds_extra.txt`, P4.5 block: four more data-reached entries (`06014298`, `0601FC1C`,
  `06026EE2`, `0604E268`). `06026EE2` is first reached on the USA attract path.

The canonical route now uses no `STV_*` variables. `STV_BOOTMODE=0 STV_CFG798=40` (the old
multi-cart, post-START entry) still reaches combat, but it is no longer the reference route.
Details, MAME comparisons and repro commands:
[P4.5 BIOS and floors](../../notes/port/p45_bios_floors.md).

## P4.6 Real-time window, sound out, control panel from keyboard and gamepads (2026-11-06)

Apply after the P4.5 chain above, in this order (both apply with `patch -p1`):

1. `p46-realtime-window.patch` (front end; `runtime/src/host/host.cpp`, `host.h`, `host_null.cpp`,
   `settings.cpp`, `runtime/src/machine/saturn.h`, `bios.cpp`):
   - `SaturnConfig` gains `arcade` (an ST-V control panel instead of a Saturn pad), `unpaced`,
     `stats` and `title`.
   - `host_stv_panel()`: in arcade mode, the window's `[arcade]` keys and the first two gamepads
     (through `[gamepad]`'s up/down/left/right/a/b/c/start, plus `pad_coin`) give P1's and P2's raw
     panel words and the system switches (COIN1/2, START1/2, TEST, SERVICE). A key tapped and let go
     between two frames still counts for one frame. The null host returns zeros.
   - `bios.cpp` `stv_scene_probe_tick`: ORs the live panel into the same raw words (`0x06000708`,
     `0x06000710`) and system-switch record (`0x06000728`) that `--stvinput` feeds. A coin switch
     closing adds one BIOS credit (`deliver_coin`). `STV_AUTOPLAY` still plays P1 when the player
     leaves P1's controls alone. Headless, `host_stv_panel` returns zeros, so every headless run is
     unchanged.
   - Settings: `[display] integer = auto` (each line a whole number of pixels tall, 4:3, sharp
     `SDL_SCALEMODE_PIXELART`; auto = on for an arcade board), `[display] frameskip = true` (while
     the host is more than a field behind, leave out up to two pictures in a row so that game speed
     and sound hold), and the `[arcade]` key map. In arcade mode a gamepad's back button is the coin,
     not the menu (the guide button and Escape still open it), and P / Pause open the pause menu too.
   - `--stats N`: every N seconds, VBlanks per second, pictures left out and the audio queue's range.
2. `p46-speed-handoff-compose.patch` (`runtime/src/machine/machine.cpp`, `runtime/src/video/vdp2.cpp`).
   Speed only; the picture and the run are byte-identical (all P4.5 regression PNGs match):
   - The master and slave SH-2s already take strict turns on two threads (`give`). The waiting thread
     now spins on an atomic turn for about a millisecond before it sleeps on the condition
     variable. About 330 turns a frame each cost a futex sleep and wake-up before (about 6 ms a
     frame on a busy host). Whose turn it is does not change.
   - `vdp2_compose` composes bands of lines on up to 8 threads. Each line depended only on the
     read-only setup and the video memories, which nothing writes while the machine waits for the
     picture. Each band has its own back and line-colour screen state.

Outside the runtime (main repository, not patches):
- `port/main_stv.cpp`: `--window` (real time, sound out, panel from keyboard and gamepads; no
  default coin/start timeline, no VBlank limit), `--fullscreen`, `--scale N`, `--settings FILE`
  (default `~/.local/share/saturn-recomp/stv.ini`, written with the defaults on first run),
  `--unpaced`, `--stats N`, and `--virtual-input` for tests. Without `--window`, nothing changes.
- `port/p46_build_window.sh` builds `port/run/P46/build-sdl/saturn_stv` against SDL3; `port/play.sh`
  runs it.

Details, key map, speed numbers and verification:
[P4.6 real time](../../notes/port/p46_realtime.md).

## P4.7 MAME fidelity and scenario capture (2026-11-07)

Apply after the P4.6 chain, from `tools/saturn-recomp`, in this order:

1. `p47-scenario-capture.patch`: installs the reviewed scenario hook from
   `work/scenario_port_hook.patch`, now protocol v1.01. `STV_SCENARIO_PIPE` opts in
   to per-VBlank semantic records, 2 MiB work RAM and optional pixels. It is
   **headless only and off by default**. `STV_SCENARIO_MEMORY=0` and
   `STV_SCENARIO_VIDEO=0` suppress the respective payloads. The hook samples before
   VBlank-IN delivery and changes no guest memory. Adds the physical TEST token.
2. `p47-bios-state-input-phase.patch`: implements the BIOS eight-byte per-slot
   settings copy (`0x634`, ROM `0x4DC0`) and backup-area pointer (`0x63C`, ROM
   `0x372C`). Schedules physical panel pins at the VBlank boundary but scans them
   into BIOS records when the guest calls `0x610`, matching ROM `0x30114` /
   `0x31498`. Includes three BIOS service tests, **131 tests, 0 failed**.

The patches use `a/runtime/...` paths, as the preceding runtime patches do:

```bash
(cd tools/saturn-recomp && patch -p1 < ../../port/runtime-patches/p47-scenario-capture.patch)
(cd tools/saturn-recomp && patch -p1 < ../../port/runtime-patches/p47-bios-state-input-phase.patch)
cmake --build port/run/P41/build-generated --target saturn_stv -j4
```

`port/main_stv.cpp` also reproduces the IPL's broad `SEGA` work-RAM fills, the
full cart-block copy including ROM padding, the cart-header settings copy and
master register/stack state and 352-dot display-off hardware state at the real power-on entry. It does not assign an
RNG seed separately. The P4.6 speed patch is untouched. Whole-RAM equality and
field-for-field scenario equality remain unresolved; see
[P4.7 measurements and limits](../../notes/port/p47_mame_fidelity.md).

## P4.8 Native Intel macOS v1.00 (2026-11-08)

Apply after the P4.7 chain, from `tools/saturn-recomp`:

```bash
patch -p1 < ../../port/runtime-patches/p48-macos-arcade-launcher.patch
```

`p48-macos-arcade-launcher.patch` adds an optional `SaturnConfig::launch_info`
string for the harness's preselected ROM paths and an arcade launcher layout.
It edits the real `[arcade]` P1/P2 keys, coin, TEST, SERVICE and pause bindings,
plus `[gamepad]` and `pad_coin`, and hides the Saturn multitap choices for an
arcade board. The original Saturn launcher remains available for disc games.
No guest timing, device, renderer, compiler or thread scheduling code changes.

Harness/build changes live in `port/`: `p41_build.py --generate-only` and
`--headless`, `main_stv.cpp --launcher` / `--no-launcher`, the Homebrew SDL3
build and play scripts, an optional in-tree app bundle generator and the
portable headless schedule/hash runner. No Homebrew installation is performed.

The historical patch files were already absent from this copy's working tree
when P4.8 started; their tracked versions remain in repository history. This
new patch is incremental against the already patched P4.7 runtime in
`tools/saturn-recomp`, not against the upstream base alone.

Build, play, verification hashes, speed and limits:
[P4.8 macOS](../../notes/port/p48_macos.md).

## P4.9 player-facing upgrades (2026-11-08)

Apply incrementally after P4.8, inside `tools/saturn-recomp`:

1. `p49-audio-queue.patch`: defer device start until samples arrive, 30 ms lead,
   smoothed host playback-rate feedback, and 75 ms discontinuity recovery that
   replaces stale audio. Guest timing and WAV generation stay unchanged.

2. `p49-player-pad-layouts.patch`: independent saved P1/P2 arcade gamepad
   mappings, including coin, with selected-controller capture in the launcher.
   Existing common/model defaults remain effective until a player is edited.

3. `p49-game-settings.patch`: MAME-verified difficulty, initial vitality/lives
   and free play in the launcher; BIOS free-play panel and credit permission.
   The harness writes the measured BIOS settings bytes from the saved INI.
   Physical TEST / the BIOS menu-reload path remains unimplemented.

4. `p49-crt-display.patch`: optional CPU scanlines/RGB mask/horizontal blur
   over the game texture, off by default, and independent Auto/Sharp/Smooth
   scaling. Shared portable SDL3 window-capture helper for verification.

Verification and player instructions: [P4.9 upgrades](../../notes/port/p49_upgrades.md).

## P4.10 second player-facing round (2026-11-08)

Apply incrementally after all four P4.9 patches:

1. `p410-arcade-touch.patch`: suppress Saturn touch pad, touch input/menu
   button, and pause-menu touch column in arcade mode; preserve Saturn behavior.

2. `p410-game-settings.patch`: MAME-measured pause/violence soft DIP controls,
   persistent launcher fields and F3 cabinet pause switch (also headless PAUSE token).
   Harness writes the region flags in `port/main_stv.cpp`.

3. `p410-save-states.patch`: portable SH-2 continuation fields, full ST-V
   machine/device snapshots, versioned identity/checksum checks, safe slave
   unwind/resume, F5/F7 and launcher/pause-menu commands; two runtime tests.
   Requires the generated-code continuation pass in `port/p410_continuations.py`
   (both build wrappers invoke it), the harness wiring in `port/main_stv.cpp`
   and the compatibility fingerprint in `port/stv.cmake`. Existing Saturn
   task-state behavior remains separate.

Verification and usage: [P4.10 upgrades](../../notes/port/p410_upgrades.md).

## P4.11 fidelity and polish (2026-11-08)

Apply incrementally after all three P4.10 patches, inside `tools/saturn-recomp`:

1. `p411-scene-capture.patch`: default-off, read-only graphics diagnostics.
   `STV_CAPTURE_SCENES=FIRST:LAST` captures every matching director field;
   `STV_CAPTURE_FIELDS=FIRST:LAST` or a comma-separated field list optionally
   narrows it, or works alone. Retains ordinary device dumps, PNGs and
   `shown-N.bin` (1024x512 big-endian displayed VDP1 dots, double-density row
   coordinates). Ordinary dumps still contain the draw framebuffer.
2. `p411-vdp2-fidelity.patch`: full-range RGB555 expansion for palettes, direct
   RGB and back screen; NBG0/NBG1 horizontal/vertical line scroll and line zoom,
   compact table records and intervals at the real LSTA0/1 registers A0..A6.
   MAME 0.285 device fixtures are pixel-identical for these functions.
3. `p411-panel-directions.patch`: physical Up/Down/Left/Right map to the real
   BIOS raw words 80/40/200/100 for keyboard, pads and named timeline inputs.
   A/B/C and autoplay's raw decisions are unchanged. To preserve an older
   timeline's raw inputs, swap UP/DOWN and LEFT/RIGHT; the P4.11 verifier does
   this explicitly for historical regression comparisons.
4. `p411-scsp-key-rates.patch`: signed octave and FNS bit9 select all 64 envelope
   rate steps; KRS15 disables scaling, D2R0 holds. Five release probes match
   MAME's observed envelope monitor; runtime tests cover the corrected rates.

The ending's green image is the authored RBG0 GAME OVER zoom, also present in
MAME, and is preserved. `0601FC1C` was already seeded; fresh single/two-player
coverage confirms it. Internal 2x/3x rendering is investigated and deferred;
there is no new resolution setting. Default-route pixel/audio/input changes
are restricted to the separately evidenced fixes above.

Build with `port/p46_build_window.sh`; CPU-only offline probes with
`bash port/p411_build_probes.sh`. Findings, MAME fixtures, all-field ending
evidence, reference pixel audit and remaining differences:
[P4.11 fidelity](../../notes/port/p411_fidelity.md).

## P4.12 Windows x64 cross-build (2026-11-08)

`p412-windows-portability.patch` is incremental against the P4.10 runtime and
harness snapshot. **It contains both `runtime/...` and `port/...` paths.** Apply
it only in the isolated staging layout made by `port/p412_build_win.sh`:

```bash
port/p412_build_win.sh
# Internally, from the repository root:
# patch -p1 -d work/win/saturn-recomp < port/runtime-patches/p412-windows-portability.patch
```

The script copies the current runtime once into `work/win/baseline/runtime`
and `work/win/saturn-recomp/runtime`, copies the harness/build inputs into
`work/win/saturn-recomp/port`, and copies the generated program into
`work/win/recomp`. Subsequent runs reuse that snapshot. It never edits shared
`tools/`, existing `port/` sources, or shared generated code. This keeps the
concurrent P4.11 work independent. For manual staging, copy `main_stv.cpp`,
`stv.cmake`, and `p410_continuations.py` into that staging `port/` first.

Changes: missing `<cstdlib>` include; MinGW manifest resource embedding with
UTF-8 and per-monitor DPI declarations; Windows-only GUI target, optional
`--stats` console, AppData paths, and remembered ROM-folder selection. The
harness and `stv.cmake` changes deliberately exist only in this patch, not in
the main repository's original files. No guest execution, Linux/macOS path,
SDL gamepad mapping, or synchronization behavior changes.

Pinned downloads, build/package instructions, Wine/Linux hash comparisons,
launcher screenshot, disk usage and remaining real-Windows checks:
[P4.12 Windows](../../notes/port/p412_windows.md).

## P4.13 player features (2026-11-08)

Apply inside `tools/saturn-recomp`, incrementally after the four P4.11 patches:

1. `p413-rewind.patch`: optional P4.10 machine snapshots in a reversible XOR
   delta ring; 30 snapshots/s, up to 20 seconds / 512 MiB allocated deltas,
   remappable Backspace/pad hold, launcher and pause-menu controls. Loading
   resumes the original continuations; default history is off.
2. `p413-speed-turbo.patch`: host-only Tab hold / F4 toggle fast-forward with
   muted/flushed host audio, and independent 0..30 Hz held-button autofire for
   both players' A/B/C at the physical-panel boundary. Dedicated launcher tabs.
3. `p413-training.patch`: default-off health/lives/free-credit RAM controls and
   original next-scene table selection, validated actor pointers/join state,
   visible CHEATS marker/title and a small snapshot part for credit restoration.
   The invulnerability candidate failed gameplay validation and is not exposed.
4. `p413-input-files.patch`: checksummed/versioned panel recording and replay,
   power-on BIOS/RTC/settings metadata or embedded machine state, deterministic
   rewind branching, launcher entries/status and invalid-file errors.

Harness wiring and portable app/ROM/icon sources are in `port/`. The notes and
verification scripts use `work/p413/` and exclude the convergence job's trees.
`port/p413_verify_patches.py` reconstructs all changed runtime files byte for
byte, with zero fuzz/offset, from the pre-P4.13 source snapshot.

### Windows staging update

The **current** `p412-windows-portability.patch` has been re-exported against
P4.13, replacing its historical P4.10-only base. Continue to apply it **only in
`work/win/saturn-recomp`**, through `port/p412_build_win.sh`. The wrapper now
refreshes source snapshots with `port/p413_stage_win.py` when inputs change;
`work/win/p413-baseline` is the current unpatched base. Historical
`work/win/baseline` / `snapshot.json` remain preserved. Shared portable ROM
preparation and Windows frontend helpers are copied from `port/`, not embedded
as another fork of the conversion code. The Windows patch includes manifest/
resource setup, the existing missing standard include and Windows-only harness
startup/CMake glue. Icon resources use the shared, `WIN32`-guarded harness CMake.

Correctness, RAM citations, memory/frame cost, UI captures and explicit Wine /
full-speed verification limits: [P4.13 features](../../notes/port/p413_features.md).

## P4.14 enhanced internal resolution (2026-11-09)

Apply inside `tools/saturn-recomp`, after the four P4.13 patches:

```bash
patch -p1 < ../../port/runtime-patches/p414-hires.patch
```

`p414-hires.patch` adds opt-in 2x/3x CPU presentation rendering. The original
VDP1 hardware buffers, bus operations, command budget, registers and guest
scheduler remain authoritative. Separate presentation buffers receive scaled
polygon/distorted-sprite draws, nearest normal/scaled sprites and lines, mesh
in original pixel cells, and the existing colour/Gouraud operations. Four
workers own disjoint bands and consume only commands actually executed by the
original raster; all finish before the guest continues. VDP2 samples scroll and
rotation planes fractionally, with original line/window/priority/calculation
semantics. Enhanced-only palette/sprite/row caches and opaque-layer pruning
avoid redundant work. An exact enhanced-only frame cache compares video memories
and render registers and tracks displayed VDP1 revisions; it presents every
requested field and invalidates on writes/erase/draw/restore. 1x is the saved default and bypasses enhanced rasterization.

Launcher **Display and sound**, and launcher/pause **Player features / training
→ Time / autofire**, expose **Internal resolution**. The harness also accepts
`--internal-resolution 1|2|3`. CRT keeps its original pixel/scanline footprint.
Both companions are included in machine snapshots, without replaying a newer
command/texture list. Source fingerprints now include raster `.inc` files
(the harness/CMake changes are in `port/`).

The Windows-only P4.12 patch is re-exported against P4.14. Apply it only through
`port/p412_build_win.sh` in isolated staging, as before. The existing P4.13-named
staging/marker paths now contain current P4.14 inputs; hash changes refresh them.
`port/p414_verify_patches.py` reconstructs the P4.13 + P4.14 chain and Windows
staging with zero fuzz/offsets.

**Performance acceptance remains open:** this saturated host does not establish
60 fields/s at 2x; 3x is explicitly labelled CPU intensive. Exact default
regression evidence, resolution/state equivalence, inspected comparisons,
measured fields/s/CPU use, and portability limits are in
[P4.14 enhanced resolution](../../notes/port/p414_hires.md).

## P4.15 performance and Retina launcher (2026-11-09)

Apply incrementally after P4.14, inside `tools/saturn-recomp`:

```bash
patch -p1 < ../../port/runtime-patches/p415-retina-launcher.patch
patch -p1 < ../../port/runtime-patches/p415-performance.patch
```

The Retina patch wraps **both** launcher and in-game ImGui draws with the
current draw data's framebuffer scale and restores the prior SDL renderer
scale. At 1x with the usual renderer scale, the wrapper only forwards the draw.
The existing Windows PerMonitorV2 manifest remains in the isolated P4.12 patch.
The software SDL probe exercises clipping and geometry at 1x/1.5x/2x without
requiring a Retina display; actual mixed-display macOS/Windows checks remain
with the PM.

The performance patch preserves the original guest safe points and handoff
schedule. ST-V's two SH-2 contexts use the runtime's existing portable minicoro
implementation on one host thread; other Saturn configurations retain their
threaded path. The generated empty-slave-loop helper batches only complete
iterations **before** the next original poll, retaining the exact idle counter,
flag, registers and remaining budget. Trace/store-watch modes disable batching.
`port/p415_idle_loops.py` fingerprints the entire original generated function
and refuses unfamiliar code. Linux, Mac and Windows build wrappers run it
idempotently after P4.10's continuation pass; the generated code is still owned
by `port/run/P41/recomp`, not the separate functional-decomp job.

VDP2 applies the existing exact palette/opaque-layer/row and unchanged-frame
caches at 1x too, tracks hardware framebuffer revisions for cache invalidation,
uses exact masks/shifts for power-of-two map dimensions, and reuses sleeping
band workers with private mutable line state. VDP1 raster stepping, guest
interrupts, SCSP/DSP/68000 arithmetic and presentation cadence remain unchanged.
`SATURN_PROFILE=1` prints exclusive host thread CPU nanoseconds and elapsed
nanoseconds by component, cumulatively every 600 fields and at shutdown. These
are host diagnostics, outside snapshots. Hardware instruction/cycle access was
blocked on this host; the report does not relabel CPU time as hardware cycles.

Release builds already used `-O3`. Harness CMake now selects `x86-64-v2` for
Linux/Windows x64 Release (no AVX/AVX2), with
`-DSATURN_STV_X86_V2=OFF` available for older x64 CPUs. macOS retains plain
x86_64. Compiler flags/build scripts join the exact-build state fingerprint.
No fast-math or LTO dependency is added. The Windows-only portability patch is
re-exported against P4.15 and still applies **only** in isolated staging through
`port/p412_build_win.sh`; the owner ZIP is refreshed there.

`port/p415_verify_patches.py` checks the complete incremental runtime and Windows
staging byte for byte, with zero fuzz/offset. Build `port/p46_build_window.sh`;
run the ordinary runtime tests and the `port/p415_verify_*` helpers. Exact
profile methodology, measured gains, unchanged PNG/trace/WAV/full-state evidence,
platform limits and next steps: [P4.15 performance](../../notes/port/p415_perf.md).

## P4.16 two-player UDP rollback netplay (2026-11-09)

Apply incrementally after both P4.15 patches, inside `tools/saturn-recomp`:

```bash
patch -p1 < ../../port/runtime-patches/p416-netplay.patch
```

The patch adds a nonblocking IPv4 UDP peer session (POSIX / Winsock, existing
`ws2_32` linkage), protocol/ROM checks, fragmented reliable power-on origin
transfer, numbered redundant panel inputs, repeat-last prediction and an
8-field bounded rollback history using the P4.10/P4.13 snapshot parts and
portable continuations. Corrections unwind through `QuickLoad`; intermediate
fields run devices/interrupts without composition, presentation or pacing.
Only the final corrected field is shown. Confirmed state CRCs detect desyncs;
errors stop the run. Network state stays outside machine snapshots.

Only confirmed PCM reaches the host or WAV. A fixed eight-field confirmation
lead plus a 60 ms device lead absorbs ordinary correction bursts; the netplay
host audio path keeps queued confirmed samples instead of applying the offline
75 ms queue-reset policy. Rewind, quick states, input movies, pause, turbo,
cheats, speed changes and enhanced rendering are disabled during sessions.
The offline path retains its original queue limits and timing.

The launcher has a Netplay tab; the HUD shows status, ping and rollback totals /
maximum depth. Harness CLI, portable source fingerprint and verification tools
are in `port/`. Continue using the normal Linux/macOS build wrappers. The
Windows-only P4.12 patch is re-exported against P4.16 and still applies **only**
in isolated staging through `port/p412_build_win.sh`; its owner ZIP is refreshed.
`port/p416_verify_patches.py` reconstructs both chains without fuzz/offsets.

The sandbox denies even loopback UDP sockets. Two-process impaired-datagram
verification therefore uses the explicit, test-only file-backed transport in
`port/p416_datagram_shim.c`; it does not establish kernel socket or internet
connectivity. Live UDP, Mac timings, speakers and Wine/Windows execution remain
PM checks. NAT traversal, a relay and automatic full-state resync are absent. The eight-field
cap does not guarantee a 16.7 ms recovery; the measured worst-case budget is unmet.

Design, host/join and forwarding instructions, measurements, determinism and
remaining acceptance gaps: [P4.16 netplay](../../notes/port/p416_netplay.md).

## P4.17 Linux release, integrated over P4.16 by P4.18 (2026-11-09)

Apply incrementally **after `p416-netplay.patch`**, inside `tools/saturn-recomp`:

```bash
patch --batch --fuzz=0 -p1 < ../../port/runtime-patches/p417-linux-release.patch
```

The re-exported patch contains only `runtime/src/host/host.cpp`: Linux release
builds default to 1280×800 and fullscreen under `SteamDeck=1` / `SteamAppId`.
It is guarded by Linux and `SATURN_P417_RELEASE`; other builds retain their
existing window behavior. Final P4.16 launcher/netplay sources remain intact.

The opt-in ROM browser and argument preparation now live in the repository's
`port/p417_linux_frontend.h`, wired through `port/main_stv.cpp` and
`port/stv.cmake` (`SATURN_STV_LINUX_RELEASE=ON`). These repository files are
not runtime patches. The release builder snapshots the **current integrated
sources**, refreshes on input hashes, and never reapplies the runtime patch.
The historical partial-P4.16 snapshot is preserved only for audit.

`port/p417_build_linux.sh` rebuilds official SDL3/AppImage/tar; the Windows-only
P4.12 patch is re-exported against this integrated tree and remains isolated
in `work/win`. `port/p418_verify_patches.py` reconstructs both patch chains
byte for byte with zero fuzz/offsets. Pins and release usage remain in
[P4.17](../../notes/port/p417_linux_release.md); current verification and artifact
hashes are in [P4.18 integration](../../notes/port/p418_integration.md).

## P4.19 speculative netplay audio (2026-11-09)

Apply `p419-speculative-audio.patch` after P4.17, inside `tools/saturn-recomp`.
Low latency is the saved default; Glitch-free retains confirmed-only playout.
A mutex-protected absolute PCM timeline feeds SDL on demand, replaces only
unsubmitted rollback differences and crossfades for 3 ms without clearing or
restarting the device. The ordinary WAV remains the confirmed stream; optional
`STV_NETPLAY_PLAYED` captures device-input PCM separately. Offline output is
unchanged. Harness CLI/test drivers live in `port/`. Design and evidence:
[P4.19 netplay polish](../../notes/port/p419_netplay_polish.md).

Then apply `p419-nat-traversal.patch`: portable built-in PCP/NAT-PMP and UPnP
IGD leased UDP mappings, configurable STUN, compact connection codes and
simultaneous UDP punching on the game socket. The launcher has Host / Join by
IP / Connect by code, SDL clipboard controls and discovery/error status.
Mapping renewals use a separate socket; exit removes the lease. Windows adds
only the system `iphlpapi` link for gateway discovery. Discovery runs only when
requested by netplay. Runtime tests include published STUN vectors and injected
router responders; `port/p419_verify_traversal.py` also exercises production
socket I/O and complete code/direct game pairs using local file-backed fixtures.
Live-router/public-STUN/internet and native Windows/macOS checks remain PM work.

P4.19 also repairs `p416_verify_netplay.py --transport udp`: zero impairment
uses direct kernel loopback; nonzero cases automatically own a
`p416_udp_proxy.py` process. UDP never builds/loads a shim. Explicit
`--transport files-proxy` verifies that same proxy through file-backed sockets
when the sandbox blocks UDP. Final evidence, 181 runtime tests, seven audio
edge-case tests, the 9,000-field audio/state matrix, refreshed owner artifacts
and exact real-socket PM commands are in the P4.19 note linked above.

## P4.22 live integration of P4.21 mods (2026-11-09)

The three `p421-mod-*.patch` files are now **rebased onto the final P4.19
sources**, superseding their original snapshot-only versions. Runtime order:

```sh
# Inside tools/saturn-recomp, after p419-nat-traversal.patch:
patch --batch --fuzz=0 -p1 < ../../port/runtime-patches/p421-mod-runtime.patch
# From the repository root, on the P4.19 harness:
patch --batch --fuzz=0 -p1 < port/runtime-patches/p421-mod-harness.patch
```

The loader preserves P4.19 speculative audio, traversal and the validated-Hello
origin gate. HELLO now carries the complete mod-set SHA-256; both direct peers
receive an explicit mod-set/hash mismatch before origin transfer. The selected
set joins the P4.19 portable fingerprint used by connection codes and probes,
even before native libraries load. Changing selection closes discovery and
invalidates old codes. Saves retain the exact set description and SDK state.
Host overlay lines now sit bottom-left above MODDED, clear of the health HUD.

A shipping hook target needs all **216 qualified adapters**. The reviewable
`port/mods/qualified-base.patch` plus `.json` preserve P4.21's qualified generated
output as a pinned deployment delta against stock P4.19 generated code. This
avoids a build dependency on `work/p421/` or modification of the separate
convergence job. `port/p422_prepare.py` checks every changed stock input, applies
that payload, then applies `p421-mod-convergence.patch`, with zero fuzz/offsets,
into `port/run/P422/recomp`. The latter patch contains the loader bridge, full
C/recipe/catalogue hashing, a portable standard `uintptr_t` prelude for LLP64,
and Linux-only exit wrapping. Neither generated program is instrumented twice.

The ordinary Linux/window/macOS/Windows wrappers all run this deployment step;
Linux and Windows stage the resulting current program. Do not apply the
convergence patch against stock generated code or reapply any runtime patch to
the already integrated live tree. The isolated Windows portability patch is
rebased again and applied **after** the mod runtime/harness in `work/win` only.

Releases ship `modding/` with the SDK, three example sources, build script and
their own `p421-target.txt`; no compiled examples or enabled selections. The
macOS bundle places it in `Contents/Resources/modding`. Build native libraries
on the target platform, including `.dylib` on the PM's Mac. Default mod folders
are Linux `$XDG_CONFIG_HOME/saturn-recomp/mods` (otherwise
`~/.config/saturn-recomp/mods`), macOS
`~/Library/Application Support/Die Hard Arcade/mods`, and Windows
`%APPDATA%/Die Hard Arcade/mods`. `--mods-dir` is the explicit override.

Reconstruction: `python3 port/p422_verify_patches.py`. Complete evidence,
artifact hashes, reproduction commands and platform limits:
[P4.22 integration](../../notes/port/p422_mods_integration.md).

## P4.24 tear-free presentation (2026-11-11)

Apply `p424-vsync.patch` inside `tools/saturn-recomp`, incrementally **after
`p421-mod-runtime.patch`** (P4.22 integration):

```sh
patch --batch --fuzz=0 -p1 < ../../port/runtime-patches/p424-vsync.patch
```

The launcher Display setting **Sync to display (VSync)** saves `[display]
vsync = true` by default, including for old files that omit the key. The host
prefers `SDL_RENDERER_VSYNC_ADAPTIVE`, falls back to interval 1, and preserves
unsynchronised presentation when off. One gate handles game, network-wait,
rewind, pause and both ImGui loops. Hidden/occluded/minimised windows suspend
sync; three consecutive presents over 40 ms trigger unsynchronised recovery,
30 normal visible presents and a 2–30 second retry backoff. Visibility recovery
also requires 30 normal presents and at least 500 ms. Network-wait redraws and
explicit unpaced/resume/fast-forward game presentation bypass sync.

Guest-time absolute pacing and audio ratio control are unchanged. Frameskip
credits at most one field of measured synchronized-present time. The policy
and clocks are host-only and never enter machine snapshots, guest pixels,
traces or WAVs. A synchronous SDL driver call cannot be cancelled safely;
recovery acts on slow **returns**, with native compositor testing still needed.

Use the ordinary Linux/window/Windows/Mac wrappers; no extra Mac patch or
platform fork is needed. `port/p424_verify_patches.py` reconstructs the complete
runtime/Windows chain with zero fuzz/offsets. Tests and release evidence,
including the sandbox's Xvfb socket restriction and PM native checks, are in
[the P4.24 note](../../notes/port/p424_vsync.md).
