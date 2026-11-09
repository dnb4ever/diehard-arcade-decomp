# P4.18 — integrate Linux release over final netplay

2026-11-09. Base HEAD `8165d445231db85451179cc9e1585391a5374f92` plus the
uncommitted integration below. All paths are repository-relative. No commit,
network download, sudo, GPU, convergence-tree changes or distribution upload.
Pre-existing `notes/pm_board.md` and `tools/decomp/difftest_v104.py` changes
were left untouched.

## What moved where

| Destination | Integrated change |
|---|---|
| `port/p417_linux_frontend.h` | Original P4.17 external-ROM discovery, shared P4.13 converter, controller-accessible folder browser, remembered source and Linux data/cache paths. |
| `port/main_stv.cpp` | Linux release guarded header/startup call; explicit `--cart` keeps the original harness path. |
| `port/stv.cmake` | Linux-only, default-OFF `SATURN_STV_LINUX_RELEASE`; converter linkage, release definitions and frontend/converter state fingerprint inputs. |
| `tools/saturn-recomp/runtime/src/host/host.cpp` | Release-only 1280×800 default window, respecting explicit scale; SteamDeck/SteamAppId fullscreen defaults. |
| `port/runtime-patches/p417-linux-release.patch` | Re-exported **runtime-only** host delta, based on final P4.16; apply after `p416-netplay.patch` inside `tools/saturn-recomp`. |
| `port/runtime-patches/README.md` | P4.17 entry after P4.16, corrected ownership/application/build instructions. |

No partial-snapshot launcher code was imported. The final P4.16 Netplay tab,
host/join fields, protocol, rollback, confirmation and audio paths remain
unchanged. The only runtime file in the P4.17 delta is `host.cpp`.
Both frontend and window defaults require Linux **and** the release option;
macOS/Windows and normal default-OFF builds retain their previous behavior.
The Steam environment changes fullscreen only in that Linux arcade release.

`port/p418_stage_linux.py`, called by `port/p417_build_linux.sh`, inventories
current runtime, harness and generated sources before every build. Input/HEAD
changes refresh the snapshot; modified build copies are repaired. The old
one-time reuse and mixed-patch application were removed. The current manifest
contains **261 files**, including final P4.16 sources and the new header.
Repeated staging verified that it matches current sources; all 261 build-input
hashes also match the shared tree in `work/p418/source-audit.json`.
The original mid-P4.16 baseline and manifest are preserved solely for audit at
`work/p417/p417-original-baseline/` and `p417-original-snapshot.json`; neither
is used as build input. Dependency caches in `work/p417/opt` were reused and
their pinned hashes/signatures verified. Release flags/SDL pin remain P4.17's.

`p412-windows-portability.patch` needed re-exporting because the harness and
CMake context moved. Its added/removed code is **identical** to the prior
patch; only hunk locations/context changed. `work/p418/windows-portability-audit.json`
records that comparison. Windows staging continues to use current portable
sources through the existing P4.13 staging helper.

`port/p416_verify_netplay.py` gains only `--binary`, selecting the executable
for script recording and both peers; its test/protocol behavior is unchanged.
The default remains the ordinary windowed build. New P4.18 helpers reconstruct
the complete runtime/Windows patch chains and compare historical regressions
without overwriting their references.

## Build and verification

```bash
JOBS=6 port/p417_build_linux.sh
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p418/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p418/runtime-tests -j6
work/p418/runtime-tests/runtime_tests
JOBS=6 port/p412_build_win.sh
python3 port/p418_verify_patches.py
python3 port/p418_verify_regression.py --binary port/run/P46/build-sdl/saturn_stv --tag window
python3 port/p418_verify_regression.py --tag linux
python3 port/p418_verify_regression.py --binary work/p417/DieHardArcade-x86_64.AppImage --tag appimage
python3 work/p418/verify-release.py
APPIMAGE_EXTRACT_AND_RUN=1 python3 port/p416_verify_netplay.py \
  --binary work/p417/DieHardArcade-x86_64.AppImage --window \
  --cases 0:0:0,100:3:20 --tag ../p418/netplay-appimage
```

All execution uses CPU rendering, SDL dummy video/audio and/or the null host.
AppImage execution uses extraction because FUSE is unavailable.

| Check | Result / evidence |
|---|---|
| Runtime tests | **164 tests, 0 failed**; `work/p418/runtime-tests.log`. |
| Ordinary SDL build | PASS; `work/p418/build-window.log`. |
| Linux release + SDL + package | PASS; `work/p418/build-linux.log`. |
| Windows x64 cross-build/package | PASS; refreshed owner ZIP; `work/p418/build-windows.log`. |
| Runtime reconstruction | P4.13→P4.17 reconstructs **82 files byte for byte**, zero fuzz/offset; `work/p418/patch-check.log`. |
| Windows reconstruction | **134 files byte for byte**, zero fuzz/offset; same log. |
| Historical regressions | Ordinary build, Linux release and actual AppImage each match **56/56 PNGs, 4 full traces, 4 guest traces and 4 complete WAVs** against every P4.13/P4.14/P4.15/P4.16 manifest, also checked against the original P4.11 oracle. `work/p418/regression/{window,linux,appimage}/manifest.json`. |
| Attract regression | Includes the **9,000-field no-coin** route: all 30 PNGs and full WAV unchanged. Also covers game-over/new-game, combat and legacy combat. |
| Linux loader compatibility | Actual glibc **2.31 loader/libraries** smoke passes with the packaged executable/SDL and exact 1x MD5. `work/p418/release/glibc231-smoke.log`. |
| ROM/frontend/persistence | ZIP in Unicode/spaces folder, remembered source after cache deletion, adjacent cart, HOME fallback, quick save/cold load all pass. |
| Controller / Steam | SDL virtual-gamepad-only browser/Start/navigation and independent SteamDeck/SteamAppId fullscreen probes pass. |
| Tar / AppImage payload | Extracted tar launches with exact smoke MD5. Independently extracted AppImage and tar match all **21 AppDir entries**; no ROMs/settings/saves/input data. `work/p418/payload-audit.json`. |

The four requested shot-1900 checks all pass, using 2,000-field runs:

| Executable | Internal resolution | PNG MD5 |
|---|---:|---|
| `work/p417/build-linux/saturn_stv` | 1x | `9bd5386ea7439621c19674381086e8fd` |
| Same Linux release | 2x | `8f1d9a1f01a2b77cd9d4f82b22b71b37` |
| Actual AppImage | 1x | `9bd5386ea7439621c19674381086e8fd` |
| Actual AppImage | 2x | `8f1d9a1f01a2b77cd9d4f82b22b71b37` |

Commands, durations, exit statuses and MD5s for release checks are in
`work/p418/release/report.json`; its driver is `work/p418/verify-release.py`.

## AppImage netplay

**Both full 9,000-field pairs pass**, paced, input delay 3, with the actual
AppImage as recorder and **both** peers. Each pair matches all **151 confirmed
checkpoints**, final PNG, every PCM sample and every byte of the final complete
machine payload. Both players join; the original verifier's 1,757-event script
covers combat, scene transitions, continues and a new game. All four peer runs
report `desync/failure=0` and **zero audio underruns, dropped chunks or resyncs**.

| One-way latency / loss / jitter | Host rollbacks / resim fields / max depth | Guest rollbacks / resim fields / max depth |
|---|---:|---:|
| 0 ms / 0% / ±0 ms | 0 / 0 / 0 | 0 / 0 / 0 |
| 100 ms / 3% / ±20 ms | 1448 / 4679 / 8 | 1495 / 5781 / 8 |

Evidence: `work/p418/netplay-appimage/report.json`, peer `run.log` files and
`work/p418/netplay-appimage.log`. As in P4.16, the sandbox denies
`socket(AF_INET, SOCK_DGRAM)` even for loopback. These pairs explicitly use
the existing **test-only file-backed datagram shim**; production protocol,
rollback, rendering and confirmed audio execute normally. This establishes
deterministic impaired-transport coverage, not kernel UDP or internet access.
The shim is preloaded only for tests and is absent from distribution payloads.
The test exercised the eight-field cap; it does not establish the previously
unmet 16.7 ms worst-case rollback budget.

## Launcher inspection

I opened and visually inspected the actual AppImage's **1280×800** captures:

- `work/p418/release/launcher-main.png`
- `work/p418/release/launcher-netplay.png`
- `work/p418/release/launcher-netplay-host.png`
- `work/p418/release/launcher-netplay-join.png`
- `work/p418/release/launcher-gamepad-nav.png`

Main controls, both player binding columns and factory-settings button fit;
text is readable without bottom/right clipping. Netplay host shows UDP 7845,
join shows IPv4:port, and delay/instructions fit. The virtual-pad screenshot
shows the connected controller and a visible focus outline on Fullscreen.
Image headers independently confirm 1280×800. Steam fullscreen probes use the
dummy display's separate 1024×768 mode; they test the fullscreen flag.

## GLIBC and deliverables

The maximum required GLIBC remains **2.30**, below the **2.31** ceiling:
game **2.29**, bundled SDL **2.30**, from `objdump -T` versioned symbols.
The AppImage type-2 runtime is statically linked. Symbol/dependency dumps are
`work/p418/objdump-*` and `ldd-*`; package details and recursive dependencies
are in `work/p417/DieHardArcade.AppDir/package-manifest.json`.
Only glibc libraries/loader resolve outside the bundle; SDL is bundled and
C++/GCC support is static. Driver libraries remain dynamically loaded from the
user's OS as documented in P4.17.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `work/p417/DieHardArcade-x86_64.AppImage` | 5,356,024 | `da3983de406e16f51c8b3552fbb12ece3c2166bd7254fe126dd5dafb88dc2f5c` |
| `work/p417/DieHardArcade-x86_64.tar.gz` | 4,879,597 | `b937ff7351b72f192fc2e6dd68f806972cd363426a9a0071bc1e8c80f15a4cdc` |
| `work/win/DieHardArcade-windows-x64.zip` | 5,581,144 | `5ede50d7bcd4a45d410fed24146d6b3131b20e36ed4792fccbd4a49d2e055959` |

AppDir regular-file payload: **12,368,506 bytes**. Sizes, hashes, GLIBC maxima
and build identities are retained in `work/p418/artifact-report.json`.
The normal Linux, release Linux and Windows builds all have unchanged netplay
source identity **`0x387e6bfac0b0d73c`**. Exact quick-state build identities
remain platform/build-specific as intended.

Physical Steam Deck/Gamescope, hardware Steam Input/controllers, speakers,
real UDP/internet reachability and execution on Windows/macOS remain owner
checks. No Wine, clean-container or physical-device pass is claimed. P4.16's
existing rollback-performance and emulation-fidelity limits remain in force.

## Ten-line summary

1. Integrated P4.17 over final P4.16 at HEAD `8165d44`; no commit made.
2. Frontend/CMake/harness changes now live in `port/`; the runtime patch changes only `host.cpp`.
3. Linux release guards preserve normal Linux, macOS, Windows and final netplay behavior.
4. The builder verifies current input hashes and refreshes stale snapshots; 261 source hashes match.
5. Linux AppImage/tar and Windows owner ZIP were rebuilt; sizes and SHA-256s are recorded above.
6. Maximum GLIBC is 2.30; the actual glibc 2.31 loader smoke passes.
7. All 164 runtime tests and zero-fuzz/offset runtime/Windows patch reconstructions pass.
8. All 56 reference PNGs/four trace pairs/four WAVs and requested Linux/AppImage 1x/2x MD5s match.
9. AppImage 0 ms and 100 ms netplay pairs pass 151 checkpoints each, exact final state/image/PCM.
10. Inspected 1280×800 main/Netplay/gamepad captures; physical Deck, live UDP and Windows execution remain owner checks.
