#!/usr/bin/env python3
"""P4.16 offline PNG/trace/WAV oracle, including the 9,000-field attract cycle."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p414_verify_regression.py')
exec(compile(p.read_text().replace("Path('work/p414/regression')", "Path('work/p416/regression')"),str(p),'exec'))
