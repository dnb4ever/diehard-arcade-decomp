#!/usr/bin/env python3
"""Make a small in-tree launcher app; ROMs and Homebrew SDL3 stay external."""
from pathlib import Path
import plistlib
import subprocess

root = Path(__file__).resolve().parent.parent
contents = root / 'port/run/P48/Die Hard Arcade.app/Contents'
macos = contents / 'MacOS'
macos.mkdir(parents=True, exist_ok=True)
with (contents / 'Info.plist').open('wb') as f:
    plistlib.dump(dict(CFBundleExecutable='play', CFBundleName='Die Hard Arcade',
                      CFBundleIdentifier='org.saturn-recomp.diehard-stv',
                      CFBundlePackageType='APPL', CFBundleShortVersionString='1.00',
                      CFBundleVersion='100', CFBundleInfoDictionaryVersion='6.0',
                      CFBundleSupportedPlatforms=['MacOSX'], NSHighResolutionCapable=True), f)
play = macos / 'play'
subprocess.run(['clang', '-Os', '-arch', 'x86_64', '-Wall', '-Wextra',
                str(root / 'port/p48_app_mac.c'), '-o', str(play)], check=True)
print('App: port/run/P48/Die Hard Arcade.app')
