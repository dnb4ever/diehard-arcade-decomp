#!/usr/bin/env python3
"""Reconstruct P4.13–P4.16 and Windows staging byte-for-byte, zero fuzz/offset."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_patches.py')
s=p.read_text().replace('work/p414/patch-check','work/p416/patch-check').replace('work/p414/windows-patch-check','work/p416/windows-patch-check')
s=s.replace("'p414-hires.patch']", "'p414-hires.patch','p415-retina-launcher.patch','p415-performance.patch','p416-netplay.patch']")
exec(compile(s,str(p),'exec'))
