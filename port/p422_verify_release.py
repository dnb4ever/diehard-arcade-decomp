#!/usr/bin/env python3
"""Audit owner archives, source-only mod kits, exact targets and old-loader smoke."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parent.parent
os.chdir(ROOT)
work = Path('work/p422/release')
work.mkdir(parents=True, exist_ok=True)
env = {k: v for k, v in os.environ.items()
       if not k.startswith(('STV_', 'SATURN_', 'P416_', 'APPIMAGE_'))}
env['XDG_CONFIG_HOME'] = str((work / 'config').resolve())
app = Path('work/p417/DieHardArcade-x86_64.AppImage').resolve()
appdir = Path('work/p417/DieHardArcade.AppDir')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(base):
    return {str(p.relative_to(base)): 'link:' + os.readlink(p) if p.is_symlink() else sha(p)
            for p in base.rglob('*') if p.is_file() or p.is_symlink()}


for name in ['appimage', 'tar', 'windows']:
    destination = work / name
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir()
with (work / 'extract.log').open('w') as log:
    subprocess.run([str(app), '--appimage-extract'], cwd=work / 'appimage', env=env,
                   stdout=log, stderr=log, check=True, timeout=60)
with tarfile.open('work/p417/DieHardArcade-x86_64.tar.gz') as archive:
    archive.extractall(work / 'tar', filter='data')
expected = inventory(appdir)
assert inventory(work / 'appimage/squashfs-root') == expected
assert inventory(work / 'tar/DieHardArcade.AppDir') == expected
with zipfile.ZipFile('work/win/DieHardArcade-windows-x64.zip') as archive:
    assert archive.testzip() is None
    archive.extractall(work / 'windows')
windows = work / 'windows/DieHardArcade-windows-x64'
assert inventory(windows) == inventory(Path('work/win/DieHardArcade-windows-x64'))
for folder in [appdir, windows]:
    assert not any(re.search(r'cart\.bin|diehard_main\.bin|\.wav$|\.inputs$|\.quickstate$|shim|probe',
                             name, re.I) for name in inventory(folder))

kits = {}
for label, folder, build in [
    ('linux', appdir / 'modding', Path('work/p417/build-linux')),
    ('windows', windows / 'modding', Path('work/win/build-windows')),
]:
    assert (folder / 'p421-target.txt').read_bytes() == (build / 'p421-target.txt').read_bytes()
    for part in ['sdk', 'examples']:
        assert inventory(folder / part) == inventory(Path('port/mods') / part)
    assert (folder / 'build_mod.sh').read_bytes() == Path('port/mods/build_mod.sh').read_bytes()
    assert not any(p.suffix in ('.so', '.dll', '.dylib') or p.name == 'enabled.txt'
                   for p in folder.rglob('*'))
    kits[label] = {'target': (folder / 'p421-target.txt').read_text().strip(), 'source_only': True}

# Build and actually load the packaged Linux example against the packaged target.
example = work / 'mods/identity'
kit = appdir / 'modding'
subprocess.run([str(kit / 'build_mod.sh'), str(kit / 'examples/identity'), str(example),
                str(kit / 'p421-target.txt')], check=True, env=env)
sysroot = Path('work/p417/opt/sysroot').resolve()
package = appdir.resolve()
loader = [str(sysroot / 'lib64/ld-linux-x86-64.so.2'), '--library-path',
          f'{package}/usr/lib:{sysroot}/lib/x86_64-linux-gnu:{sysroot}/usr/lib/x86_64-linux-gnu',
          str(package / 'usr/bin/saturn_stv')]
smoke = {}
for label, mods in [('off', 'none'), ('identity', 'identity')]:
    output = work / ('glibc231-' + label)
    output.mkdir(exist_ok=True)
    command = loader + ['--cart', 'port/diehard_main.bin', '--cartrom', 'cart.bin',
                        '--mods-dir', str(work / 'mods'), '--mods', mods,
                        '--vblanks', '2000', '--shots', '1900', '--out', str(output)]
    with (output / 'run.log').open('w') as log:
        subprocess.run(command, env=env, stdout=log, stderr=log, check=True, timeout=180)
    digest = hashlib.md5((output / 'shot-1900.png').read_bytes()).hexdigest()
    assert digest == '9bd5386ea7439621c19674381086e8fd', (label, digest)
    smoke[label] = digest
manifest = json.loads((appdir / 'package-manifest.json').read_text())
assert max(tuple(map(int, s.split('.'))) for s in manifest['glibc_max'].values()) <= (2, 31)
identities = {str(build): re.search(r'STV_NETPLAY_CODE 0x([0-9a-f]+)',
                                  (build / 'p410_identity.h').read_text())[1]
              for build in [Path('port/run/P46/build-sdl'), Path('work/p417/build-linux'),
                            Path('work/win/build-windows')]}
assert len(set(identities.values())) == 1, identities
artifacts = {}
for name in ['work/p417/DieHardArcade-x86_64.AppImage', 'work/p417/DieHardArcade-x86_64.tar.gz',
             'work/win/DieHardArcade-windows-x64.zip']:
    path = Path(name)
    artifacts[name] = {'bytes': path.stat().st_size, 'sha256': sha(path)}
report = {'artifacts': artifacts, 'mod_kits': kits, 'netplay_builds': identities,
          'glibc231_smoke': smoke, 'glibc_max': manifest['glibc_max'],
          'appdir_entries': len(expected), 'windows_entries': len(inventory(windows))}
(work / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print('PASS: archive payloads, source-only kits, exact targets, glibc 2.31 off/identity smoke')
print(json.dumps(report, indent=2))
