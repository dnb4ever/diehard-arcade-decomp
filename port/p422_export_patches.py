#!/usr/bin/env python3
"""Re-export the P4.21 delta against the preserved P4.19 integration baseline."""
import difflib
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / 'work/p422'


def diff(old, new, prefix, paths):
    lines, hashes = [], {}
    for rel in sorted(paths):
        a, b = old / rel, new / rel
        if a.exists() and b.exists() and a.read_bytes() == b.read_bytes():
            continue
        lines.extend(difflib.unified_diff(a.read_text().splitlines(True) if a.exists() else [],
                                         b.read_text().splitlines(True) if b.exists() else [],
                                         fromfile='a/' + prefix + '/' + str(rel) if a.exists() else '/dev/null',
                                         tofile='b/' + prefix + '/' + str(rel) if b.exists() else '/dev/null'))
        if b.exists():
            hashes[prefix + '/' + str(rel)] = hashlib.sha256(b.read_bytes()).hexdigest()
    return ''.join(lines), hashes


def export():
    changed = {}
    for name, old, new, prefix, paths in [
        ('runtime', WORK / 'baseline/runtime', ROOT / 'tools/saturn-recomp/runtime', 'runtime', None),
        ('harness', WORK / 'baseline/port', ROOT / 'port', 'port', [Path('main_stv.cpp')]),
        ('convergence', WORK / 'qualified-base', WORK / 'qualified-merged', 'generated', None),
    ]:
        if paths is None:
            paths = {p.relative_to(t) for t in [old, new] for p in t.rglob('*')
                     if p.is_file() and p.suffix not in ('.orig', '.rej') and '__pycache__' not in p.parts}
        patch, hashes = diff(old, new, prefix, paths)
        (ROOT / 'port/runtime-patches' / f'p421-mod-{name}.patch').write_text(patch)
        changed.update(hashes)
    (WORK / 'patch-report.json').write_text(json.dumps(dict(changed_files=changed), indent=2) + '\n')
    print('Exported rebased loader, harness and convergence patches:', len(changed), 'changed files')


if __name__ == '__main__':
    export()
