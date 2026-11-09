# P4.22 — live mod-loader integration over P4.19

2026-11-09. Base: committed `e600540` (final P4.19). No commit, downloads,
network access or GPU use. P4.23 and the separate convergence work were left
untouched. This resumes the interrupted in-place integration.

## What moved and how to rebuild

The P4.21 runtime and harness now live in `tools/saturn-recomp/runtime` and
`port/main_stv.cpp`. The three `p421-mod-*.patch` files were re-exported against
the final P4.19 baseline. Historical prototype patches remain under
`work/p422/original-*.patch`; the old `p421_stage.py` and `p421_export.py` now
direct users to the live integration instead of applying the rebased patches
to their obsolete snapshot.

Patch order on an unmodified P4.19 tree:

1. Apply `port/runtime-patches/p421-mod-runtime.patch` inside
   `tools/saturn-recomp` with `patch --batch --fuzz=0 -p1`.
2. Apply `port/runtime-patches/p421-mod-harness.patch` from the repository root
   with `patch --batch --fuzz=0 -p1`.
3. Run the original continuation and idle-loop passes on `port/run/P41/recomp`.
   `port/p422_prepare.py` deploys the pinned 216-adapter qualified program into
   `port/run/P422/recomp`, then applies `p421-mod-convergence.patch` there.
4. Apply the refreshed Windows portability patch only in its isolated
   `work/win/saturn-recomp` staging, after the mod runtime/harness.

The normal build wrappers perform their respective steps. Do not reapply
runtime patches to this already integrated checkout.

The shipping hook bridge needs the complete qualified program. A small bridge
patch against stock generated code would leave the callable adapters missing.
`port/mods/qualified-base.patch` and `qualified-base.json` retain the reviewed
P4.21 deployment as a hash-pinned text delta against the stock P4.19 generated
program. This removes the shipping dependency on `work/p421/` without changing
`port/converge/`, `work/converge/` or functional source. The deployment preserves
all 216 adapters: 208 nonpoll entries support native hooks; eight resumable
entries remain catalogue-only under ABI 1.

The convergence patch preserves complete C/recipe/catalogue hashing, registers
the loader bridge, uses standard `uintptr_t` for LLP64 Windows, and restricts
exit wrapping to Linux. The ordinary wrappers use the qualified output once;
they do not instrument continuations a second time. Checkout relocation is
allowed only for the generator's fallback `SATURN_RUNTIME` CMake path. All
other pinned bytes remain authoritative. A simulated macOS home folder (`Die Hard Arcade`, with spaces)
checkout deploys identically; a changed guest input still rejects.

## P4.19 conflicts resolved

- The Mods tab coexists with the final Game, Player features and Netplay tabs.
  P4.19 connection-code, router and speculative-audio controls remain present.
- The full sorted mod-set SHA-256 is appended to HELLO. A host checks it before
  authorizing origin transfer and sends the explicit reason to both peers:
  `mod set/hash mismatch (local <hash-prefix>, peer <hash-prefix>)`.
  The P4.19 validated-HELLO origin gate remains intact.
- The mod digest joins the portable P4.19 fingerprint used by codes and probes.
  The launcher computes the selected inventory's fingerprint before loading
  any native libraries. Changing an enable flag closes discovery and
  invalidates old codes. Code refusal explicitly names a game-build or
  mod-set/hash mismatch. The original portable base is retained separately.
- Save identity includes the set digest. The `mods` state part stores the full
  sorted ID/version/hash description plus each mod's 256 SDK bytes. Changed
  selections or native bytes reject an old state; rollback restores SDK state.
- The deterministic frame callback remains at VBlank-IN, including replayed
  hidden fields. P4.19's confirmed WAV and speculative device queue retain
  their existing paths.
- Host overlay lines now rise from bottom-left above MODDED. The frame-HUD
  example no longer covers the P1 health bar.

## Data folders and release contents

| Platform | Default mods folder |
|---|---|
| Linux | `$XDG_CONFIG_HOME/saturn-recomp/mods`, otherwise `~/.config/saturn-recomp/mods` |
| macOS | `~/Library/Application Support/Die Hard Arcade/mods` |
| Windows | `%APPDATA%/Die Hard Arcade/mods` |

`--mods-dir` is the explicit override. A custom settings path does not move
mods. No production path points at `work/p421`. Linux XDG, HOME fallback,
settings independence and override behavior were exercised through the actual
loader (`work/p422/paths/report.json`). Windows/macOS defaults are implemented
in their platform branches; their native execution remains a PM check.

All packages contain `modding/`: the public SDK/catalogue, three example C
sources and manifests, `build_mod.sh`, instructions and that package's exact
`p421-target.txt`. No native examples or enabled selections are packaged.
`p422_package_mods.py` is shared by Windows, Linux and both Mac bundlers. Mac
content resides in `Contents/Resources/modding`; both Mac build wrappers use
the qualified deployment. The native build script selects `.so`, `.dll` or
`.dylib` on the target platform. The PM builds Mac examples on the Mac.

The packaged Linux identity source was built with the packaged script and
target, loaded by the packaged application and verified under the actual
glibc 2.31 loader. Archive extraction inventories match the staged payloads.
The kit audit finds no compiled examples, selections, ROMs, saves or test shims.

## Verification on the final build

Evidence is under `work/p422/`; no gameplay media was added to tracked paths.
All final mod tests use window target
`51bc128da6a6cce3c566d993a7780e60b0a0a34b42ef80ffbc873152c91335bc`.
Earlier prototype-target results were repeated after rebuilding the examples.

| Check | Result and evidence |
|---|---|
| Runtime suite | **181 tests, 0 failed**; `tests-resume.log` |
| Runtime/Windows patch chain | **97 runtime and 149 Windows files**, byte-identical reconstruction, zero fuzz/offset; relocation and changed-input rejection pass; `patch-verification.json` |
| Mods off | **56 reference PNGs**, four complete/full and guest traces, four complete WAVs identical, including 9,000-field no-coin attract and game-over/second-game routes; `verify-final/report.json` |
| Loaded identity | Same **56 PNGs, four full/guest traces and four WAVs**; original-body auditing enabled; same report |
| Linux release oracle | All **56 PNGs, four full/guest traces and four WAVs** match every retained P4.11–P4.16 reference; no-coin and game-over routes included; `regression/linux/manifest.json` |
| Required smoke | 1x `9bd5386ea7439621c19674381086e8fd`; 2x `8f1d9a1f01a2b77cd9d4f82b22b71b37`; `work/p419/smoke/p422-window/report.json` |
| Dynamic identity ABI | **20,000** complete-context vectors, both CPUs and complete RAM unchanged; pre-load preview equals opened fingerprint; `abi-resume.log` |
| Existing convergence probes | All six pass; `convergence-probes.json` and `.log` |
| Loader/SDK/ZIP negatives | All **32 cases** pass; disabled libraries never execute; `loader-tests-final/report.json` |
| Actual launcher events | Enable persists, folder URI encodes spaces/Unicode, dropped deflated ZIP installs disabled; `ui-delivery/report.json` |
| Health behavior | All **1,412 joined-player samples** remain at 114 HP; ordinary minimum/VB1900 HP is 78; 492 pixels change only in health rectangle `[240,18)..[336,24)`; `health-delivery/report.json` |
| Save behavior | SDK HUD count 1500 saves; cold restore reaches identical VB1800 PNG/count; wrong set and changed native bytes reject; `sessions-delivery/report.json` |
| Direct modded peers | 2,100 fields, **36 matching checksums**, identical final PNG/WAV/full payload; reversed selection order; same report |
| Impaired modded peers | 1,200 fields, 30 ms / 1% / ±10 ms, **21 matching checksums**, equal PNG/WAV/full payload; 5/7 rollbacks, depth at most 8; same report |
| Direct mismatches | Different sets and same-ID changed native bytes make both peers exit 2 before origin transfer, with explicit mod-set/hash reasons; same report |
| Mod-aware codes | Matching reversed sets connect under 30 ms / 1% / ±10 ms; **21 matching checksums** and equal PNG/WAV/full payload. Different sets and changed native bytes change the code fingerprint and refuse before handshake; `codes/report.json` |
| P4.19 traversal | STUN/symmetric detection, PCP, NAT-PMP, IGD, chunked HTTP, timed renewal/deletion, code pair, direct mapped pair and delay mismatch all pass; `work/p419/traversal/p422-resume/report.json` |
| P4.19 launcher | Host, Join and Code flows, including real SDL clipboard actions, pass; `p419-ui.log` and `p419-ui/*.png` |
| P4.19 proxy | 1,200 fields, 30 ms / 1% loss / ±10 ms / 2% corruption, **21 matching checksums**, equal PNG/WAV/full payload; `work/p416/p422-proxy/report.json` |
| P4.19 low-latency audio | Four **9,000-field** pairs at 0/50/100/150 ms, 0/1/3/5% loss and 0/10/20/30 ms jitter: **151 checkpoints per pair**, entire final machine/PNG/confirmed WAV equal across peers and all four cases; `work/p419/audio/p422-resume/report.json` |
| P4.19 Glitch-free | 900 fields, 100 ms / 3% / ±20 ms, **16 matching checkpoints**, equal full payload/PNG/WAV; speculative path absent; `work/p419/audio/p422-glitch-free/report.json` |
| Package audit | AppImage/tar payload inventories equal; ZIP CRCs and staged inventory equal; source-only kits and exact targets match; packaged off/identity smoke passes with glibc 2.31; `release/report.json` |

All networking above uses the explicit **test-only file-backed transport**.
The proxy case exercises the P4.19 proxy over that same shim. No real UDP,
router, Internet or physical audio result is claimed. The release executables
do not link the shim. The long audio matrix ran concurrently on this shared
host. It reports seven speculative correction events, 1,816 unsubmitted stereo
frames corrected, 9,592 submitted differences and zero large adjacent-sample
threshold crossings. Empty device blocks remain measurable under impairment;
speaker audibility and stable performance are unmeasured here.

Opened and visually inspected the final captures:

- `work/p422/verify-final/example-none/window.png`
- `work/p422/verify-final/example-infinite-health/window.png`
- `work/p422/verify-final/example-frame-hud/window.png`
- `work/p422/verify-final/launcher/mods.png`
- `work/p422/ui-delivery/mods-after-drop.png`
- `work/p422/sessions-delivery/restore-window.png`
- `work/p422/p419-ui/code.png`

The longer green health bar is visible. HUD text reads
`FRAME 1900 | SCENE 3 | HUD TICKS 1900`, bottom-left above MODDED; restored
text reads `HUD TICKS 1800`. Both remain clear of the health band. The real
Mods and P4.19 Netplay tabs are readable and coexist.

## Artifacts and identities

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `work/win/DieHardArcade-windows-x64.zip` | 5,738,293 | `14c2c83f1df243062fc2862d9311a55312732546bb4b0f3a09c97e64ee458c2d` |
| `work/p417/DieHardArcade-x86_64.AppImage` | 5,569,016 | `aadb8df96a2811f7b1dfe187e557bf3afc44071558e8b29705daa5bd7ad4483a` |
| `work/p417/DieHardArcade-x86_64.tar.gz` | 5,117,696 | `71ee5ede5a26222322258f98b34b08e2e723a32553a8ad7c944b1ede643b0026` |

Window executable SHA-256:
`8d72ad03d316466036d7574ad677fa78b749d2a1c2fca8e664dfb5238da69b56`.
Linux release executable:
`e4dd68a5db32c5a7f9905116ba05557f43a69ff3a02dc156b09a2cf27020d811`.
Windows executable:
`056f8615dfb9045957e796367f544e0ec76a6939bc74391251e6160676dbfdf9`.

All three builds share portable netplay source identity `194812a8221f778e`.
Exact mod targets deliberately differ with compiler/platform/frontend:

- Window: `51bc128da6a6cce3c566d993a7780e60b0a0a34b42ef80ffbc873152c91335bc`
- Linux release: `2cbc5b69433c8dd594f79fe8c2ba6964054211cb438425edf576d6bd6b741f6e`
- Windows: `d356e7c11f9e96abf8472ebb7e5396920a96fdc5cd7d749a6e3f5664f63e10bc`

## Resume audit, reproduction and remaining platform work

The two empty `p422_verify_codes.py` / `p422_verify_paths.py` files were
reconstructed. Changed sources were checked for empty/truncated content and
Python/shell syntax; the full patch reconstruction and native/cross builds
validate the integrated source payload. The interrupted Linux SDL build was
rebuilt successfully. The ENOSPC-era 50 ms WAVs were shorter than their RIFF
headers declared (26,195,696 / 26,208,016 actual bytes versus 26,507,408 declared).
Those failed captures were discarded and the complete audio matrix rerun.
`work/p422/capture-integrity.json` verifies 201 PNGs, 29 WAV lengths and all
final audio-matrix artifact hashes/sizes. Its only empty files are successful
wrapper/extraction logs, not sources or evidence payloads.

Disk checks stayed above 50 GiB during this resumed run, above the requested
15 GiB stop threshold. Cleanup removed **867,283,136 bytes** of raw checkpoint
dumps, duplicate/interrupted audio and extracted archive copies after checking
(`work/p422/cleanup.json`); final free space was **61 GiB**. Future P4.21 oracle runs omit raw
VRAM/display dumps; selected PNGs, traces and WAVs remain the oracle. Hardware
access summaries are only a few KiB per run; no unbounded hardware trace was
enabled.

Reproduce from repository root with the existing local dependencies. In a
restricted workspace, use a writable XDG test directory:

```sh
export DHA_OFFLINE=1
export XDG_CONFIG_HOME="$PWD/work/p422/default-config"
bash port/p46_build_window.sh
python3 port/p422_verify_patches.py
python3 port/p422_verify_paths.py
cmake --build work/p422/runtime-tests -j6
work/p422/runtime-tests/runtime_tests
P421_WORK=work/p422 python3 port/p421_verify.py
P421_WORK=work/p422 python3 port/p421_verify_loader.py
P421_WORK=work/p422 python3 port/p421_verify_health.py
P421_WORK=work/p422 python3 port/p421_verify_ui.py
P421_WORK=work/p422 python3 port/p421_probe_build.py
P421_WORK=work/p422 python3 port/p421_verify_sessions.py
python3 port/p422_verify_codes.py
python3 port/p419_verify_smoke.py --tag p422-window
python3 port/p419_verify_traversal.py --tag p422-resume
python3 port/p419_verify_audio.py --parallel --window --frames 9000 --tag p422-resume
python3 port/p419_verify_audio.py --audio glitch-free --window --frames 900 --cases 100:3:20 --tag p422-glitch-free
python3 port/p416_verify_netplay.py --transport files-proxy --frames 1200 --cases 30:1:10 --corruption 2 --tag p422-proxy
bash port/p412_build_win.sh
bash port/p417_build_linux.sh
python3 port/p417_package_appimage.py
python3 port/p422_verify_release.py
```

`work/p422/{build,generated,runtime,port}` point to the live final inputs;
`build-stock` and `snapshot/` preserve the stock comparison inputs. The helpers
take `P421_WORK` so the original prototype evidence remains undisturbed.
Loader fixtures and peer packet folders reset between runs.

Native macOS builds, example `.dylib` compilation and app launch remain with
the PM. Both Mac shell recipes and bundler Python parse successfully, and the
relocated-checkout deployment is tested. Windows x64 cross-build/package pass;
native Windows execution, file-manager behavior and physical controls/audio
remain PM checks. The PM also runs real UDP. Native module determinism remains
the mod author's responsibility; exact native hashes prevent transparent
cross-platform mod matching. Historical exact-build saves are incompatible.

1. Integrated P4.21 runtime/harness into the live final P4.19 tree without a commit.
2. Re-exported ordered runtime, harness, convergence and isolated Windows patches.
3. Deployed all 216 qualified adapters from pinned source payloads without work/p421 dependencies.
4. Preserved P4.19 audio/traversal and joined mod hashes into HELLO, codes and save metadata.
5. Installed normal per-platform mod folders and packaged disabled source-only example kits.
6. Moved frame-HUD text above MODDED and inspected its clear placement and health effect.
7. Passed 181 runtime tests, six convergence probes, 32 loader cases and 20000 identity vectors.
8. Matched all 56 reference PNGs/traces/WAVs with mods off and identity, plus both required smoke hashes.
9. Passed modded sessions/mismatch/save/code tests and the P4.19 file-transport/audio/proxy suite.
10. Refreshed Windows ZIP and Linux AppImage/tar; native Mac/Windows and real UDP remain PM checks.

## Mac v1.06

2026-11-09, Intel i9 Mac (macOS 26.5, Apple clang, Homebrew SDL3). Only the Mac copy was
touched. P620 was read-only, and nothing was committed on either machine. Evidence is in
`work/p422mac/` in the Mac copy.

### Sync

- Backups first: the Mac's uncommitted v1.05 files are in `work/p422mac/pre-sync-mac-worktree.tgz`
  and the old runtime is in `work/p422mac/pre-sync-saturn-recomp.tgz`. Those v1.05 files equal
  P620's `ce23abe` (one blank line differs in `p416_netplay.md`).
- `git fetch ssh://p620/~/decomp/diehard-stv master` then `git merge --ff-only`: `8852d7f` moved to
  `e600540` (P4.19). No commit was made.
- The 38 uncommitted P4.22 paths came over by tar (list in `work/p422mac/synced-files.txt`). Excluded:
  everything `p423*`, `notes/pm_board.md`, and `work/`.
- `tools/saturn-recomp/runtime` was rsynced from P620, without `.git` or `*.orig`. It is now
  content-identical to P620's integrated runtime. No patches were reapplied, as this note says.
  `port/` also matches P620 except `run/`, `work/` and `p423*`.
- Version bump, done the same way as v1.05: `port/p411_app_mac.c` startup string `1.06`, and
  `port/p411_bundle_mac.py` `CFBundleShortVersionString 1.06 / CFBundleVersion 106`. These are local
  edits that are not committed.
- The previous v1.05 app was copied with `ditto` to `port/run/P411/Die Hard Arcade v1.05.app`.
  v1.02–v1.04 were not touched.
- Both build trees had stale CMake caches pointing at `port/run/P41/recomp`, and the build source is
  now `P422/recomp`. I deleted `CMakeCache.txt` and `CMakeFiles/` in `port/run/P48/build-window` and
  `port/run/P411/build-app`. Fresh checkouts don't hit this.

### Build

`bash port/p48_build_mac.sh` and `bash port/p411_build_app.sh` both returned rc 0
(`work/p422mac/build-{window,app}.log`). Unlike P4.16, there are now warnings:
1,137 `-Wunused-value` in 25 generated `P422/recomp/converge_*.c` files, from the
`((s32)dt_write(...))` statement casts. Another 11 are `-Wdeprecated-declarations` for `std::filesystem::u8path`
(`p421_mods.cpp`, `launcher.cpp:34`). Both are cosmetic.

App: `port/run/P411/Die Hard Arcade.app`. Info.plist reads 1.06/106 and the launcher log reads
`Die Hard Arcade 1.06`. `codesign --verify --deep --strict` passes. The only non-system Mach-O
dependency is `@rpath/libSDL3.0.dylib` (bundled). `Contents/Resources/modding` holds only the source kit
(sdk, three example sources/manifests, `build_mod.sh`, READMEs) and `p421-target.txt`.
Mac mod target is `a8d4c38eb0fdab45a2c532ed79ab3eb6ad27fc84ef581e8dce3cc6aba48e8b7e`, the same for the
P4.8 tree and the macOS-14 app. Portable netplay source identity `194812a8221f778e` equals the
Linux/Windows value.

| Artifact | SHA-256 |
|---|---|
| `work/p422mac/DieHardArcade-mac-v1.06.zip` (4,381,886 bytes; `ditto -c -k --keepParent`) | `b9a2900edea9bb406e3e0495e79d01d3a98c5b3043458a9da07780aace9655ac` |
| App `Contents/MacOS/saturn_stv` | `0a62c0e0ced588eaf59766aac0c75f3dc36b1a0ee26296e179b6dc971b2abaaf` |
| `port/run/P48/build-window/saturn_stv` | `05088b07249bd3beb835e201d9c216190161d20665c8f11eaa395e348365f363` |

The zip has no ROMs and no mod libraries. Its only dylib is SDL3.

### Verification

All app checks ran on a `ditto` copy in the session scratchpad. The environment was
`env -i HOME=<fake home> PATH=/usr/bin:/bin` with SDL dummy video/audio.

| Check | Result |
|---|---|
| Smoke, P4.8 binary: `python3 port/p419_verify_smoke.py --binary port/run/P48/build-window/saturn_stv --tag p422mac-p48` | 1x `9bd5386ea7439621c19674381086e8fd`, 2x `8f1d9a1f01a2b77cd9d4f82b22b71b37`, **PASS** |
| Smoke, bundled app binary (`--cart port/diehard_main.bin --cartrom cart.bin --internal-resolution N --vblanks 2000 --shots 1900`) | Same two md5s, rc 0, 0 FATAL, **PASS** (`work/p422mac/app-smoke/`) |
| Default mods folder (`--mods-list`, fake HOME) | `$HOME/Library/Application Support/Die Hard Arcade/mods`, **PASS** |
| Launcher via the bundle exe `DieHardArcade` (`SATURN_LAUNCHER_SHOT`, plus `SATURN_MODS_SHOT=1` / `SATURN_NETPLAY_SHOT=1`) | `work/p422mac/launcher/launcher-{main,MODS,NETPLAY}.png`, inspected. Tabs: Game / controls, Player features / training, Mods, Netplay. Mods lists the folder in Application Support, **Open mods folder**, the drop-ZIP hint, and the three built examples unchecked. Netplay shows Connection Offline / Input delay 3 / Netplay audio Low latency / Connection settings |
| Example mods built with the app's kit: `bash build_mod.sh examples/<m> <out> p421-target.txt` | All three `.dylib` built (`-Werror`, clean), in `work/p422mac/mods-built/` |
| Identity mod, smoke route (`--mods identity`) | shot-1900 `9bd5386e…86e8fd`, **bit-identical** |
| infinite-health, window shot at VB 1900 (`STV_AUTOPLAY=100 SATURN_WINDOW_SHOT… --window --no-launcher --unpaced --settings - --scale 3 --stvinput 99999:COIN`) | `work/p422mac/mods-window/infinite-health/window.png`, inspected. The P1 bar is full green with no red section (stock `none/window.png` shows the red loss), and the overlay reads `INFINITE HEALTH - joined players` above `MODDED (1)` |
| frame-hud, same route | `work/p422mac/mods-window/frame-hud/window.png`, inspected. `FRAME 1900 \| SCENE 3 \| HUD TICKS 1900` is bottom-left above `MODDED (1)` and **well clear of the P1 health bar**. The game framebuffer equals stock |
| Netplay, kernel UDP over 127.0.0.1, app binary, 2,100 fields, delay 3, two-player `--stvinput @work/p422mac/net/panels.txt` | **Unmodded:** both rc 0, 0 desync, shot-2100 `6a6706f8…927360` on both, 36/36 confirmed CRCs identical, 3/6 rollbacks, max 2. **Modded** (`frame-hud,infinite-health` vs reversed order): both rc 0, `e0d8874a…702801` on both, 36/36 CRCs identical. **Mismatch** (infinite-health vs none): both exit 2 before origin transfer with `NETPLAY STOPPED: mod set/hash mismatch (local e2225dfab811, peer e3b0c44298fc)` |

The P416 file transport is Linux-only (`LD_PRELOAD` shim), so the Mac pair used kernel UDP
on loopback, which the macOS firewall does not block.

Prebuilt examples: **not packaged** (source-only, as P4.22 prefers). No enabled selection ships.

### Issues for the PM

- The guest's mismatch message is the host's text verbatim, so on the guest "local" and "peer" are
  swapped: the guest has no mods (`e3b0…`) but sees `local e2225…`. Cosmetic.
- New compile warnings, all cosmetic (see Build): `-Wunused-value` in generated converge code
  (a `(void)` cast in the emitter, or `-Wno-unused-value` for those files, would fix it), and the
  `u8path` deprecation.
- Any run of the binary, headless smoke included, creates the default mods folder. On this Mac
  the real `~/Library/Application Support/Die Hard Arcade/mods` now exists, empty.
- Not checked here: Finder double-click / Metal window with real mods, the **Open mods folder**
  button (it uses `SDL_OpenURL`), drag-drop ZIP install on macOS, runtime unit tests on clang,
  and cross-machine UDP (PM).

## PM addendum (2026-11-09)

Checks run by the PM on the build host outside the Codex sandbox; log `work/p422/pm-runs.log`.

| Check | Result |
|---|---|
| Windows package under Wine 11.21, `--vblanks 2000 --shots 1900` | rc 0, shot-1900 MD5 `9bd5386ea7439621c19674381086e8fd` |
| Window build, real UDP, 9000 fields, `0:0:0,100:3:20` | PASS both, 151 checksums each, PNG/PCM/full machine equal |
| Actual AppImage, real UDP, 9000 fields, `0:0:0,100:3:20` | PASS both, 151 checksums each, PNG/PCM/full machine equal |
| Mac v1.06 (section above) | smoke 1x/2x, launcher, `.dylib` examples, modded/unmodded/mismatched UDP loopback all pass |

Known limits: modded netplay only matches between identical builds (native mod hashes differ per platform/compiler); unmodded cross-platform play is unaffected. Cosmetic follow-ups: the guest-side mismatch message reuses host wording (local/peer swapped); `-Wunused-value` noise from generated `converge_*.c`; `std::filesystem::u8path` deprecation warnings; every run creates the default mods folder.
