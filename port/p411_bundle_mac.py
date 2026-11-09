#!/usr/bin/env python3
"""P4.11 v1.00: assemble a standalone, ad-hoc signed "Die Hard Arcade.app".

Copies the game binary and every non-system dylib it needs (Homebrew SDL3) into
the bundle, rewrites install names/rpaths to @executable_path/../Frameworks,
adds the native ROM/settings launcher and third-party licenses, signs, then
checks that nothing references /usr/local, /opt/homebrew or other outside paths.
No ROM data is copied; ROMs live in ~/Library/Application Support/Die Hard Arcade/.
"""
from p422_package_mods import package_mods
import argparse
import plistlib
import re
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', default='port/run/P411/build-app/saturn_stv')
parser.add_argument('--sdl-prefix', help='SDL3 install prefix, for its license file')
parser.add_argument('--min-macos', default='14.0')
parser.add_argument('--out', default='port/run/P411/Die Hard Arcade.app')
args = parser.parse_args()

SYSTEM = ('/usr/lib/', '/System/Library/')
app = root / args.out
contents = app / 'Contents'
macos, frameworks, resources = contents / 'MacOS', contents / 'Frameworks', contents / 'Resources'


def run(*cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def deps(macho):
    """Install names this Mach-O links, excluding its own id line for dylibs."""
    lines = run('otool', '-L', str(macho)).splitlines()[1:]
    names = [line.strip().split(' (')[0] for line in lines]
    if macho.suffix == '.dylib' and names:
        own = run('otool', '-D', str(macho)).splitlines()[1:]
        names = [n for n in names if not own or n != own[0].strip()]
    return names


def rpaths(macho):
    return re.findall(r'cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (\S+)', run('otool', '-l', str(macho)))


def resolve(name, loader_rpaths):
    if name.startswith('@rpath/'):
        for rp in loader_rpaths:
            p = Path(rp) / name[len('@rpath/'):]
            if p.exists():
                return p
        raise SystemExit(f'cannot resolve {name} via {loader_rpaths}')
    return Path(name)


if app.exists():
    shutil.rmtree(app)
for d in (macos, frameworks, resources / 'licenses'):
    d.mkdir(parents=True)

game = macos / 'saturn_stv'
shutil.copy2(root / args.binary, game)
game.chmod(0o755)

# Copy non-system dylibs (transitively) and point every reference at the bundle.
queue, bundled = [(game, rpaths(root / args.binary))], {}
while queue:
    macho, loader_rpaths = queue.pop()
    for name in deps(macho):
        if name.startswith(SYSTEM) or name.startswith('@executable_path/'):
            continue
        src = resolve(name, loader_rpaths).resolve()
        base = Path(name).name
        if base not in bundled:
            dst = frameworks / base
            shutil.copy2(src, dst)
            dst.chmod(0o755)
            run('install_name_tool', '-id', f'@rpath/{base}', str(dst))
            bundled[base] = dst
            queue.append((dst, rpaths(src)))
        run('install_name_tool', '-change', name, f'@rpath/{base}', str(macho))
for macho in [game, *bundled.values()]:
    for rp in rpaths(macho):
        run('install_name_tool', '-delete_rpath', rp, str(macho))
run('install_name_tool', '-add_rpath', '@executable_path/../Frameworks', str(game))

# Geometry is generated locally; no copyrighted game artwork is used.
shutil.copy2(root / 'port/assets/p413-icon.icns', resources / 'DieHardArcade.icns')

launcher = macos / 'DieHardArcade'
run('clang', '-Os', '-arch', 'x86_64', f'-mmacosx-version-min={args.min_macos}', '-Wall', '-Wextra',
    str(root / 'port/p411_app_mac.c'), str(root / 'port/p413_roms.c'), '-o', str(launcher))

licenses = {'saturn-recomp-LICENSE': root / 'tools/saturn-recomp/LICENSE',
            'imgui-LICENSE.txt': root / 'tools/saturn-recomp/runtime/third_party/imgui/LICENSE.txt',
            'inter-font-LICENSE.txt': root / 'tools/saturn-recomp/runtime/third_party/inter/LICENSE.txt'}
if args.sdl_prefix:
    licenses['SDL3-LICENSE.txt'] = Path(args.sdl_prefix) / 'LICENSE.txt'
for name, src in licenses.items():
    if src.exists():
        shutil.copy2(src, resources / 'licenses' / name)

with (contents / 'Info.plist').open('wb') as f:
    plistlib.dump(dict(CFBundleExecutable='DieHardArcade', CFBundleName='Die Hard Arcade',
                      CFBundleDisplayName='Die Hard Arcade',
                      CFBundleIdentifier='org.saturn-recomp.diehard-arcade',
                      CFBundleIconFile='DieHardArcade.icns',
                      CFBundlePackageType='APPL', CFBundleShortVersionString='1.06',
                      CFBundleVersion='106', CFBundleInfoDictionaryVersion='6.0',
                      CFBundleSupportedPlatforms=['MacOSX'], LSMinimumSystemVersion=args.min_macos,
                      LSApplicationCategoryType='public.app-category.arcade-games',
                      NSHighResolutionCapable=True), f)

package_mods(resources / 'modding', (root / args.binary).parent / 'p421-target.txt')

run('codesign', '--force', '--deep', '--sign', '-', str(app))
run('codesign', '--verify', '--deep', '--strict', str(app))

# Standalone check: every Mach-O references only the OS or the bundle.
bad = []
for macho in [game, launcher, *bundled.values()]:
    for name in deps(macho) + rpaths(macho):
        if not (name.startswith(SYSTEM) or name.startswith('@')):
            bad.append(f'{macho.name}: {name}')
if bad:
    raise SystemExit('outside references remain:\n  ' + '\n  '.join(bad))
roms = [p.name for p in app.rglob('*') if p.suffix in ('.bin', '.zip', '.13', '.2', '.3', '.4', '.5')]
if roms:
    raise SystemExit(f'ROM-like files must not be bundled: {roms}')
print(f'App: {args.out} (bundled: {", ".join(sorted(bundled)) or "none"}; signed ad hoc)')
