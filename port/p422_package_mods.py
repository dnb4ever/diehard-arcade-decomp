#!/usr/bin/env python3
"""Package only the public mod SDK, source examples and exact app target; no native libraries."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent.parent


def package_mods(destination, target):
    destination, target = Path(destination), Path(target)
    digest = target.read_text().strip()
    if len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest):
        raise ValueError('Missing exact mod build target: ' + str(target))
    # This subtree is generated package content only, never the user's mods folder.
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    for name in ['sdk', 'examples']:
        shutil.copytree(ROOT / 'port/mods' / name, destination / name)
    for name in ['README.md', 'build_mod.sh']:
        shutil.copy2(ROOT / 'port/mods' / name, destination / name)
    shutil.copy2(target, destination / 'p421-target.txt')
    (destination / 'README-package.txt').write_text('''Example mods are shipped as source and are disabled by default.
Build each for this app's platform/architecture using build_mod.sh and
p421-target.txt. Put the output folders in the Mods tab's displayed folder.
Linux: $XDG_CONFIG_HOME/saturn-recomp/mods (default ~/.config/saturn-recomp/mods)
macOS: ~/Library/Application Support/Die Hard Arcade/mods
Windows: %APPDATA%/Die Hard Arcade/mods
Enable selected examples explicitly in the launcher. Each marks play MODDED.
Rebuild after updating the app; use identical enabled native files for netplay.
No prebuilt .so/.dll/.dylib files or saved selections are included.
''')
