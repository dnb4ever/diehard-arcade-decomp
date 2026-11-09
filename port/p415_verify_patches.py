#!/usr/bin/env python3
"""Reconstruct P4.13 through P4.15 and current Windows staging, without fuzz/offsets."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_patches.py')
s=p.read_text().replace("work/p414/patch-check", "work/p415/patch-check").replace("work/p414/windows-patch-check", "work/p415/windows-patch-check")
s=s.replace("'p414-hires.patch']", "'p414-hires.patch','p415-retina-launcher.patch','p415-performance.patch']")
exec(compile(s,str(p),'exec'))
