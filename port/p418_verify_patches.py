#!/usr/bin/env python3
"""Reconstruct P4.13–P4.18 runtime and Windows staging with zero fuzz/offsets."""
from pathlib import Path
p = Path(__file__).resolve().with_name('p414_verify_patches.py')
s = p.read_text().replace('work/p414/patch-check', 'work/p418/patch-check').replace(
    'work/p414/windows-patch-check', 'work/p418/windows-patch-check')
s = s.replace("'p414-hires.patch']", "'p414-hires.patch','p415-retina-launcher.patch',"
              "'p415-performance.patch','p416-netplay.patch','p417-linux-release.patch']")
exec(compile(s, str(p), 'exec'))
