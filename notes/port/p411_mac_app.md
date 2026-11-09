# P4.11: standalone macOS app v1.01

2026-11-08, on the Intel i9 Mac (Darwin 25.5.0, Apple clang 21, Homebrew SDL3 3.4.12).
Paths are repository-relative. No commit was made on either machine.

## Sync to P4.10

- The Mac copy fast-forwarded from `be93c6e` to p620's `98ca3ae`. Its uncommitted P4.8/P4.9
  files were all identical to, or older versions of, files in p620's `57a6783`. Nothing was
  unique to the Mac. A safety tarball of those files is at `work/p411/pre-sync-mac-worktree.tgz`
  (gitignored).
- `tools/saturn-recomp`: the three P4.10 patches were applied in order on top of the Mac's
  P4.8+P4.9 runtime. All 27 changed runtime files are now byte-identical to p620's except
  `runtime/src/video/video.cpp`. On p620 that file carries Codex's in-progress P4.11
  `STV_CAPTURE_SCENES` capture code (`p411-scene-capture.patch`), which this note does not include.
- `bash port/p48_build_mac.sh` built cleanly with no warnings (`work/p411/build-window.log`).

### P4.10 verification on the Mac

| Check | Result |
|---|---|
| Smoke: `--vblanks 2000 --shots 1900`, default route | rc 0, 0 FATAL, shot md5 `e67bc072dedf1fef5f5286df70399bca`, matching the previous Mac/Linux reference |
| `port/p48_verify.py` against p620's `port/run/P48/p410-final/manifest.json` | **56/56 PNGs and all four trace hashes match** (nocoin 30, gameover 16, combat 5, legacy 5); `work/p411/regression.log` |
| `python3 port/p410_verify_states.py --binary port/run/P48/build-window/saturn_stv` | PASS: 300 warm + 500 cold frames, RAM/semantics identical, 4/4 bad states rejected. The PCM SHA256 `c6e6d6a3…80c1` (saved cursor 1,102,700, final 1,471,274, slot 4,758,781 bytes) is identical to P4.10's Linux result; `work/p411/states.log` |
| Launcher capture (`SATURN_LAUNCHER_SHOT`, SDL dummy) | `work/p411/launcher-p48.png`, inspected: shows cabinet pause (F3), violence (blood), F5/F7 quick-save line and *Load quick save*, and no touch overlay |

On this i9, headless runs at 82–90 VB/s, about 1.4–1.5x real time.

## Standalone app

New files. The P4.8 files stay as the last working in-tree version.

- `port/p411_build_app.sh`: a separate Release tree `port/run/P411/build-app` with
  `MACOSX_DEPLOYMENT_TARGET=14.0`, which is Homebrew SDL3's own floor. The default P4.8 build
  targets macOS 26. It runs the same generation/continuation steps as P4.8, then the bundler.
- `port/p411_bundle_mac.py`: builds `port/run/P411/Die Hard Arcade.app`. It copies the game
  binary and every non-system dylib (only `libSDL3.0.dylib`) into `Contents/Frameworks`, sets
  install names to `@rpath/…`, and replaces the Homebrew rpath with
  `@executable_path/../Frameworks`. It also adds the native launcher and the licenses for SDL3,
  Dear ImGui, the Inter font and saturn-recomp, writes Info.plist (1.01,
  `org.saturn-recomp.diehard-arcade`, `LSMinimumSystemVersion` 14.0) and signs with an ad-hoc
  signature (`codesign --force --deep -s -`). The build fails if any Mach-O still references a
  path other than `/usr/lib`, `/System` or `@…`, or if ROM-like files ended up in the bundle.
- `port/p411_app_mac.c`: `Contents/MacOS/DieHardArcade`, the bundle executable.
  - Its data folder is `~/Library/Application Support/Die Hard Arcade/`. It holds `cart.bin`,
    `diehard_main.bin`, `stv.ini` (launcher/game settings), `stv.ini.diehard.quickstate` (F5/F7),
    `shots/` (F12) and `app.log`.
  - On first launch, a MAME `diehard.zip` or its five loose game ROMs (`fpr19119.13`,
    `mpr19115.2` … `mpr19118.5`) in that folder are turned into `cart.bin` + `diehard_main.bin`.
    This uses the same layout as `build_cart.py`: `diehard_main.bin` is `cart.bin[0x200000:+0x8DE20]`.
    It needs only the system `/usr/bin/unzip`, takes about 0.5 s, and the result is SHA-256
    checked against the known-good images. The BIOS ROMs are not needed, because the BIOS is
    HLE. No Python or repository files are required.
  - If the ROMs are missing, a native dialog names the folder and the files and offers
    **Choose ROM folder…** (a folder picker accepting diehard.zip, the loose files or a
    prepared cart.bin), **Show Folder** (opens it in Finder) and **Quit**. A bad dump or wrong
    set gets a separate alert.
  - It then execs `Contents/MacOS/saturn_stv --window --launcher` with those absolute data
    paths, after clearing inherited `STV_*` variables, and passes extra arguments through.

**No runtime C++ change was needed.** The existing `--cart/--cartrom/--settings/--out`
options and the settings-relative quick-state slot already cover every path. So
`port/runtime-patches/p411-mac-app-paths.patch` does not exist, and nothing needs merging with
the Codex P4.11 work.

### Install and use (owner)

1. Build: `bash port/p411_build_app.sh` (needs Xcode CLT, CMake, Ninja and Homebrew SDL3 on
   the *build* Mac only).
2. Drag `port/run/P411/Die Hard Arcade.app` to `/Applications`, or copy it to another Mac.
   On another Mac, after a download or AirDrop, clear quarantine once with
   `xattr -dr com.apple.quarantine "/Applications/Die Hard Arcade.app"`, or right-click > Open.
   The app is not notarized.
3. Put your own MAME `diehard.zip` in `~/Library/Application Support/Die Hard Arcade/`
   (Finder: Go > Go to Folder…), or start the app and use **Choose ROM folder…**.
   Already prepared `cart.bin` + `diehard_main.bin` also work. On this Mac they are already
   copied there.
4. Open the app. Use the launcher to set controls and game settings, then Start. Escape/P
   pauses, F5/F7 quick save/load, F11 fullscreen, F12 screenshot.

### Verification

Everything ran from a copy outside the repo, made with `ditto` into the session scratchpad.
The environment was `env -i HOME=<fake home> PATH=/usr/bin:/bin`, so no Homebrew or DYLD
variables were set. The evidence is in `work/p411/app/`.

- `otool -L`/`LC_RPATH` show only `/usr/lib`, `/System` and `@rpath/libSDL3.0.dylib` →
  `@executable_path/../Frameworks`. `minos 14.0` holds for all three Mach-Os.
  `codesign --verify --deep --strict` passes on the copy.
- `DYLD_PRINT_LIBRARIES` shows SDL3 loaded from the bundle's `Contents/Frameworks`. The smoke
  command run with the bundled `saturn_stv` gives rc 0, 0 FATAL and shot md5 `e67bc072…bca`,
  so the macOS-14-targeted build is identical too.
- **ROMs present:** with diehard.zip in the fake home's Application Support, the app prepared
  ROMs whose hashes match the repository images. `app-launcher.png` was inspected: the same
  launcher, with "Program: diehard_main.bin" from the data folder. With
  `--no-launcher --vblanks 1950 --shots 1900 --save-at 1500` it booted and saved the quick slot
  in the data folder. `shot-1900.png` was inspected (title/attract screen) and is identical to
  the P4.8 in-tree build in the same window mode (`c114f772…`). `--load-state` from the data
  folder loaded VB 1500. Loose ROM files (`app-loose.log`, `loose-launcher.png`) also work.
- **ROMs missing:** an empty Application Support logs "ROMs missing", and the launcher waits on
  the dialog (`osascript` alive after 6 s; ended by stopping that test process → quit, rc 1).
  A one-byte-corrupted `mpr19117.4` is rejected by SHA-256 with the bad-set alert, followed by
  the ROMs-needed dialog (`app-bad.log`). All AppleScript snippets were also syntax-checked.
  **The native dialogs were not screenshotted:** `screencapture` cannot capture the display
  from this agent environment, as in P4.8.

## Open issues

- **Desktop launch is still unverified here.** A Finder double-click, Cocoa/Metal window,
  real dialogs, pads and speakers need a check from the owner's desktop. P4.8 found that
  LaunchServices `open` is refused in this environment, so it was not used.
- **Not notarized.** An ad-hoc signature runs locally. Other Macs need the quarantine step
  above. Notarization needs a paid Apple Developer ID (`codesign -s "Developer ID…"
  --options runtime` + `notarytool`).
- **Intel-only, macOS 14+.** It runs on Apple Silicon only via Rosetta 2, which is untested.
  A universal build needs an arm64 SDL3 and an arm64 recompile/runtime build.
- **Distribution caution:** the app ships no ROM bytes, but `saturn_stv` *is* a static
  recompilation of the game's SH-2 program, which makes it a derivative of copyrighted code.
  Keep it for the owner's own machines; don't post it publicly.
- Generic app icon (no `.icns`). The supported set is fixed to MAME `diehard` (USA) by SHA-256;
  other regions/revisions are rejected.
- Quick-save slots are tied to the exact build fingerprint, so P4.8 in-tree slots and app slots
  are not interchangeable, and every rebuild invalidates old slots (P4.10 design).
- On p620 the macOS-14 build path was not exercised; these scripts are macOS-only.
