# P4.24 — tear-free presentation

2026-11-11. Base HEAD `bd57aca`; no commit. Default-on VSync, visibility
suspension, slow-present recovery, tests and rebuilt Linux/Windows artifacts
are implemented. **Physical Metal/D3D presentation and native compositor stall
acceptance remain PM checks. Xvfb could not start inside this sandbox.**

All paths in this note are repository-relative. Evidence and media live only
under `work/p424/`. The initial runtime and executable are preserved in
`work/p424/baseline/runtime/` and `work/p424/baseline-stv`. The pre-existing
`notes/pm_board.md` modification was left alone. No convergence tree, functional
decompilation source, GPU, network, sudo or unrelated process was used.
`/home` had 256 GiB free initially and 255 GiB after verification, above the
15 GiB stop threshold. Hardware capture logging stayed off.

## Setting and timing

The launcher **Game / controls → Display and sound → Sync to display (VSync)**
is checked by default. It saves the boolean `[display] vsync = true`; old
settings files that omit the key inherit true. Switching it takes effect in
the launcher immediately and persists through reopening. Setting it false
restores unsynchronised presents, the original absolute guest-time pacing and
the original frameskip threshold. No new CLI switch is needed:

```ini
[display]
vsync = false
```

`host.cpp` requests `SDL_RENDERER_VSYNC_ADAPTIVE` first, then interval 1 if the
renderer refuses adaptive. If both fail, it logs the unsupported mode once
and continues unsynchronised. It does not retry an unsupported backend every
frame. The real software renderer here accepts interval 1; the test fixture
also exercises successful adaptive negotiation and complete rejection.

The new `presentation.h` policy, measured present duration, visibility state,
retry timers and log state are exclusively host data. No device, guest
scheduler, PCM generation, renderer pixel arithmetic or snapshot part changed.
`host_pace()` still calls the original `pace_wait(base, guest_now, host_now)`.
A present that consumes 12 ms consumes that much of the next absolute wait;
no per-frame sleep is added to the guest deadline and no VSync duration is
added to `g_base`. The existing greater-than-100-ms discontinuity rebase and
the host audio queue/frequency-ratio controller are unchanged. The display's
60 Hz clock does not replace the guest's approximately 59.83 Hz raster clock.

Frameskip adds a credit equal to the previous synchronized present's measured
blocking time, capped at one guest field. This prevents ordinary refresh
waiting from being treated as a new field of compute backlog. Off,
unsupported and recovery modes have zero credit. True sustained overload
still reaches the original bounded skip policy.

Explicit `--unpaced`, fast-forward and resume game presentation bypass VSync
so those operations retain their original purpose. Their bypass is logged on
transition, rather than once per frame. Launcher/menu presentation normally
uses VSync regardless of whether guest pacing has started.

## Stall fallback and coverage

All actual `SDL_RenderPresent` calls now pass through `host_sdl_present()`:

| Path | Treatment |
|---|---|
| Ordinary game `draw()` / `host_present()` | Setting and recovery policy; guest pacing remains separate. |
| `host_net_wait()` | `draw(false)` uses unsynchronised presents, so redraws do not wait for refresh while polling a peer. |
| Pause menu | Shared `draw()` gate, including visibility and slow-return recovery. |
| `host_rewind_frame()` | Shared `host_present()` / `draw()` gate. |
| Launcher loop | Shared gate; its SDL window events update visibility. |
| ImGui player-feature menu | Shared gate and window-event handling. |

The gate checks `SDL_WINDOW_HIDDEN`, `SDL_WINDOW_MINIMIZED` and
`SDL_WINDOW_OCCLUDED` before presenting. The game retains its existing hidden
window draw suppression. The launcher and other loops also suppress hidden
presents while sync is requested. SDL hidden/shown, minimized/restored,
occluded/exposed events refresh the policy, and the pre-present flag check
covers changes between event processing and drawing. Focus loss alone does
not disable VSync for a still-visible window.

Three **consecutive** synchronized returns over **40 ms** immediately select
interval 0 after the third return. A normal return breaks that sequence.
Recovery requires both the retry deadline and **30 consecutive normal visible
unsynchronised presents**. Initial slow-return cooldown is 2 seconds;
repeated failures double it through 4, 8, 16 and a 30-second cap. A visibility
suspension additionally requires 30 normal visible presents and at least
500 ms before retrying; it preserves a longer existing slow-return cooldown.
Hidden iterations do not count toward recovery. Mode changes log once each,
including initial adaptive/on, suspension, recovery and reactivation.

**Synchronous-call limit:** SDL renderer calls remain on the owning thread,
as required by the Metal/main-thread renderer path. A call already blocked
inside the graphics driver cannot be safely cancelled by this policy.
Detection acts on **returned** durations. Visibility suppression prevents
known hidden-window presents, and the slow-return policy protects subsequent
presents, but an indefinitely hung first driver call can still stall gameplay
and exceed netplay's existing timeout. No cross-thread SDL mutation, forced
thread cancellation or claim of an unconditional timeout guarantee is made.
A strict guarantee against that failure would require a larger separation of
guest execution from main-thread presentation. Native compositor stress
acceptance remains open for this reason.

## Verification

Aggregate machine-readable results: `work/p424/verification-report.json`.
Reproduction helpers are `port/p424_verify.py`, `p424_verify_present.py`,
`p424_verify_patches.py` and `p424_report.py`.

| Check | Result / evidence |
|---|---|
| Runtime suite | **186 tests, 0 failed**; `work/p424/runtime-tests.log`. |
| Presentation / pace / settings under ASan+UBSan | **21/0**; `work/p424/sanitized.log`. LeakSanitizer disabled because this sandbox runs under ptrace; no leak result claimed. |
| Fake adaptive / interval-1 / unsupported backend | PASS; production policy unit tests include negotiation, off, visibility and network-wait bypass. |
| Slow-return policy | PASS; consecutive/reset behavior, 40-ms threshold, cooldown, normal-visible recovery and increasing retry delay. |
| Simulated blocking 60 Hz present | **36,000 fields**, about ten minutes; absolute pacing base never changes, phase stays within one refresh and frameskip stays false. `presentation_test.cpp` uses the production policy and `pace_wait()`. |
| Production gate with injected 50-ms presents and actual SDL hide/show | PASS for game and launcher; `work/p424/present-verification.json`, `fake-game.log`, `fake-launcher.log`. Both log adaptive → recovery → hidden → recovery → adaptive exactly once per transition. |
| Checkbox / save / reopen | Actual SDL mouse events uncheck the default-on box, save `vsync = false` and reopen off; inspected `work/p424/launcher.png` and `launcher-off.png`. All controls still fit at 960×720. |
| Unpaced path | Only initial on and bypass transitions, no per-frame mode churn; `work/p424/unpaced.log`. |
| Windowed option **off** | **56 PNGs, four complete/guest trace pairs, four complete WAVs identical** to all seven retained P4.11/P4.13–P4.16/P4.19/P4.22 reference manifests. Includes 9,000-field no-coin attract. `work/p424/window-off/report.json`. |
| Windowed option **on** | Same complete reference equality; `work/p424/window-on/report.json`. These are paced SDL window runs, not headless checks with an unused setting. |
| Actual packaged AppImage | All 56 PNGs, four trace pairs and four WAVs match those same references; both requested smoke hashes pass. `work/p424/appimage/report.json`. This release oracle is headless. |
| Exact machine state | Complete 4,758,796-byte on/off quick-state files are identical. All **23** parts, including mods, also match the pre-change executable's payload. Fixed RTC fixture only; no host pacing clock is frozen. |
| P4.19 proxy matrix | **0/50/100/150 ms**, 0/1/3/5% loss, ±0/10/20/30-ms jitter; each 900 fields and 16 checkpoints, full final state, PNG and WAV equal. `work/p424/netplay-proxy/`. |
| P4.19 audio shim matrix | Same four 900-field cases pass with default low-latency audio and VSync on; `work/p424/audio/`. |
| P4.19 setup/traversal | STUN/symmetric detection, PCP, NAT-PMP, IGD, chunked IGD, timed lease renewal/deletion, 900-field code/direct pairs and code-delay mismatch pass; `work/p424/traversal/`. |
| Protocol / session locks | Delay mismatch, injected desync, host-origin override and SDL quick-state/fast-forward locks pass; `work/p424/protocol.log`, `ui-locks.log`. |
| Netplay with presentation stalls | 900 fields, 100 ms / 3% / ±20, injected slow returns and hide/show on both peers: no desync/timeout; all 16 checkpoints/full state/PNG/WAV equal. `work/p424/netplay-stall/`. |
| Actual AppImage netplay | 900 fields each, 0 and 100 ms / 3% / ±20 through the P4.19 file-backed UDP proxy; same full equality. `work/p424/netplay-appimage/`. |
| Patch reconstruction | **99 runtime / 151 Windows files**, byte-identical, zero fuzz/offset; relocation and unreviewed guest-input rejection still pass. `work/p424/patch-verification.json`. |
| Package audit | AppImage/tar payloads independently extracted and compared with AppDir; ZIP integrity/payload checked; source-only mod kits and exact targets verified, no ROMs/test shims. `work/p424/release/report.json`. |
| Old Linux loader | Actual glibc 2.31 loader passes stock and identity-mod shot-1900; game max symbol 2.29, SDL 2.30. Same release report. |

The sandbox denies kernel sockets, including the UNIX sockets Xvfb needs.
The owned Xvfb process failed with “Failed to find a socket to listen on”; it
was stopped and its repetitive log reduced to the relevant errors in
`work/p424/xvfb/server.log`. **No Xvfb success, kernel UDP success, physical
VSync or native Windows/Mac execution is claimed.** Network results above
use the explicit P4.19 test-only file-backed transports and real proxy logic.
None of the fixtures is shipped.

Paced dummy/software results, sampled before the next present:

| Mode / 9,000-field route | Median fields/s | Sampled phase range, ms | First → last sampled phase, ms |
|---|---:|---:|---:|
| On / no-coin | 59.89 | -14.624 .. 9.841 | 9.841 → -2.033 |
| Off / no-coin | 59.84 | -13.516 .. 0.464 | -9.348 → 0.464 |
| On / game-over | 59.83 | -14.495 .. 5.922 | -5.516 → -3.958 |
| Off / game-over | 59.83 | -14.629 .. -2.443 | -10.110 → -3.731 |

Negative phase means the next guest deadline is still ahead. Across all four
routes, five-second samples range 59.73–60.49 fields/s and phase remains
bounded at -14.683..16.950 ms, without growing delay. Concurrent package
compilation was allowed to run. Off omitted two startup pictures per route;
on omitted two or three, confined to the first ten seconds. **No later
omissions** occur in these 22,200-field runs per mode. These data and the
zero-skip simulated refresh test support the frameskip interaction; they do
not claim that an overloaded computer never skips.

Requested shot-1900 hashes, with explicit `200:COIN,340:START1` input so window
and headless runs follow the same paid route:

- 1x: `9bd5386ea7439621c19674381086e8fd` — on, off and actual AppImage.
- 2x: `8f1d9a1f01a2b77cd9d4f82b22b71b37` — on, off and actual AppImage.

Complete on/off quick-state SHA-256:
`d2c4478c0ced1d16f125fe03b66273879a285633c6f4db5a6bee2f98736895e7`.
Shared baseline/on/off state-payload SHA-256:
`d10380145e3eaa402a906aec0fe683f74f2db0f9ac2fd949969d199242b1f0db`.
The baseline file differs only in its exact-build identity bytes 16..23;
changing source intentionally changes that identity. Toggling the setting
does not. Portable unmodded netplay identity remains **`194812a8221f778e`**
across ordinary Linux, Linux release and Windows.

## Builds, patch and artifacts

The runtime change is the new ordered
`port/runtime-patches/p424-vsync.patch` (**eight files**), applied after
`p421-mod-runtime.patch`. Its SHA-256 is
`9246a6ce87e7d79022881d35db4106a3f75ba574825d82a528e229da6fdfb004`.
The runtime-patches README records the order and behavior. Do not reapply
patches to this already integrated live runtime. Windows portability remains
its existing isolated overlay, with no changed Windows-specific edits.

All requested rebuild commands succeeded, using cached dependencies only:

```sh
DHA_OFFLINE=1 bash port/p46_build_window.sh
DHA_OFFLINE=1 JOBS=6 bash port/p412_build_win.sh
DHA_OFFLINE=1 JOBS=6 bash port/p417_build_linux.sh
# The Linux builder invokes the packager; rerun alone only if needed:
DHA_OFFLINE=1 python3 port/p417_package_appimage.py
```

| Refreshed owner artifact | Bytes | SHA-256 |
|---|---:|---|
| `work/p417/DieHardArcade-x86_64.AppImage` | 5,573,112 | `4593e0e8605d7c2966ee566cbdb43c6a5a55041ff2f168d347e2bf536f2bc776` |
| `work/p417/DieHardArcade-x86_64.tar.gz` | 5,119,781 | `155b49393c5f1492a2d2cccde10e232a1117909c02f9428d7a02fddeb13d7bc2` |
| `work/win/DieHardArcade-windows-x64.zip` | 5,739,781 | `0b34159146b4313911cc361f1157a3c29ab26776039cfea4bb5f614a6a0e5dcd` |

Ordinary window executable SHA-256:
`e2614daa1412a486b3d1722abddb03b2f62df166ee23d5d7d2af4e51c96a124a`.
Linux release executable SHA-256:
`dc4f2a9258101ee57b12ec4f65949c04d4fd1789b2f7a1ca3a636a1f675d5c15`.
Windows executable SHA-256:
`7c507f87b6bf493cccd35367f2f8918a71e1e69727f127431161458b45579888`.

Verification commands:

```sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p424/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p424/runtime-tests -j6
work/p424/runtime-tests/runtime_tests
python3 port/p424_verify.py --sync on --tag window-on
python3 port/p424_verify.py --sync off --tag window-off
python3 port/p424_verify_present.py --netplay
python3 port/p424_verify_patches.py
python3 port/p424_verify.py --sync on --tag appimage --checks regression,smoke --headless --binary work/p417/DieHardArcade-x86_64.AppImage
python3 port/p416_verify_netplay.py --window --transport files-proxy --frames 900 --cases 0:0:0,50:1:10,100:3:20,150:5:30 --tag ../p424/netplay-proxy
python3 port/p419_verify_audio.py --window --frames 900 --tag ../../p424/audio
python3 port/p419_verify_traversal.py --frames 900 --tag ../../p424/traversal
python3 port/p424_report.py
```

`p424_report.py` checks all retained historical manifests, complete on/off
states, the preserved baseline payload and current owner artifact hashes.
It requires the baseline evidence retained here. Release extraction and
intermediate comparison copies were removed after their inventories passed;
reports, logs, primary comparison outputs and owner archives remain.

## PM checks on Mac and Windows

1. Sync the new runtime files or apply the ordered patch once to the integrated
   P4.22 runtime. Both `port/p48_build_mac.sh` and `port/p411_build_app.sh`
   already use that same runtime, qualified P422 generated program and shared
   CMake fingerprint. They need no new platform branch or VSync build flag.
   Rebuild both on the Mac; no macOS binary was produced on Linux.
2. On the owner's Intel i9 / built-in 60 Hz Retina / SDL Metal renderer,
   confirm the checked default with an old settings file missing the key.
   Play the flash-heavy scenes, compare on/off, fullscreen/windowed and Retina
   scaling. The log should say adaptive, or on if Metal rejects adaptive.
   Verify the saved checkbox state after reopening the app. Repeat on native
   Windows/D3D with the refreshed ZIP and at fractional DPI.
3. During both launcher and gameplay, minimize, completely cover the window,
   switch macOS Spaces / Windows desktops, lock/unlock and restore. Repeat
   with a native network peer still running. Verify continued peer progress,
   no timeout, suspension/recovery/reactivation logs without per-frame spam,
   and synchronized presentation returning after visibility stabilizes.
   Stress the known compositor-stall scenario explicitly; the first-call
   cancellation limit above prevents claiming this result from unit tests.
4. With `--stats 5`, run several minutes on each physical renderer. Confirm
   roughly 59.83 guest fields/s, bounded phase/audio queue and no sustained
   frameskip caused by ordinary refresh waiting. Listen for audio continuity.
   Check a 120 Hz or variable-refresh display if available; repeat with
   adaptive refusal so interval 1 is exercised.
5. Repeat the smoke hashes and real kernel-UDP 0/100-ms netplay tests on native
   Mac/Windows. The historical file-shim tests here do not replace those
   platform runs. A native mod must be rebuilt against its package's updated
   exact target, as with other source changes.
6. Outside this socket-restricted sandbox, repeat the paced software runs
   under an owned Xvfb server using `port/p424_verify.py --x11 --sync on
   --tag xvfb-on` and the corresponding off run. This uses the caller's
   `DISPLAY`; keep `SDL_RENDER_DRIVER=software`. Xvfb itself may not reproduce
   physical refresh synchronization, so retain the simulated-present test.

## Ten-line summary

1. Added saved default-on “Sync to display (VSync)” in launcher Display settings.
2. Prefer adaptive VSync, fall back to interval 1, and preserve original unsynchronised behavior off.
3. One presentation gate covers game, network wait, rewind, pause and both launcher draws.
4. Hidden windows suspend sync; three slow returns trigger bounded-backoff recovery after normal visible presents.
5. Guest-time pacing and audio ratio control remain authoritative; frameskip credits at most one refresh wait.
6. Runtime tests pass 186/0; simulated ten-minute blocking presentation has no drift or skips.
7. On/off retain all 56 reference PNGs, traces, WAVs, both smoke MD5s and complete identical quick states.
8. P4.19 audio/proxy/traversal/protocol/UI suites and injected-stall netplay pass through explicit file shims.
9. Rebuilt and audited Windows ZIP plus Linux AppImage/tar; exported the ordered eight-file runtime patch.
10. No commit; Xvfb sockets and native Mac/Windows physical/compositor checks remain open, including first-call hangs.

## PM addendum (2026-11-12)

- Windows package under Wine 11.21: rc 0, shot-1900 MD5 `9bd5386ea7439621c19674381086e8fd`.
- The build host has no Xvfb (not installed, no sudo), so the `--x11` runs were not possible; the SDL dummy/software runs above stand.
- Mac v1.07 (Intel i9, Metal) built from this runtime, version 1.07 (build 107), v1.06 kept. Smoke with vsync on and off: 1x `9bd5386e…`, 2x `8f1d9a1f…` both ways.
- Metal accepted interval-1 VSync (log: `display: VSync on`, not adaptive). The VSync-off window run held 59.82-59.83 VBlanks/s, 0 pictures left out, pace lag about -4 ms over 240 s.
- The VSync-on window run was stopped at the owner's request after 10 s: 59.63 then 60.13 VBlanks/s, 21 then 2 pictures left out, pace lag 11.5 then 19.5 ms. Too short to judge drift; **open: a several-minute VSync-on run on the Mac, the minimise/occlusion stall test, and the owner's visual check of the flashing scenes.**
