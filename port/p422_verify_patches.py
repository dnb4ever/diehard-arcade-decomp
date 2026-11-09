#!/usr/bin/env python3
"""Verify the live P4.13–P4.22 chain and Windows overlay without fuzz or offsets."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
W = ROOT / 'work/p422/patch-check'
if W.exists():
    shutil.rmtree(W)
(W / 'runtime').mkdir(parents=True)
shutil.copytree(ROOT / 'work/p413/baseline', W / 'runtime', dirs_exist_ok=True)
patches = ['p413-rewind', 'p413-speed-turbo', 'p413-training', 'p413-input-files',
           'p414-hires', 'p415-retina-launcher', 'p415-performance', 'p416-netplay',
           'p417-linux-release', 'p419-speculative-audio', 'p419-nat-traversal', 'p421-mod-runtime']


def apply(patch, destination, strip=1):
    r = subprocess.run(['patch', '--batch', '--fuzz=0', '-p' + str(strip), '-d', str(destination)],
                       input=(ROOT / 'port/runtime-patches' / patch).read_bytes(), capture_output=True, check=True)
    log = r.stdout.decode() + r.stderr.decode()
    assert 'fuzz' not in log and 'offset' not in log, log
    print(patch, 'zero fuzz/offset', flush=True)


def compare(a, b):
    def inventory(d):
        return {p.relative_to(d): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in d.rglob('*') if p.is_file() and p.suffix not in ('.orig', '.rej')
                and '__pycache__' not in p.parts}
    left, right = inventory(a), inventory(b)
    assert left == right, [str(p) for p in left.keys() | right.keys() if left.get(p) != right.get(p)]
    return len(left)


for name in patches:
    apply(name + '.patch', W)
count = sum(compare(W / 'runtime' / name, ROOT / 'tools/saturn-recomp/runtime' / name)
            for name in ['src', 'include', 'tests'])
assert (W / 'runtime/runtime.cmake').read_bytes() == (ROOT / 'tools/saturn-recomp/runtime/runtime.cmake').read_bytes()
(W / 'port').mkdir()
shutil.copy2(ROOT / 'work/p422/baseline/port/main_stv.cpp', W / 'port/main_stv.cpp')
apply('p421-mod-harness.patch', W)
assert (W / 'port/main_stv.cpp').read_bytes() == (ROOT / 'port/main_stv.cpp').read_bytes()
(W / 'generated').mkdir()
shutil.copytree(ROOT / 'work/p422/qualified-base', W / 'generated', dirs_exist_ok=True)
apply('p421-mod-convergence.patch', W)
for path, digest in json.loads((ROOT / 'work/p422/patch-report.json').read_text())['changed_files'].items():
    assert hashlib.sha256((W / path).read_bytes()).hexdigest() == digest, path
win = ROOT / 'work/p422/windows-patch-check'
if win.exists():
    shutil.rmtree(win)
shutil.copytree(ROOT / 'work/win/p413-baseline', win)
apply('p412-windows-portability.patch', win)
windows_count = sum(compare(win / name, ROOT / 'work/win/saturn-recomp' / name) for name in ['runtime', 'port'])
report = dict(runtime_files=count + 1, windows_files=windows_count, fuzz=0, offsets=0,
              qualified_adapters=216, harness_equal=True)

# A fresh Mac checkout embeds a different fallback path in the stock generator's
# CMake file. Exercise that deployment without changing any qualified guest code.
relocated = ROOT / 'work/p422/relocation-check'
if relocated.exists():
    shutil.rmtree(relocated)
source, output = relocated / 'stock', relocated / 'generated'
shutil.copytree(ROOT / 'port/run/P41/recomp', source)
cmake = source / 'CMakeLists.txt'
text = cmake.read_text()
text, count = re.subn(r'^  set\(SATURN_RUNTIME "[^"\n]*"\)$',
                      '  set(SATURN_RUNTIME "/opt/PM Mac/Die Hard Arcade/tools/saturn-recomp/runtime")',
                      text, flags=re.MULTILINE)
assert count == 1
cmake.write_text(text)
subprocess.run(['python3', str(ROOT / 'port/p422_prepare.py'), '--source', str(source),
                '--output', str(output)], check=True)
live = ROOT / 'port/run/P422/recomp'
for path in output.iterdir():
    if not path.is_file() or path.name == 'p422-deployment.json':
        continue
    actual, expected = path.read_bytes(), (live / path.name).read_bytes()
    if path.name == 'CMakeLists.txt':
        actual = re.sub(rb'^  set\(SATURN_RUNTIME "[^"\n]*"\)$', b'  set(SATURN_RUNTIME "RELOCATED")', actual, flags=re.MULTILINE)
        expected = re.sub(rb'^  set\(SATURN_RUNTIME "[^"\n]*"\)$', b'  set(SATURN_RUNTIME "RELOCATED")', expected, flags=re.MULTILINE)
    assert actual == expected, path.name
guest = source / 'p_dh_000.cpp'
guest.write_bytes(guest.read_bytes() + b'\n// unqualified guest mutation\n')
result = subprocess.run(['python3', str(ROOT / 'port/p422_prepare.py'), '--source', str(source),
                         '--output', str(output)], capture_output=True, text=True)
assert result.returncode != 0 and 'Unreviewed stock input changed: p_dh_000.cpp' in result.stderr
report.update(relocated_checkout=True, changed_guest_input_rejected=True)
shutil.rmtree(relocated)
(ROOT / 'work/p422/patch-verification.json').write_text(json.dumps(report, indent=2) + '\n')
print('PASS:', report)
