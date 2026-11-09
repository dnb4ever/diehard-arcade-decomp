#!/usr/bin/env python3
"""Package the isolated P4.12 x64 build; inspect imports, never copy ROMs."""
from p422_package_mods import package_mods
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
README = """Die Hard Arcade - Windows x64 (P4.19)

Requires 64-bit Windows 10 or Windows 11. Extract the ENTIRE zip into a folder
before playing. Keep DieHardArcade.exe and its DLLs together. No installer or
administrator rights are needed. This executable is unsigned; Windows may
show a SmartScreen prompt. Only run a build you trust.

ROMs are NOT included. Supply the supported MAME diehard (USA) set as diehard.zip,
or its five loose files (fpr19119.13, mpr19115.2 through mpr19118.5). Drop the zip
into %APPDATA%\\Die Hard Arcade, or put it in another folder and choose that
folder at startup. A zip path also works with --rom-dir. Preparation happens
inside the app, without Python, PowerShell, unzip, or an external emulator.
Prepared cart.bin is accepted after SHA-256 verification; the program slice is
derived automatically. The app stores verified images in AppData. No ROMs are
written into this distribution folder. Review launcher settings and click Start.
To choose a different folder: play.bat --choose-roms
To use a specific folder: play.bat --rom-dir "D:\\Games\\My ROMs"
To play immediately: play.bat --no-launcher

Settings, ROM-folder choice, quick save, and screenshots are stored under:
  %APPDATA%\\Die Hard Arcade\\
The INI is stv.ini; F5 writes stv.ini.diehard.quickstate, F7 loads it.
Quick saves belong to this exact build and these ROM images; another build
may reject them. This is one quick slot, not original cabinet bookkeeping.
To reset settings, close the game and remove stv.ini. To reset ROM selection,
remove rom-folder.txt. --choose-roms selects another source; verified cached
images normally take priority.

P1: arrows = move, Z/X/C = A/B/C, 5 = coin, 1 = Start.
P2: R/F/D/G = up/down/left/right, A/S/Q = A/B/C, 6 = coin, 2 = Start.
Escape/P = pause menu; F11 or Alt+Enter = fullscreen; F12 = screenshot.
F5/F7 = quick save/load. F3 = cabinet pause if enabled in game settings.
Player features / training: enable up to 20 seconds of rewind (Backspace / pad
left shoulder, remappable), hold Tab or toggle F4 for fast-forward with muted
audio, and set independent P1/P2 A/B/C autofire rates. Training health/lives/
credits and next-power-on scene select default off; active cheats are marked.
Internal resolution: 1x (original, default), 2x, or 3x (CPU intensive).
Choose it in Display and sound, or pause > Player features / training.
High resolution changes presentation only. Lower it if performance drops.
Input files: Record from power-on / quick save or Play input file. Files are
versioned and tied to this build/ROM set. Quit normally to finish recording.
Xbox/PlayStation/Switch-style gamepads use SDL3's gamepad API. The first two
connected pads are P1/P2. D-pad/left stick = move; Xbox X/A/B = A/B/C;
Back/Select = coin, Start = Start, Guide = pause. Edit each player's layout
in the launcher. Connect pads before launch for predictable player order.
Avoid TEST/F2: the original BIOS service menu is not implemented.

@@P419_NETPLAY@@

The normal app opens no console. Use play.bat --stats 5 to attach/open one
and report speed/audio statistics. From a terminal you can redirect stdout
and stderr to a log. Advanced harness flags (--cart, --cartrom, --vblanks,
--shots, --out, --settings) remain available; an explicit --cart selects the
headless harness defaults unless --window or --launcher is also supplied.

Cross-built on Linux with llvm-mingw/UCRT and the official SDL3 MinGW release.
P4.19 verification and platform limits: notes/port/p419_netplay_polish.md in
the source repository. Real Windows, physical controllers, speaker audio,
fullscreen and mixed-DPI monitors still need an owner play-test.
"""
README += "\nMod SDK and source examples: modding/README-package.txt\n"
README = README.replace("@@P419_NETPLAY@@", (ROOT / "port/p419_netplay_help.txt").read_text())


def imports(path, readobj):
    result = subprocess.run([str(readobj), "--file-headers", "--coff-imports", str(path)],
                            check=True, capture_output=True, text=True).stdout
    if "IMAGE_FILE_MACHINE_AMD64" not in result:
        raise RuntimeError(f"Not an x64 PE: {path}")
    return re.findall(r"^  Name: (.+\.dll)$", result, re.M | re.I)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=ROOT / "work/win/build-windows/DieHardArcade.exe")
    parser.add_argument("--toolchain", type=Path, default=ROOT / "work/win/opt/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64")
    parser.add_argument("--sdl", type=Path, default=ROOT / "work/win/opt/SDL3-3.4.20/x86_64-w64-mingw32")
    parser.add_argument("--output", type=Path, default=ROOT / "work/win/DieHardArcade-windows-x64")
    parser.add_argument("--zip", action="store_true", help="also produce OUTPUT.zip")
    args = parser.parse_args()
    readobj = args.toolchain / "bin/llvm-readobj"
    out = args.output.resolve()
    # Never zip a previous owner's ROMs or saves. Only our generated package is reusable.
    allowed = {"DieHardArcade.exe", "play.bat", "README-windows.txt", "package-manifest.json", "licenses", "modding"}
    candidates = {p.name.lower(): p for p in (args.toolchain / "x86_64-w64-mingw32/bin").glob("*.dll")}
    candidates["sdl3.dll"] = args.sdl / "bin/SDL3.dll"
    allowed.update(p.name for p in candidates.values())
    if out.exists():
        unknown = {p.name for p in out.iterdir()} - allowed
        if unknown:
            raise RuntimeError(f"Refusing to package unrecognized files: {sorted(unknown)}")
        for p in out.glob("*.dll"):
            p.unlink()  # Remove obsolete copied runtimes; dependencies are rediscovered below.
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.exe, out / "DieHardArcade.exe")
    subprocess.run([str(args.toolchain / "bin/llvm-strip"), str(out / "DieHardArcade.exe")], check=True)
    queue = [out / "DieHardArcade.exe"]
    graph = {}
    # These imports are supplied by Windows 10/11. Every other DLL must be bundled.
    system = {n.lower() for n in "kernel32 user32 shell32 ole32 oleaut32 ws2_32 iphlpapi advapi32 gdi32 imm32 msvcrt setupapi version winmm ntdll bcrypt crypt32 shlwapi comdlg32 ucrtbase".split()}
    while queue:
        binary = queue.pop()
        names = imports(binary, readobj)
        graph[binary.name] = names
        for name in names:
            lower = name.lower()
            if lower in candidates:
                source = candidates[lower]
                dest = out / source.name
                if not dest.exists():
                    shutil.copy2(source, dest)
                    queue.append(dest)
            elif lower.removesuffix(".dll") not in system and not lower.startswith(("api-ms-win-", "ext-ms-win-")):
                raise RuntimeError(f"Unresolved import: {binary.name}: {name}")
    (out / "README-windows.txt").write_bytes(README.replace("\n", "\r\n").encode("utf-8"))
    (out / "play.bat").write_bytes(b'@echo off\r\nsetlocal\r\npushd "%~dp0"\r\nfor %%A in (%*) do if /I "%%~A"=="--stats" goto console\r\nstart "" "%~dp0DieHardArcade.exe" %*\r\npopd\r\nexit /b\r\n:console\r\n"%~dp0DieHardArcade.exe" %*\r\nset "DHA_EXIT=%ERRORLEVEL%"\r\npopd\r\nexit /b %DHA_EXIT%\r\n')
    licenses = out / "licenses"
    if licenses.exists():
        shutil.rmtree(licenses)  # This directory is wholly generated by this script.
    licenses.mkdir()
    runtime = ROOT / "work/win/saturn-recomp/runtime"
    sources = {"SDL3.txt": args.sdl.parent / "LICENSE.txt",
               "llvm-mingw.txt": args.toolchain / "LICENSE.TXT",
               "saturn-recomp.txt": ROOT / "tools/saturn-recomp/LICENSE",
               "Dear-ImGui.txt": runtime / "third_party/imgui/LICENSE.txt",
               "Inter.txt": runtime / "third_party/inter/LICENSE.txt"}
    for name, source in sources.items():
        shutil.copy2(source, licenses / name)
    # These dependencies carry their license notices in source headers.
    for name, source in {"Musashi.txt": "musashi/m68kcpu.c", "SoftFloat.txt": "musashi/softfloat/softfloat.c",
                         "stb_truetype.txt": "stb/stb_truetype.h", "minicoro.txt": "minicoro/minicoro.h"}.items():
        shutil.copy2(runtime / "third_party" / source, licenses / name)
    package_mods(out / "modding", args.exe.parent / "p421-target.txt")
    manifest = {"architecture": "x86_64", "subsystem": "Windows GUI", "imports": graph, "sha256": {}}
    for path in sorted(out.rglob("*")):
        if path.is_file() and path.name != "package-manifest.json":
            manifest["sha256"][path.relative_to(out).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "package-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if args.zip:
        with zipfile.ZipFile(out.with_suffix(".zip"), "w", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(out.rglob("*")):
                if path.is_file():
                    archive.write(path, Path(out.name) / path.relative_to(out))
        print(out.with_suffix(".zip").relative_to(ROOT))
    print(f"{out.relative_to(ROOT)}: {sum(p.stat().st_size for p in out.rglob('*') if p.is_file()):,} bytes")
    print("Bundled DLLs:", ", ".join(sorted(p.name for p in out.glob("*.dll"))))


if __name__ == "__main__":
    main()
