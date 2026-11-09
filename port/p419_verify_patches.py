#!/usr/bin/env python3
"""Reconstruct P4.13–P4.19 and isolated Windows sources with zero fuzz/offset."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_patches.py')
s=p.read_text().replace('work/p414/patch-check','work/p419/patch-check').replace('work/p414/windows-patch-check','work/p419/windows-patch-check')
s=s.replace("'p414-hires.patch']", "'p414-hires.patch','p415-retina-launcher.patch','p415-performance.patch','p416-netplay.patch','p417-linux-release.patch','p419-speculative-audio.patch','p419-nat-traversal.patch']")
exec(compile(s,str(p),'exec'))
