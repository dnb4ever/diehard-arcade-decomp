# P4.12 — Windows x64 build

2026-11-08 (America/New_York). Cross-built on Linux without sudo. No commit.
Runtime and harness changes are isolated in `work/win/`; the existing Linux
and macOS sources/builds were not edited. Concurrent P4.11 changes were left
alone. This milestone uses the runtime snapshot taken at its start, including
P4.8/P4.9/P4.10, rather than following later in-place edits.

## Build and package

From the repository root:

```bash
JOBS=6 port/p412_build_win.sh
```

Prerequisites: Linux x86_64, Python 3.12+, CMake 3.20+, Make, `patch`, network
access, and the existing generated `port/run/P41/recomp`. The script downloads
and SHA-256-checks its pinned compiler and official SDL development archive,
copies the runtime/harness/generated program once, applies the P4.12 patch
only there, runs the continuation pass only on the copy, and builds Release.
No installation, sudo, Wine installation, or modification of `tools/` occurs.
If the generated program is absent, generate it as a separate coordinated
step using the existing P4.1 workflow; this script deliberately does not write
shared generated files while another job is working there.

Outputs:

- `work/win/build-windows/DieHardArcade.exe`: unstripped build executable.
- `work/win/DieHardArcade-windows-x64/`: ready-to-copy package folder.
- `work/win/DieHardArcade-windows-x64.zip`: owner distribution archive.
- `work/win/DieHardArcade-windows-x64/package-manifest.json`: package SHA-256s
  and recursively checked PE imports.
- `work/win/snapshot.json`: SHA-256 inventory of the original runtime,
  harness/build inputs, and generated program. `work/win/baseline/` retains
  those original sources. Subsequent builds reuse them, not the moving tree.

`port/p412_package_win.py --zip` can repack an existing build. It copies only
the executable, required DLLs, launch batch file, instructions, notices and
manifest. It rejects unrecognized files in an existing destination before
making a zip. **No ROM data is in the folder or zip.** The generated native
program still requires the owner's external cartridge images at runtime.

The package contains `DieHardArcade.exe`, `SDL3.dll`, `libc++.dll`,
`libunwind.dll`, `play.bat`, `README-windows.txt`, the manifest and notices.
Recursive import inspection found no need for `libwinpthread-1.dll`, libgcc,
or libstdc++; the shipped libc++ uses native Windows thread primitives.
Windows 10/11 supply the UCRT/API-set and ordinary Windows DLL imports.

## Pinned downloads and checksums

| Component | Release / archive | SHA-256 |
|---|---|---|
| llvm-mingw / UCRT | `20261104`, `llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz` | `919380403de87dca3c581c2053230cd298af5e891286628ba0613b7fed370926` |
| Official SDL3 development kit | `3.4.20`, `SDL3-devel-3.4.20-mingw.tar.gz` | `d2d3bf3fb97a90b22e908a306d7362631ef1bab229d988fe6669595dc29cd9ed` |
| Portable Wine, first attempt | `11.21`, `wine-11.21-amd64.tar.xz` | `1f12f2aef4f5d0b5c0548ceecdb20068edc59fcd23a5da481e7059a25070eac9` |
| Portable Wine, working variant | `11.21`, `wine-11.21-amd64-wow64.tar.xz` | `33f3c5b56b1deb8f6725539ea006f27b5d781b3b4ae6a8a1f492d8af4fb9dc83` |

Sources: [llvm-mingw release](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261104),
[official SDL release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.20),
[Kron4ek Wine release](https://github.com/Kron4ek/Wine-Builds/releases/tag/11.21).
All four archives matched the published GitHub release asset SHA-256 digests;
both Wine archives also matched the release's `sha256sums.txt` (itself verified
against its published asset digest). Raw release API records are retained in
`work/win/{llvm-mingw,SDL,Wine-Builds}-release.json`; download records are in
`work/win/opt/downloads.json` and `work/win/opt/build-downloads.json`.
These checks establish agreement with published digests, not independent
publisher signature verification.

Compiler: LLVM clang **23.1.5**, target `x86_64-w64-windows-gnu`, UCRT,
libc++, lld. CMake **4.2.3**, Unix Makefiles. SDL headers/import library/DLL
all come from the same official x86_64 development archive. Dear ImGui and
Inter are the existing vendored runtime copies.

For Wine reproduction, download the working archive and `sha256sums.txt`
from the linked release into `work/win/opt/`, verify the hashes above, and
extract the archive there. No root installation is needed. Tests use
`work/win/opt/wine-11.21-amd64-wow64/bin/wine` and private prefixes under
`work/win/`. The build script itself needs only llvm-mingw and SDL.

## Owner installation and play

1. Copy `work/win/DieHardArcade-windows-x64.zip` to a Windows 10/11 x64 PC
   and extract the **whole** archive. Keep the DLLs beside the executable.
2. Supply the project's prepared `cart.bin` (20,971,520 bytes) and
   `diehard_main.bin` (581,152 bytes) together in a folder. A MAME zip is not
   the expected input. The package supplies neither image.
3. Double-click `DieHardArcade.exe` or `play.bat`. If both images are next to
   the exe they are selected automatically. Otherwise choose their folder
   in the native Windows folder picker. That folder is remembered.
4. Review controls and settings in the launcher, then click Start. Insert
   a coin with 5 and press 1 to join as P1; 6 and 2 serve P2.

`play.bat --choose-roms` selects a different ROM folder.
`play.bat --rom-dir "D:\Games\My ROMs"` selects one explicitly.
`play.bat --no-launcher` goes straight to play with saved settings.
`play.bat --stats 5` attaches or opens a console for speed/audio reports;
normal launch is a Windows GUI executable and opens no console.

Settings are `%APPDATA%\Die Hard Arcade\stv.ini`; remembered media location
is `rom-folder.txt` in that directory. F5/F7 save/load
`stv.ini.diehard.quickstate`; F12 screenshots go into `shots\` there.
Files beside the exe are not used for settings or saves. Explicit advanced
`--settings`, `--quick-state` and `--out` arguments can override these paths.
An explicit `--cart` retains harness/headless defaults, making the original
Linux smoke command usable without changing its guest input timeline.

P1 uses arrows and Z/X/C; P2 uses R/F/D/G and A/S/Q. Escape/P opens the
pause menu; F11 or Alt+Enter toggles fullscreen. SDL3's existing Gamepad API
handles Xbox/PlayStation/Switch-style controllers, with separate P1/P2
bindings editable in the launcher. The first two connected pads are P1/P2;
Back/Select inserts a coin and Start joins. Connect them before launch for
predictable player order. F2/TEST still reaches the unimplemented HLE BIOS
exit; original cabinet bookkeeping/EEPROM support is not added here.

## Portability patch and collision handling

`port/runtime-patches/p412-windows-portability.patch` has six files and uses
both `runtime/...` and `port/...` paths. Apply it from the staging root,
**not** from the shared runtime clone:

```bash
patch -p1 -d work/win/saturn-recomp < port/runtime-patches/p412-windows-portability.patch
```

The build wrapper does this automatically and detects an already applied
patch. Existing `port/main_stv.cpp` and `port/stv.cmake` were not edited;
their Windows changes are carried in the patch and applied to staging copies.

- `runtime/src/sound/sound.cpp`: include `<cstdlib>` for `std::getenv`.
  This was the only C++ compile failure; no execution semantics change.
- `runtime/runtime.cmake` and `runtime/src/host/windows/saturn.rc`: embed
  the existing manifest through a resource compiler for MinGW. MSVC keeps
  its existing manifest mechanism; non-Windows builds keep their old path.
- `runtime/src/host/windows/saturn.manifest`: retain UTF-8 process code page;
  add PerMonitorV2 DPI awareness with the older per-monitor fallback.
- Staged `port/main_stv.cpp`: `_WIN32`-guarded GUI defaults, optional console,
  AppData location and output-folder creation, ROM-folder picker/remembering, and executable-directory
  ROM discovery independent of the working directory. Unicode folder paths
  use wide Windows APIs and explicit UTF-8 conversion.
- Staged `port/stv.cmake`: Windows-only GUI subsystem/output name and
  shell/COM libraries for the picker.

Binary ROM, PNG, WAV and state I/O already uses `rb`/`wb`; text reports may
use Windows CRLF. Filesystem joins accept both Windows separators and `/`.
POSIX agent sockets and Linux uinput already have Windows guards; timing and
thread handoffs use standard C++ chrono/thread APIs. No Windows mmap,
usleep, clock_gettime or pthread shim was needed. Agent-FD mode remains
explicitly unavailable on Windows, as before.

LLVM's integrated COFF assembler compiled all 17 generated function units
without size errors and handles bigobj automatically. Do not pass GNU as's
`-Wa,-mbig-obj` to it. Target pointer size is 8, long is 4, long double is 16,
and `__int128` is supported (16 bytes). Successful execution and matching
PNG/audio counts test the relevant arithmetic/ABI behavior; no guest math,
compiler optimization policy, or emulated timing was changed.

The patch reconstructed all six staged files byte for byte from the baseline
with **zero fuzz**. Original snapshot SHA-256s still match. Linux also builds
with the patch applied to another staging copy; its 2,000-VBlank smoke PNG
remains identical (`work/win/verify/final/linux-patched-smoke/shot-1900.png`). macOS was not available;
all added platform behavior is guarded by `_WIN32`/`WIN32`/`MINGW`.

## Verification

The final packaged executable exits 0 on both the requested **2,000-VBlank
smoke** and the **9,000-VBlank no-coin attract regression**. Both use SDL dummy
video/audio headless. The fresh Linux and Wine PNG files are byte-identical:

| Route | Shot VBlank | Fresh Linux MD5 = final Wine MD5 |
|---|---:|---|
| Default paid smoke | 1900 | `e67bc072dedf1fef5f5286df70399bca` |
| No-coin attract | 300 | `6c9a79530a32671a6795236090053b3e` |
| No-coin attract | 1900 | `c114f772f53057aa1908e918b61e1f87` |
| No-coin attract | 8870 | `20d81bf75c55b49f41bf8e0b1b3be336` |
| No-coin attract | 9000 | `6c9a79530a32671a6795236090053b3e` |

Wine wall times: **28.814 s** smoke and **109.341 s** attract. These are
correctness timings on a shared host, not Windows hardware performance claims.
Both OSes stop with identical master PC/PR/SR. Smoke audio: **1,471,274**
samples, **1,294,332** non-silent, 68000 PC **0001A2**. No-coin audio:
**6,626,101** samples, **6,221,408** non-silent, PC **000192**. Zero FATAL
in these valid game runs. This checks five PNGs, not every intervening frame.

Full evidence is retained under `work/win/verify/`, with exact final argv,
exit codes, PNG hashes and wall times in `work/win/verify/final/report.json`.
Windows runtime tests also pass: **136 tests, 0 failed**
(`work/win/verify/final/runtime-tests/run.log`; earlier pass also in
`work/win/tests-wine.log`).

Additional Windows checks pass against the final executable:

- Default AppData settings and output folder creation, ROM reads from the
  Unicode/spaces path, and remembered folder reopening from another directory.
- Quick save at VB1500, replacement of the same **4,758,712-byte** slot, and
  a fresh-process load. The VB1900 PNG after load is byte-identical to the
  original continuous windowed run (MD5 `c114f772f53057aa1908e918b61e1f87`).
  This checks that frame, not the entire P4.10 frame/RAM/PCM suite on Windows.
- Two SDL virtual gamepads connect as players 1/2 and pass the existing
  runtime's button/axis validation while running 200 fields. Real hardware
  hotplug and physical input remain untested.
- `play.bat` normal Start opens the launcher and closes its command shell;
  `--stats` takes the console branch and a ten-field game exits cleanly.
  Both were invoked through Wine `cmd`; logs are `batch-normal.log` and
  `batch-stats.log` under `work/win/verify/final/`.
- Final launcher capture: `work/win/verify/final/launcher.png`, rendered and
  inspected at 960x720, with the quick-load button now enabled. It retains
  the complete settings/control layout shown in the earlier captures.

The unpaced dummy-audio window checks report queue empty events; they are
not latency or speaker-quality measurements. Xvfb/Wine also logs missing
GLX warnings with GLX deliberately disabled. One batch launch logged Wine's
`Disk quota exceeded` warning, but both batch checks exited 0 and their
screenshots/settings/save files and all package checksums verified.

The fresh Linux reference is `work/win/build-linux/saturn_stv`, compiled from
the original snapshot. Its command was the requested default paid-input smoke:

```bash
SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy work/win/build-linux/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2000 --shots 1900 \
  --out work/win/verify/linux-smoke
```

Wine uses the same arguments and dummy video/audio for headless execution:

```bash
WINEPREFIX="$PWD/work/win/wineprefix-final" WINEARCH=win64 WINEDEBUG=-all \
WINEDLLOVERRIDES='mscoree,mshtml=' SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy \
work/win/opt/wine-11.21-amd64-wow64/bin/wine \
  work/win/DieHardArcade-windows-x64/DieHardArcade.exe \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2000 --shots 1900 \
  --out work/win/verify/final/wine-smoke
md5sum work/win/verify/{linux-smoke,final/wine-smoke}/shot-1900.png
```

The first vanilla amd64 Wine attempt failed before application loading:
`set_thread_area` in its i386 preloader was killed by SIGSYS (exit 159).
`work/win/wineboot.strace` and `wine-x64.strace` capture that failure.
The amd64 **WoW64** release works without this bootstrap. No syscall policy
or privilege changes were made. A concurrent Wine attempt was discarded
after exit 137 without captures; final Wine checks run sequentially within
one session/private prefix, against the final package.

Launcher evidence: `work/win/verify/launcher-wine.png`, then
`launcher-unicode.png` and `launcher-remembered.png`. All were captured with
`SATURN_LAUNCHER_SHOT` and inspected. The 960x720 launcher has legible paths,
Start/Load/Quit, P1/P2 keys and pad columns, game settings, CRT/scaling and
volume. A media path containing spaces and `é` persists and reopens from a
different working directory. The save/output check caught and fixed a missing default `shots` subfolder;
the Windows-only startup now creates the chosen output directory. The original
Windows SDL dummy window attempt
failed with `Couldn't create window: File not found`; launcher/window tests
therefore use the Windows video driver on a private Xvfb display and SDL's
software renderer with dummy audio. Xvfb runs with GLX disabled. These are
software presentation checks, not GPU/desktop/speaker measurements.

## Size and remaining checks

Final distribution folder: **15,764,536 bytes** (15.03 MiB).
ZIP: **5,455,112 bytes** (5.20 MiB), 16 files, integrity checked.
ZIP SHA-256: `eb815ad6817ff03bb9767adfe633d814d5d89edbf80d987c91e6bf9088302221`.
Executable SHA-256: `df95bfcd507ed6a3caa1af8f3b55e46fa7ac73a09be6a03e2fb5dbf60f45f7d2`.

`du -s -B1 work/win` reports **2,710,831,104 bytes** allocated, about
**2.53 GiB**, comfortably under 10 GB. This includes toolchain, SDL, working
portable Wine, both verification prefixes, builds, snapshots, archives and
captures. Peak before cleanup was about 3.4 GiB. The unused vanilla-Wine
extraction, preliminary package and failed configure directory were removed;
the checked download archives and failure logs remain for audit/reproduction.

Real Windows 10/11 installation and play still need the owner: double-click
startup, the interactive native folder picker, launcher Start, hardware
keyboard/gamepad hotplug/player order, actual speakers/audio latency,
fullscreen and mixed-DPI displays. The binary is unsigned; SmartScreen or
antivirus reputation prompts are possible. There is no installer, signing
certificate or automatic update mechanism. Preserve the entire extracted
folder and only run builds you trust.

Quick saves are tied to the exact source/compiler/ABI/ROM fingerprint;
Linux, macOS, other milestones and future Windows builds may reject them.
Existing P4.7 emulation fidelity gaps and the unimplemented BIOS TEST/EEPROM
lifecycle remain. No new real-Windows performance or hardware compatibility
claim is made from Wine.

## Ten-line summary

1. Windows x64 Release builds with llvm-mingw 20261104/UCRT and official SDL3 3.4.20.
2. Rebuild and package with `port/p412_build_win.sh`; shared Linux/macOS sources stay untouched.
3. Deliver `work/win/DieHardArcade-windows-x64.zip` (5.20 MiB), extracted as a whole.
4. Supply external `cart.bin` and `diehard_main.bin`; choose their folder or place them beside the exe.
5. Settings, screenshots and quick saves use `%APPDATA%\Die Hard Arcade`.
6. Wine's 2,000-VBlank smoke PNG exactly matches fresh Linux MD5 `e67bc072dedf1fef5f5286df70399bca`.
7. The 9,000-VBlank no-coin regression matches all four Linux PNGs, with identical stop/audio counts.
8. All 136 Windows runtime tests, launcher capture, virtual pads and quick-save overwrite/cold load pass.
9. Work/toolchain/build/evidence occupy 2.53 GiB after cleanup; no ROMs, sudo or git commit in delivery.
10. Real Windows hardware play and the interactive folder picker remain to test; the executable is unsigned.
