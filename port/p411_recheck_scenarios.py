#!/usr/bin/env python3
"""Fresh native P4.7 captures against this milestone's hash-verified MAME captures."""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
from types import SimpleNamespace

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[1]
os.chdir(root)
sys.path.insert(0, str(root / 'tools/scenario'))
import check_scenarios as checker

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary', default='port/run/P46/build-sdl/saturn_stv')
p.add_argument('--baseline', type=Path, default=Path('work/scenario_p411_baseline'))
p.add_argument('--output', type=Path, default=Path('work/scenario_p411_final'))
a = p.parse_args()
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
saved = json.loads((a.baseline / 'manifest.json').read_text())
for group in ['tools', 'inputs']:
    for path, digest in saved[group].items():
        assert sha(path) == digest, 'Changed reference input/tool: ' + path
mame = 'port/run/P45/mamepkg/x/usr/games/mame'
assert sha(mame) == saved['binaries'][mame], 'MAME executable changed'
for path in ['port/diehard_main.bin', 'cart.bin']:
    assert sha(path) == saved['binaries'][path], 'ROM changed: ' + path
assert not a.output.exists(), 'Choose a fresh output directory'
a.output.mkdir(parents=True)
args = SimpleNamespace(port=a.binary, cart='port/diehard_main.bin', cartrom='cart.bin',
                       port_hook=True, timeout=1800, mame=mame, mame_lib='',
                       rompath='port/run/P45/mame/roms',
                       ram_frames={1, 7, 9, 11, 13, 314, 316, 702, 1526},
                       png={9, 11, 314, 316, 1526, 2100, 5000, 7177, 9300})
cal = checker.Scenario('calibration', 500, [(0, 0, 0)] * 501, [])
native = checker.capture_port(args, cal, a.output / 'calibration-port', video=False)
mcal = a.baseline / 'calibration-mame/frames.jsonl'
alignment = checker.boot_offset(native, checker.load_records(mcal))
checker.write_json(a.output / 'alignment.json', alignment)
manifest = dict(saved, alignment=alignment)
manifest['binaries'] = dict(saved['binaries'], **{a.binary: sha(a.binary)})
manifest['reuse'] = dict(mame_baseline=str(a.baseline), calibration_sha256=sha(mcal),
                         reason='Fresh P4.11 MAME captures; unchanged executable, ROM, inputs, tools and mask.')
checker.write_json(a.output / 'manifest.json', manifest)

def run(path):
    scenario = checker.read_scenario(path)
    base = a.output / scenario.name
    native = checker.capture_port(args, scenario, base / 'port', video=True)
    source = a.baseline / scenario.name / 'mame'
    shutil.copytree(source, base / 'mame', copy_function=os.link)
    reference = checker.load_records(source / 'frames.jsonl')
    checker.validate_frames(reference, scenario.frames + alignment['offset'])
    checker.write_json(base / 'mame/reuse.json', dict(source=str(source), records_sha256=sha(source / 'frames.jsonl')))
    result = checker.compare(scenario, native, reference, alignment)
    checker.write_json(base / 'comparison.json', result)
    print(scenario.name, result['status'], 'coverage', result['coverage'],
          'matched screens', result['matched_screens'], flush=True)
    return result

results = []
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    for result in pool.map(run, sorted(saved['inputs'])):
        results.append(result)
        results.sort(key=lambda r: r['name'])
        checker.write_json(a.output / 'results.json', dict(version=checker.VERSION, scenarios=results))
        (a.output / 'summary.md').write_text(checker.summary(results))
print(checker.summary(results))
