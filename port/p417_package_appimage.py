#!/usr/bin/env python3
"""P4.17 pinned no-root dependencies and ROM-free Linux release packaging."""
import argparse
import concurrent.futures
import hashlib
import html
import json
import lzma
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import urllib.request
from p422_package_mods import package_mods

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'work/p417'
OPT = WORK / 'opt'
ASSETS = [
    ('libsdl-org/SDL', 'release-3.4.20', 'SDL3-3.4.20.tar.gz', 'f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f'),
    ('libsdl-org/SDL', 'release-3.4.20', 'SDL3-3.4.20.tar.gz.sig', '776200b5c71f8c0168c0e89d8fb78103623dc15e284e9daf0a0eceb4e43d5533'),
    ('AppImage/appimagetool', '1.9.1', 'appimagetool-x86_64.AppImage', 'ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0'),
    ('AppImage/type2-runtime', '20251108', 'runtime-x86_64', '2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d'),
    ('AppImage/type2-runtime', '20251108', 'runtime-x86_64.sig', 'de8262d60b5f82bcff11b77598aedef86d2c375fc246d6a675f6cac68b6dfda7'),
    ('AppImage/type2-runtime', '20251108', 'signing-pubkey.asc', 'db8b615eb5bbf5e8418d52906b4a492960926370e2c5041ad6acaccda56ef0c6'),
]
# These are build-only development headers/link libraries. SDL dlopens platform drivers.
DEBS = '''libc6 libc6-dev libc-dev-bin libcrypt1 libcrypt-dev libnsl2 libnsl-dev
libtirpc3 libtirpc-dev libtirpc-common linux-libc-dev
gcc-10-base libgcc-s1 libstdc++6 libx11-dev libx11-6 libxext-dev libxext6
libxrandr-dev libxrandr2 libxcursor-dev libxcursor1 libxi-dev libxi6 libxfixes-dev
libxfixes3 libxss-dev libxss1 libxrender-dev libxrender1 libxkbcommon-dev libxkbcommon0
libwayland-bin libwayland-dev libwayland-client0 libwayland-server0 libwayland-cursor0 libwayland-egl1
libasound2-dev libasound2 libpulse-dev libpulse0 libudev-dev libudev1 libdbus-1-dev
libdbus-1-3 libxau-dev libxau6 libxdmcp-dev libxdmcp6 libxcb1-dev libxcb1 x11proto-dev
xtrans-dev libegl-dev libegl1 libglx-dev libglx0 libgl-dev libgl1 libgles-dev libgles1
libgles2 libglvnd-dev libglvnd0 libffi-dev libffi7 libbsd0 libmd0 libxtst-dev libxtst6'''.split()


def run(*args, **kwargs):
    return subprocess.run([str(a) for a in args], check=True, **kwargs)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def download(url, path, digest=None):
    if not path.exists():
        if os.environ.get('DHA_OFFLINE') == '1':
            raise SystemExit(f'Offline dependency missing: {path}')
        pending = path.with_name(path.name + '.part')
        urllib.request.urlretrieve(url, pending)
        pending.replace(path)
    if digest and sha(path) != digest:
        raise SystemExit(f'SHA256 mismatch: {path}')
    return path


def verify_signature(sig, target, fingerprint):
    result = subprocess.run(['gpg', '--homedir', str(OPT / 'gnupg'), '--batch',
                             '--status-fd=1', '--verify', str(sig), *([str(target)] if target else [])],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    # InRelease can contain additional signatures whose keys are not in this keyring.
    valid = re.findall(r'^\[GNUPG:\] VALIDSIG (\w+) ', result.stdout, re.M)
    if fingerprint not in valid:
        raise SystemExit(result.stderr + '\nRequired signing fingerprint absent: ' + fingerprint)
    print(result.stderr, end='')


def fetch():
    OPT.mkdir(parents=True, exist_ok=True)
    for repo, tag, name, digest in ASSETS:
        metadata = OPT / (repo.split('/')[-1] + '-release.json')
        download(f'https://api.github.com/repos/{repo}/releases/tags/{tag}', metadata)
        record = json.loads(metadata.read_text())
        asset = next(a for a in record['assets'] if a['name'] == name)
        if asset['digest'] != 'sha256:' + digest:
            raise SystemExit(f'Published digest changed: {name}')
        download(asset['browser_download_url'], OPT / name, digest)
        print(f'Verified {name}: {digest}', flush=True)
    for name in ['appimagetool-x86_64.AppImage', 'runtime-x86_64']:
        (OPT / name).chmod(0o755)
    sdl = OPT / 'SDL3-3.4.20'
    if not sdl.exists():
        with tarfile.open(OPT / 'SDL3-3.4.20.tar.gz') as archive:
            archive.extractall(OPT, filter='data')
    gpg = OPT / 'gnupg'
    gpg.mkdir(mode=0o700, exist_ok=True)
    signing_page = download('https://libsdl.org/signing-keys.php', OPT / 'sdl-signing-keys.html')
    keys = re.findall(r'-----BEGIN PGP PUBLIC KEY BLOCK-----.*?-----END PGP PUBLIC KEY BLOCK-----',
                      html.unescape(signing_page.read_text()), re.S)
    (OPT / 'sdl-keys.asc').write_text('\n'.join(keys) + '\n')
    debkey = download('https://ftp-master.debian.org/keys/archive-key-11.asc', OPT / 'debian-11.asc')
    for key in [OPT / 'sdl-keys.asc', OPT / 'signing-pubkey.asc', debkey]:
        run('gpg', '--homedir', gpg, '--batch', '--import', key)
    verify_signature(OPT / 'SDL3-3.4.20.tar.gz.sig', OPT / 'SDL3-3.4.20.tar.gz',
                     '0900104363B4C9D4223DE149D913FE7D4B61D39B')
    verify_signature(OPT / 'runtime-x86_64.sig', OPT / 'runtime-x86_64',
                     '570C77ACEA40C0F1B758902CBF96CCA56490F695')
    release = download('https://deb.debian.org/debian/dists/bullseye/InRelease', OPT / 'bullseye-InRelease',
                       '7917d4598768bfc4a25776eb0f6bf2f7d1f13301c2dc1e16467ef170dd752d40')
    verify_signature(release, None, 'A7236886F3CCCAAD148A27F80E98404D386FA1D9')
    index = download('https://deb.debian.org/debian/dists/bullseye/main/binary-amd64/Packages.xz',
                     OPT / 'bullseye-Packages.xz', '1c3405444297ead864711c18e64bce4810db63b48340a2f9d81182d9f235ad71')
    if not re.search(sha(index) + r'\s+' + str(index.stat().st_size) + r'\s+main/binary-amd64/Packages.xz', release.read_text()):
        raise SystemExit('Package index does not match signed InRelease')
    rows = {}
    for block in lzma.decompress(index.read_bytes()).decode().split('\n\n'):
        row = dict(line.split(': ', 1) for line in block.splitlines() if line and not line.startswith(' ') and ': ' in line)
        if 'Package' in row:
            rows[row['Package']] = row
    records = [{k: rows[name][k] for k in ['Package', 'Version', 'Filename', 'SHA256']} for name in DEBS]
    (OPT / 'deb').mkdir(exist_ok=True)
    sysroot = OPT / 'sysroot'
    sysroot.mkdir(exist_ok=True)

    def get_deb(row):
        return download('https://deb.debian.org/debian/' + row['Filename'],
                        OPT / 'deb' / Path(row['Filename']).name, row['SHA256'])

    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
        for archive in pool.map(get_deb, records):
            run('dpkg-deb', '-x', archive, sysroot)
    for path in sysroot.rglob('*'):
        if path.is_symlink() and path.readlink().is_absolute():
            target = sysroot / str(path.readlink()).lstrip('/')
            path.unlink()
            path.symlink_to(os.path.relpath(target, path.parent))
    (OPT / 'sysroot-packages.json').write_text(json.dumps(records, indent=2) + '\n')
    fetch_cpp11(sysroot)
    download('https://raw.githubusercontent.com/AppImage/type2-runtime/20251108/LICENSE',
             OPT / 'AppImage-runtime-LICENSE.txt', 'aa154fc9070614bbe7921f89db11efd1dba7a1f3a41685958110e2230f9c0ca1')
    for name, digest in [('COPYING3', '8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903'),
                         ('COPYING.RUNTIME', '9d6b43ce4d8de0c878bf16b54d8e7a10d9bd42b75178153e3af6a815bdc90f74')]:
        download('https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-11.5.0/' + name,
                 OPT / ('GCC11-' + name), digest)
    (WORK / 'toolchain.cmake').write_text(TOOLCHAIN)
    (OPT / 'downloads.json').write_text(json.dumps([dict(repository=r, tag=t, name=n, sha256=h) for r,t,n,h in ASSETS], indent=2) + '\n')


def fetch_cpp11(sysroot):
    # Existing C++20 atomic::wait requires GCC 11+. The focal build preserves glibc 2.31.
    base = 'https://ppa.launchpadcontent.net/ubuntu-toolchain-r/test/ubuntu/'
    fingerprint = 'C8EC952E2A0E1FBDC5090F6A2C277A0A352154E5'
    key = download('https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x' + fingerprint,
                   OPT / 'focal-toolchain-key.asc')
    run('gpg', '--homedir', OPT / 'gnupg', '--batch', '--import', key)
    release = download(base + 'dists/focal/InRelease', OPT / 'focal-toolchain-InRelease',
                       'ad2aa11d3dc4af499ccc1da00a1ce6a52ca39ff9228d3f2613a9e80e9aee941c')
    verify_signature(release, None, fingerprint)
    index = download(base + 'dists/focal/main/binary-amd64/Packages.xz', OPT / 'focal-toolchain-Packages.xz',
                     '906cdc622d6fd9975d36d0c4ba86dbcf95a949426ce174aa7e0ce8de99768653')
    if not re.search(sha(index) + r'\s+' + str(index.stat().st_size) + r'\s+main/binary-amd64/Packages.xz', release.read_text()):
        raise SystemExit('C++ package index does not match signed InRelease')
    rows = {}
    for block in lzma.decompress(index.read_bytes()).decode().split('\n\n'):
        row = dict(line.split(': ', 1) for line in block.splitlines() if line and not line.startswith(' ') and ': ' in line)
        if 'Package' in row:
            rows[row['Package']] = row
    records = []
    for name in ['libstdc++-11-dev', 'libgcc-11-dev', 'gcc-11-base']:
        row = rows[name]
        archive = download(base + row['Filename'], OPT / 'deb' / Path(row['Filename']).name, row['SHA256'])
        run('dpkg-deb', '-x', archive, sysroot)
        records.append({k: row[k] for k in ['Package', 'Version', 'Filename', 'SHA256']})
    (OPT / 'cpp11-packages.json').write_text(json.dumps(records, indent=2) + '\n')


TOOLCHAIN = '''get_filename_component(P417_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSROOT "${P417_ROOT}/opt/sysroot")
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(_gcc "${CMAKE_SYSROOT}/usr/lib/gcc/x86_64-linux-gnu/11")
set(CMAKE_C_FLAGS_INIT "-march=x86-64-v2")
set(CMAKE_CXX_FLAGS_INIT "-march=x86-64-v2 -nostdinc++ -isystem ${CMAKE_SYSROOT}/usr/include/c++/11 -isystem ${CMAKE_SYSROOT}/usr/include/x86_64-linux-gnu/c++/11 -isystem ${CMAKE_SYSROOT}/usr/include/c++/11/backward")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-B${_gcc}/ -B${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu/ -L${_gcc} -L${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu -L${CMAKE_SYSROOT}/lib/x86_64-linux-gnu -static-libstdc++ -static-libgcc -Wl,-rpath-link,${CMAKE_SYSROOT}/lib/x86_64-linux-gnu:${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-B${_gcc}/ -B${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu/ -L${_gcc} -L${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu -L${CMAKE_SYSROOT}/lib/x86_64-linux-gnu -static-libgcc -Wl,-rpath-link,${CMAKE_SYSROOT}/lib/x86_64-linux-gnu:${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/x86_64-linux-gnu/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
'''

README = '''Die Hard Arcade: Linux x86_64 release (P4.19)
Requires an x86-64-v2 CPU and glibc >= 2.31. No ROMs are included.

Linux: chmod +x DieHardArcade-x86_64.AppImage, then double-click or run it.
If FUSE is unavailable: APPIMAGE_EXTRACT_AND_RUN=1 ./DieHardArcade-x86_64.AppImage
Alternatively extract DieHardArcade-x86_64.tar.gz and run DieHardArcade.AppDir/AppRun.
Do not move AppRun away from its usr/ folder.

Supply your own supported MAME diehard (USA) set: diehard.zip, the five loose
members fpr19119.13 and mpr19115.2 through mpr19118.5, or prepared cart.bin.
Place it next to the AppImage (next to AppRun for the tar), or select its folder
with the built-in browser. --rom-dir FOLDER_OR_ZIP and --choose-roms also work.
The converter verifies SHA-256 and derives diehard_main.bin automatically.
Settings remain $XDG_DATA_HOME/saturn-recomp/stv.ini, falling back to
~/.local/share/saturn-recomp/stv.ini. Quick save: stv.ini.diehard.quickstate;
ROM cache: roms/; screenshots: shots/; remembered folder: rom-folder.txt.
All are in that same data directory. Existing settings are reused.
Explicit --cart commands retain the original headless harness behavior.

Steam Deck: use Desktop Mode, copy the AppImage to a permanent folder and mark
it executable (Properties > Permissions). In Steam select Games > Add a
Non-Steam Game > Browse, choose the AppImage, and add it. Use the native Linux
runtime: leave "Force compatibility tool" OFF. Select the Gamepad controller
layout so Steam Input exposes a standard SDL gamepad. Return to Gaming Mode
and start it from Library > Non-Steam. If FUSE fails, add the extracted AppRun
instead, or use launch options: APPIMAGE_EXTRACT_AND_RUN=1 %command%
SteamDeck=1 or nonempty SteamAppId selects fullscreen automatically. F11 toggles.
Use 1280x800 and default 1x internal resolution; 2x/3x cost more CPU.

D-pad/left stick navigates the launcher, A selects, B backs out. Select tabs
with the D-pad and A. Folder selection also supports a gamepad. Steam+X opens Steam's
keyboard for optional text fields (movie paths and netplay addresses).
P1/P2 are the first two connected SDL gamepads. Back/Select inserts a coin,
Start joins; Xbox X/A/B = arcade A/B/C. D-pad/left stick moves. Guide opens the
pause menu when Steam does not intercept it; keyboard Esc/P also opens it.
P1 keyboard: arrows, Z/X/C, 5 coin, 1 start. F5/F7 quick save/load; F12 screenshot.

This package bundles SDL3 3.4.20; C++/GCC support is linked statically.
Display/audio/controller drivers are loaded from the OS as needed (X11,
Wayland, ALSA/PulseAudio, udev); graphics drivers are deliberately not bundled.
No physical Deck, speakers, gamepad hardware or GPU was tested by P4.17.
Save states require an exact source/compiler/ROM fingerprint.
See licenses/ and package-manifest.json. Sources/build instructions are in
notes/port/p417_linux_release.md in the project checkout.
''' + '\n' + (ROOT / 'port/p419_netplay_help.txt').read_text()


def package():
    appdir = WORK / 'DieHardArcade.AppDir'
    if appdir.exists():
        shutil.rmtree(appdir)  # Wholly generated, restricted to this job's AppDir.
    binary = appdir / 'usr/bin/saturn_stv'
    binary.parent.mkdir(parents=True)
    (appdir / 'usr/lib').mkdir()
    shutil.copy2(WORK / 'build-linux/saturn_stv', binary)
    run('strip', '--strip-unneeded', binary)
    sdl = next((OPT / 'sdl/lib').glob('libSDL3.so.0.*'))
    dest = appdir / 'usr/lib' / sdl.name
    shutil.copy2(sdl.resolve(), dest)
    run('strip', '--strip-unneeded', dest)
    (dest.parent / 'libSDL3.so.0').symlink_to(dest.name)
    (appdir / 'AppRun').write_text('''#!/bin/sh
set -eu
APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export DHA_PACKAGE_DIR="$APPDIR"
export LD_LIBRARY_PATH="$APPDIR/usr/lib"
exec "$APPDIR/usr/bin/saturn_stv" "$@"
''')
    (appdir / 'AppRun').chmod(0o755)
    (appdir / 'diehard-arcade.desktop').write_text('''[Desktop Entry]
Type=Application
Name=Die Hard Arcade
Comment=Native ST-V arcade port; supply your own ROMs
Exec=AppRun
Icon=diehard-arcade
Terminal=false
Categories=Game;ActionGame;
StartupWMClass=Die Hard Arcade
''')
    shutil.copy2(ROOT / 'port/assets/p413-icon.png', appdir / 'diehard-arcade.png')
    (appdir / '.DirIcon').symlink_to('diehard-arcade.png')
    (appdir / 'README-linux.txt').write_text(README + '\nMod SDK and source examples: modding/README-package.txt\n')
    package_mods(appdir / 'modding', WORK / 'build-linux/p421-target.txt')
    licenses = appdir / 'licenses'
    licenses.mkdir()
    rt = WORK / 'saturn-recomp/runtime'
    sources = {'SDL3.txt': OPT / 'SDL3-3.4.20/LICENSE.txt',
               'saturn-recomp.txt': WORK / 'saturn-recomp/LICENSE',
               'Dear-ImGui.txt': rt / 'third_party/imgui/LICENSE.txt',
               'Inter.txt': rt / 'third_party/inter/LICENSE.txt'}
    for name, relative in {'Musashi.txt':'musashi/m68kcpu.c', 'SoftFloat.txt':'musashi/softfloat/softfloat.c',
                           'stb_truetype.txt':'stb/stb_truetype.h', 'minicoro.txt':'minicoro/minicoro.h'}.items():
        sources[name] = rt / 'third_party' / relative
    sources['AppImage-runtime.txt'] = OPT / 'AppImage-runtime-LICENSE.txt'
    sources['GPL-3.txt'] = OPT / 'GCC11-COPYING3'
    sources['GCC-exception.txt'] = OPT / 'GCC11-COPYING.RUNTIME'
    sources['GCC-runtime.txt'] = OPT / 'sysroot/usr/share/doc/gcc-11-base/copyright'
    for name, source in sources.items():
        shutil.copy2(source, licenses / name)
    # The strict allowlist makes accidental ROM/data inclusion a packaging error.
    allowed = {'AppRun', '.DirIcon', 'diehard-arcade.desktop', 'diehard-arcade.png',
               'README-linux.txt', 'package-manifest.json', 'usr/bin/saturn_stv',
               'usr/lib/libSDL3.so.0', 'usr/lib/' + dest.name}
    graph, versions = {}, {}
    for path in [binary, dest]:
        result = run('ldd', path, capture_output=True, text=True, env={**os.environ, 'LD_LIBRARY_PATH': str(dest.parent)})
        graph[str(path.relative_to(appdir))] = result.stdout
        for line in result.stdout.splitlines():
            if 'not found' in line:
                raise SystemExit(line)
            match = re.search(r'=> (/.+?) \(', line)
            if match:
                resolved = Path(match[1]).resolve()
                if not resolved.is_relative_to(appdir) and resolved.name not in {'libc.so.6','libm.so.6','libdl.so.2','libpthread.so.0','librt.so.1'}:
                    raise SystemExit(f'Unexpected external dependency: {line}')
        symbols = run('objdump', '-T', path, capture_output=True, text=True).stdout
        required = sorted(set(re.findall(r'\bGLIBC_(\d+\.\d+(?:\.\d+)?)\b', symbols)), key=lambda v: tuple(map(int,v.split('.'))))
        versions[str(path.relative_to(appdir))] = required[-1]
        if tuple(map(int, required[-1].split('.'))) > (2,31):
            raise SystemExit(f'glibc 2.31 baseline exceeded: {path}: {required[-1]}')
    manifest = dict(architecture='x86-64-v2', sdl='3.4.20', glibc_max=versions,
                    dependencies=graph, snapshot_sha256=sha(WORK / 'snapshot.json'), sha256={})
    for path in sorted(appdir.rglob('*')):
        if path.is_file():
            relative = path.relative_to(appdir).as_posix()
            if relative not in allowed and not relative.startswith(('licenses/', 'modding/')):
                raise SystemExit(f'Unexpected package file: {relative}')
            manifest['sha256'][relative] = sha(path)
    (appdir / 'package-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    with tarfile.open(WORK / 'DieHardArcade-x86_64.tar.gz', 'w:gz') as archive:
        archive.add(appdir, arcname=appdir.name)
    run(OPT / 'appimagetool-x86_64.AppImage', '--no-appstream', '--comp', 'zstd',
        '--mksquashfs-opt', '-processors', '--mksquashfs-opt', '2',
        '--runtime-file', OPT / 'runtime-x86_64', appdir, WORK / 'DieHardArcade-x86_64.pending.AppImage',
        env={**os.environ, 'APPIMAGE_EXTRACT_AND_RUN':'1', 'ARCH':'x86_64'})
    (WORK / 'DieHardArcade-x86_64.pending.AppImage').replace(WORK / 'DieHardArcade-x86_64.AppImage')
    for path in [WORK / 'DieHardArcade-x86_64.AppImage', WORK / 'DieHardArcade-x86_64.tar.gz']:
        print(f'{path.relative_to(ROOT)}: {path.stat().st_size:,} bytes; SHA256 {sha(path)}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fetch', action='store_true', help='fetch and verify pinned build-only dependencies')
    args = parser.parse_args()
    fetch() if args.fetch else package()
