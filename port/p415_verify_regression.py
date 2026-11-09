#!/usr/bin/env python3
"""Run the unchanged P4.14 default oracle into this milestone's evidence directory."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_regression.py')
exec(compile(p.read_text().replace("Path('work/p414/regression')", "Path('work/p415/regression')"),str(p),'exec'))
