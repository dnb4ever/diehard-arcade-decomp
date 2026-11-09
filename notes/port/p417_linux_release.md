# P4.17 — Linux AppImage and Steam Deck

2026-11-09. Portable x86_64 Release, bundled official SDL3, no ROM data.

**P4.18 integration supersedes the frozen-snapshot build, mixed patch, and artifact
hashes below.** The frontend is now in `port/`, the runtime-only patch follows
final P4.16, and the builder refreshes from current input hashes. Current
verification and rebuilt artifacts: [P4.18 integration](p418_integration.md).
All paths below are repository-relative. No commit, sudo, GPU access or edits
to existing `port/` files, shared `tools/saturn-recomp/`, `project/`, or
`tools/decomp/` were made by this milestone.

The source was frozen once at initial HEAD `c2a8e58`, including the applied
runtime patches and the concurrent P4.16 files as they existed then.
`work/p417/snapshot.json` inventories 260 original files; the originals are
in `work/p417/baseline/`, with build copies in `work/p417/saturn-recomp/` and
`work/p417/recomp/`. Subsequent runs reuse that snapshot and verify its
original hashes. They never refresh from the moving shared tree. The other
job advanced HEAD during this work; that did not change these sources.
**The P4.17 patch must be rebased after P4.16 before integration or rebuilding
from its final sources.** These artifacts establish offline release packaging
for the captured snapshot, not acceptance of P4.16 networking.

## Build and outputs

From the repository root:

```bash
JOBS=6 port/p417_build_linux.sh
```

Build prerequisites: Linux x86_64, GCC/G++ 11 or newer, CMake 3.24+, Make,
Python 3.12+, GnuPG, `dpkg-deb`, `patch`, binutils, `pkg-config`, and the
existing generated `port/run/P41/recomp/`. The script deliberately does not
regenerate or patch shared generated code. Generate that tree separately if
it is absent. No SDL development installation, FUSE, Docker, Python packages,
root installation or graphics hardware is needed for the build.

The wrapper freezes sources once, verifies/downloads dependencies into
`work/p417/opt/`, applies the new patch only to the copy, runs the existing
continuation and P4.15 idle-loop passes on copied generated C++, builds SDL3
and the game, then packages them. CMake uses a Debian 11 glibc link/header
sysroot plus the Ubuntu toolchain team's GCC 11 C++ library built for focal.
The host compiler here is GCC 15.2.0. Release uses `-O3 -DNDEBUG`,
`-march=x86-64-v2`, no fast-math or LTO, and static libstdc++/libgcc support.
The current runtime requires C++20 `atomic::wait`, which Debian 11's GCC 10
headers do not implement. GCC 11 headers/static libraries solve that without
raising the glibc baseline. Compiler intrinsic headers remain those of the
host compiler; compiler and C++ library versions need not be identical.

Outputs:

- `work/p417/DieHardArcade-x86_64.AppImage`: owner AppImage.
- `work/p417/DieHardArcade-x86_64.tar.gz`: FUSE-free distribution of the same AppDir.
- `work/p417/DieHardArcade.AppDir/`: unpacked package; executable `AppRun`.
- `work/p417/build-linux/saturn_stv`: unstripped game binary.
- `work/p417/DieHardArcade.AppDir/package-manifest.json`: payload hashes,
  recursively resolved ELF dependencies, maximum GLIBC versions, snapshot hash.
- `work/p417/build-final.log`, `work/p417/build-complete.log`,
  `work/p417/package-complete.log`, `work/p417/verify-delivery/`: build and validation evidence.

Repackage an existing build with `python3 port/p417_package_appimage.py`.
`--fetch` verifies dependencies and creates the link toolchain instead.
AppImage publication uses a temporary output followed by atomic replacement,
so repackaging works even while an earlier AppImage is open. AppImage uses
zstd SquashFS; this pinned appimagetool supports zstd. The plain tar uses gzip.
The package has an `AppRun`, `.desktop` file, `.DirIcon`, the existing original
`port/assets/p413-icon.png`, the game, SDL, instructions and license notices.
Its file allowlist excludes ROM, save, settings and input-recording data.

## Verified downloads

| Component | Pin | SHA-256 |
|---|---|---|
| Official SDL source | `release-3.4.20 / SDL3-3.4.20.tar.gz` | `f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f` |
| appimagetool x86_64 | `1.9.1` | `ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0` |
| Type-2 runtime x86_64 | `20251108` | `2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d` |
| Debian bullseye package index | `main/binary-amd64/Packages.xz` | `1c3405444297ead864711c18e64bce4810db63b48340a2f9d81182d9f235ad71` |
| Focal toolchain PPA index | `main/binary-amd64/Packages.xz` | `906cdc622d6fd9975d36d0c4ba86dbcf95a949426ce174aa7e0ce8de99768653` |

Sources: [SDL official release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.20),
[appimagetool 1.9.1](https://github.com/AppImage/appimagetool/releases/tag/1.9.1),
[type-2 runtime](https://github.com/AppImage/type2-runtime/releases/tag/20251108),
[Debian bullseye signed index](https://deb.debian.org/debian/dists/bullseye/InRelease),
[Ubuntu toolchain team focal PPA](https://ppa.launchpadcontent.net/ubuntu-toolchain-r/test/ubuntu/dists/focal/InRelease).

All GitHub asset hashes match their published release API digests; raw API
records are retained in `work/p417/opt/*-release.json`. SDL's detached signature
also verifies using the current RSA signing key from its
[official signing-key page](https://libsdl.org/signing-keys.php), fingerprint
`0900104363B4C9D4223DE149D913FE7D4B61D39B`. The type-2 runtime signature verifies
against its published key, fingerprint
`570C77ACEA40C0F1B758902CBF96CCA56490F695`. appimagetool has a published digest
but no detached signature in this release. Public keys were obtained from the
publisher's HTTPS source; this does not establish an independent trust chain.

Each sysroot package is checked against SHA-256 in its authenticated index.
The Debian InRelease signing subkey is
`A7236886F3CCCAAD148A27F80E98404D386FA1D9`, under archive key
`1F89983E0081FDE018F3CC9673A4F27B8DD47936`. The toolchain PPA's signing key is
`C8EC952E2A0E1FBDC5090F6A2C277A0A352154E5`, also identified by Launchpad's archive
metadata. The verifier requires those specific valid fingerprints; additional
unavailable signatures on the same InRelease do not invalidate the required
valid signature. Individual package versions/hashes are recorded in
`work/p417/opt/sysroot-packages.json` and `cpp11-packages.json`. C++ packages
are `11.5.0-10ubuntu1~20~ppa3`. Signed index hashes are pinned, so repository
updates fail closed rather than silently changing the build.

SDL **3.4.20 matches the Windows build's pinned release**. The supplied Mac
notes describe Homebrew **3.4.12**, so those existing Mac artifacts do not
currently share a single SDL version with Windows. This Linux package uses
Windows's explicit 3.4.20 pin; no Mac files or Homebrew installation were changed.

SDL is built with X11, Wayland, OpenGL/GLES/Vulkan presentation support,
ALSA, PulseAudio, udev, HIDAPI, Linux and virtual gamepads. PipeWire's native
backend and KMSDRM are disabled; SteamOS can use PulseAudio compatibility and
its compositor. The matching Debian 11 `wayland-scanner` is explicitly used:
the host's newer scanner generates calls absent from the older headers.
No SDL source modifications are needed. Driver libraries are loaded dynamically
from the user's OS; vendor graphics drivers are deliberately outside the bundle.

## Linux installation and ROMs

1. Copy the AppImage to a permanent location and mark it executable:
   `chmod +x DieHardArcade-x86_64.AppImage`.
2. Double-click it, or run `./DieHardArcade-x86_64.AppImage`.
   If FUSE is unavailable:
   `APPIMAGE_EXTRACT_AND_RUN=1 ./DieHardArcade-x86_64.AppImage`.
3. Alternatively extract `DieHardArcade-x86_64.tar.gz` and execute
   `DieHardArcade.AppDir/AppRun`. Keep `AppRun` and `usr/` together.
4. Supply your own MAME **diehard (USA)** set: `diehard.zip`, its five loose
   members (`fpr19119.13`, `mpr19115.2` through `mpr19118.5`), or prepared
   `cart.bin`. Place it beside the AppImage, or select its folder in the app.
   For the tar distribution, place it beside `AppRun` or choose its folder.
5. Review settings in the launcher, select Start, insert a coin and join.

The app searches its verified cache, beside the AppImage, the remembered
folder, and its data folder. An explicit `--rom-dir FOLDER_OR_ZIP` chooses a
source; `--choose-roms` opens the browser even if a valid cache exists.
The in-window missing-ROM message names the supported files and cache folder,
with **Use this ROM folder**, **Home**, **Package folder**, **Storage / SD card**,
**Parent folder**, **Show hidden** and **Quit**. The directory list supports
D-pad/stick navigation and A selection. B goes to the parent folder. It needs
no Zenity, portal, Python, shell unzip, or desktop folder picker.

The existing `port/p413_roms.c/.h` performs bounded ZIP/loose/prepared conversion,
SHA-256 verification and atomic writes. It derives `diehard_main.bin` from a
valid cart. Wrong or missing data remains on the ROM screen with an error;
an explicitly selected source does not silently fall back to another set.
No ROM data is present in either distribution.

The existing Linux settings/save location is preserved:

| Item | Location |
|---|---|
| Settings | `$XDG_DATA_HOME/saturn-recomp/stv.ini`, or `~/.local/share/saturn-recomp/stv.ini` |
| Quick-save slot | `stv.ini.diehard.quickstate`, beside settings |
| ROM cache | `roms/cart.bin` and `roms/diehard_main.bin`, under that same data directory |
| Remembered source | `rom-folder.txt`, under that same data directory |
| F12 screenshots | `shots/`, under that same data directory |
| Default input recording | Existing settings-relative quick-slot path plus `.inputs` |

Existing settings are reused. Explicit `--settings`, `--quick-state`, `--out`
and other harness options remain available. An explicit `--cart` bypasses
release discovery and retains the original headless/default-input behavior.
The package never changes the process working directory, so repository-relative
smoke commands still work. ROMs/saves are never written into a mounted AppImage.

## Steam Deck

In Desktop Mode, copy the AppImage somewhere permanent and enable its
executable permission (Properties → Permissions). In Steam choose
**Games → Add a Non-Steam Game → Browse**, select the AppImage and add it.
Leave **Force compatibility tool** off: this is a native Linux executable.
Select Steam Input's **Gamepad** layout so SDL receives standard controller
buttons rather than a desktop mouse/keyboard layout. Return to Gaming Mode;
launch it from **Library → Non-Steam**. If FUSE fails, add the extracted
`DieHardArcade.AppDir/AppRun` instead, or use launch options
`APPIMAGE_EXTRACT_AND_RUN=1 %command%` for the AppImage.

`SteamDeck=1` or nonempty `SteamAppId` selects fullscreen by default. Desktop
launch remains windowed unless saved/CLI fullscreen requests it. The release's
default window is 1280×800 (an explicit `--scale` is honored); the existing responsive launcher scaling makes
text about 17 logical pixels tall there. F11 toggles fullscreen. Use the
Deck's 1280×800 mode and original **1x internal resolution** initially;
2x/3x require more CPU. Physical Deck performance was not measured.

The existing SDL3 Gamepad API already opens/hotplugs controllers, assigns
P1/P2 slots, reads D-pad and left-stick axes, and saves per-player bindings.
Existing ImGui setup already enables `NavEnableGamepad`, selects SDL's
`AutoAll` gamepad mode and suppresses menu activation during binding capture.
P4.17 reuses those paths. Navigate with D-pad/stick, select with A, use B to
back out, and select tabs with the D-pad and A. Move the D-pad to establish
navigation focus when first connecting a pad. Steam+X opens Steam's on-screen
keyboard for optional text fields such as netplay addresses or movie paths.

Default gameplay: first two pads are P1/P2; Back/Select inserts a coin,
Start joins; Xbox X/A/B correspond to arcade A/B/C. Guide opens pause when
Steam does not intercept it; keyboard Esc/P also opens pause. Keyboard P1:
arrows, Z/X/C, 5 coin, 1 start. F5/F7 quick save/load; F12 screenshot.
Real Steam Input layouts, hardware hotplug and Steam's Guide interception
still need the owner to check on a Deck.

## Patch and README entry for later integration

`port/runtime-patches/p417-linux-release.patch` has four files, with both
`port/...` and `runtime/...` paths. Apply only inside the isolated layout:

```bash
patch --batch --fuzz=0 -p1 -d work/p417/saturn-recomp \
  < port/runtime-patches/p417-linux-release.patch
```

The wrapper detects an already applied patch. It adds the Linux-only ROM
frontend header and harness call, CMake's opt-in
`SATURN_STV_LINUX_RELEASE=ON` wiring/shared converter, and small host-window
size/Steam fullscreen defaults. The new frontend/converter sources join the
state fingerprint. The browser uses flattened ImGui navigation so controller
focus crosses into/out of its directory list. Nothing changes default
behavior on macOS/Windows or in builds without this release option. Guest
execution, emulation timing, renderer algorithms and netplay are untouched.

`work/p417/patch-check-final.log` proves all four patched files reconstruct
byte for byte from the frozen baseline, with **zero fuzz and zero offsets**.
The original baseline inventory remains unchanged. **Rebase this patch after
P4.16**; do not apply it to the shared runtime clone or refresh the snapshot
while another job is editing that clone.

Suggested `port/runtime-patches/README.md` entry (text only; that file was
not edited here):

> **P4.17 Linux release:** After the final P4.16 sources are integrated, rebase
> `p417-linux-release.patch`. It contains both `port/...` and `runtime/...`
> paths; apply it only in the snapshot created by `port/p417_build_linux.sh`.
> The opt-in Linux release frontend prepares verified external ROMs through
> the shared P4.13 converter, supplies a gamepad-accessible folder browser,
> retains Linux settings/save paths and selects fullscreen under Steam.
> Other platforms/default builds retain their existing behavior. Build and
> package SDL3/AppImage/tar through that script; see
> `notes/port/p417_linux_release.md` for pins, glibc and verification evidence.

## Verification and portability

Final evidence is in `work/p417/verify-delivery/`; commands, exit status and
wall times are in `report.json`. Reproduce this job's integration checks with
`P417_VERIFY_DIR=verify-again python3 work/p417/verify.py` after building
(the helper and SDL virtual-pad interposer are retained under `work/p417/`).
All runs use SDL dummy video/audio and software rendering. No GPU was used.

The actual AppImage was executed with extraction enabled because `/dev/fuse`
is absent. Its required smoke command is:

```bash
mkdir -p work/p417/verify-manual
APPIMAGE_EXTRACT_AND_RUN=1 SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy \
  SDL_RENDER_DRIVER=software work/p417/DieHardArcade-x86_64.AppImage \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2000 --shots 1900 \
  --out work/p417/verify-manual
md5sum work/p417/verify-manual/shot-1900.png
```

| Check | Result |
|---|---|
| AppImage 2000-field smoke | Exit 0, zero FATAL; shot 1900 MD5 **`9bd5386ea7439621c19674381086e8fd`** |
| AppImage no-coin attract, 9000 fields | Exit 0, **30/30 PNG SHA-256s** and complete WAV identical to `work/p415/regression/manifest.json` |
| glibc 2.31 loader/libraries | Same 2000-field smoke and exact MD5, running the packaged binary with the Debian 11 loader and its libraries |
| FUSE-free tar | Extracted AppRun passes the same 2000-field smoke and exact MD5 |
| User-selected ROM folder | Compressed `diehard.zip` in a Unicode/spaces path prepares both images with the repository SHA-256s |
| Persistence | Settings/quick slot in existing Linux location, quick save at VB60, cold load and stop at VB90; remembered source reloads after deleting only the test cache and changing working directory |
| Adjacent ROM discovery | AppImage finds an external prepared cart beside itself, from another working directory |
| HOME fallback | With empty XDG_DATA_HOME, settings go to the existing `~/.local/share/saturn-recomp` location |
| Controller-only browser and Start | SDL virtual gamepad uses D-pad/A through the new browser and real launcher, then runs to VB90; no keyboard/mouse events |
| Controller navigation | Real SDL virtual-pad D-pad input moves focus to the Scaling control; inspected `launcher-gamepad-nav.png` |
| Steam fullscreen | Both `SteamDeck=1` and `SteamAppId=123456` independently produce the SDL fullscreen flag |
| Existing two-player defaults | Both SDL virtual gamepads attach and pass the runtime's existing button/axis checks |
| Missing/bad ROM UI | Real in-window captures, no ROMs packaged; incorrect explicit source remains on the ROM screen |

No-coin WAV SHA-256:
`e18575ba97d92a11fd902542714c1c73f301b7dce087c2ae81c3f956c60ee7a8`.
This is a packaging/regression check, not a new full-game or netplay validation.
The deliberately unpaced dummy-audio save/pad checks can report audio resyncs;
they do not measure speaker output, audio latency or paced performance.

**The combined maximum required GLIBC version is 2.30**: packaged game
**2.29**, packaged SDL **2.30**, measured from undefined versioned symbols
using `objdump -T`. The build targets glibc **2.31** and the actual 2.31-loader
smoke passes. It does not claim testing on a complete older distribution.
`objdump-binary.txt` and `objdump-sdl.txt` retain the full dynamic symbol tables.
The AppImage type-2 runtime is statically linked and has no glibc dependency.

The executable has `RUNPATH=$ORIGIN/../lib`. With an empty inherited library
environment, `ldd` resolves SDL from `usr/lib/` in the AppDir. Only
`libc.so.6`, `libm.so.6`, `libpthread.so.0`, `libdl.so.2` and the glibc loader
resolve outside it (plus the kernel vDSO). No host SDL, libstdc++, libgcc,
X11/audio driver or build-directory library is a linked dependency.
`ldd-binary.txt`, `ldd-sdl.txt`, `readelf-binary.txt`, and the dummy-launcher
`loader-trace.log` provide independent evidence; AppRun also replaces inherited
LD_LIBRARY_PATH with its own bundle path. SDL's intentional `dlopen` use of
OS display/audio/controller drivers on a real desktop remains necessary.

A clean container was **unavailable without sudo**: Docker, Podman and proot
are absent; installed bwrap exits 1 because unprivileged namespaces are denied.
Evidence: `work/p417/container-check.log`. No policy or privilege changes were
attempted. The empty-environment checks and old-loader run supplement that
missing container check; they are not reported as a container pass.

I opened and visually inspected the actual 1280×800 main, features, missing-ROM
and gamepad-focus captures. Start/Quit, all settings and both players' key/pad
columns fit; text is readable, no bottom/right clipping. The ROM screen names
the files/cache, offers the browser controls and leaves room for folder lists.
The features tab's rewind/turbo fields fit. Images:

- `work/p417/verify-delivery/launcher-main.png`
- `work/p417/verify-delivery/launcher-features.png`
- `work/p417/verify-delivery/launcher-missing.png`
- `work/p417/verify-delivery/launcher-gamepad-nav.png`
- `work/p417/verify-delivery/launcher-bad-rom.png`

The Steam-env fullscreen probes use SDL dummy's 1024×768 virtual display,
so they prove the fullscreen flag separately from the 1280×800 layout captures.
They do not prove Gamescope behavior on physical hardware.

## Sizes and remaining work

| Artifact | Bytes | MiB |
|---|---:|---:|
| `work/p417/DieHardArcade-x86_64.AppImage` | 5,356,024 | 5.11 |
| `work/p417/DieHardArcade-x86_64.tar.gz` | 4,879,594 | 4.65 |
| AppDir regular-file payload (21 file/symlink entries) | 12,368,506 | 11.80 |

DieHardArcade-x86_64.AppImage SHA-256: `1dbb7b61d41d46f810b6b48e83745eb7f52d1f02ad1b42eca7d3e4058fa40403`.

DieHardArcade-x86_64.tar.gz SHA-256: `0373fc789566844f9ffaa6a65551e54f055346d6454d4e305eeb01a56f4ea4b2`.

The AppImage was independently extracted and all 21 file/symlink entries
match the AppDir byte for byte; no ROM data files occur. All 260 frozen
baseline originals retain their recorded SHA-256s. Evidence:
`work/p417/artifact-audit.json` and `work/p417/artifact-audit-final/`.

Final `work/p417/` allocated usage: **1,327,296,512 bytes (1.24 GiB)**,
including downloads, sysroot, sources, builds, archives and all verification evidence;
well below the requested approximately 6 GB limit.


Remaining owner checks: install/launch on a physical Steam Deck/SteamOS 3 and
an older Linux desktop; real Gamescope fullscreen, Steam Input/hardware pads,
SD-card permissions, physical speakers/latency, and paced performance. Use the
native Linux target, not Proton. No clean-container result or physical Deck
measurement is claimed. The supplied Mac notes' older SDL pin is explicitly
recorded above. Rebase against the final P4.16 tree before integration.

Existing BIOS TEST/EEPROM and emulation-fidelity limits remain. Quick states
and input files require the existing exact source/compiler/ROM fingerprint;
this release need not load another milestone's or another platform's states.
No installer, updater, signing identity or distribution upload was added.

## Ten-line summary

1. Built isolated x86-64-v2 Release with official SDL3 3.4.20 and no ROM data.
2. Deliver `work/p417/DieHardArcade-x86_64.AppImage` and its FUSE-free tar.gz.
3. Rebuild with `port/p417_build_linux.sh`; the original snapshot is reused and checked.
4. Signed sources/indexes and published GitHub asset SHA-256 digests verify.
5. Linux settings/saves stay in `saturn-recomp` under XDG_DATA_HOME or the existing HOME fallback.
6. External ROM discovery, remembered folders and a controller-accessible browser are integrated.
7. Steam environments default to fullscreen; inspected 1280×800 launcher and virtual-pad navigation pass.
8. AppImage, tar and glibc-2.31-loader smoke MD5 is `9bd5386ea7439621c19674381086e8fd`.
9. All 30 no-coin PNGs/full WAV match; maximum GLIBC is 2.30 and only glibc libraries are external.
10. Container/physical Deck checks remain unavailable; rebase P4.17 after P4.16 before integration.
