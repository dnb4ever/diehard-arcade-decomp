#!/usr/bin/env python3
"""P4.19 offline oracle against every retained P4.11–P4.16 reference."""
from pathlib import Path
p=Path(__file__).resolve().with_name('p418_verify_regression.py')
exec(compile(p.read_text().replace("Path('work/p418/regression')","Path('work/p419/regression')"),str(p),'exec'))
