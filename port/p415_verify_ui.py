#!/usr/bin/env python3
"""Repeat real SDL launcher/pause events, preserving P4.14 captures."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_ui.py')
s=p.read_text().replace("Path('work/p414/ui')", "Path('work/p415/ui')")
exec(compile(s,str(p),'exec'))
