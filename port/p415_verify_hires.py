#!/usr/bin/env python3
"""Run P4.14 authority/state invariants without overwriting its reference evidence."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_hires.py')
s=p.read_text().replace("Path('work/p414/verify-short' if a.short else 'work/p414/verify')", "Path('work/p415/verify-short' if a.short else 'work/p415/verify')")
exec(compile(s,str(p),'exec'))
