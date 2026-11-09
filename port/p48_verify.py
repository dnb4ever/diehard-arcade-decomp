#!/usr/bin/env python3
"""Replay P4.5/P4.6 schedules and write portable PNG/trace hash manifests."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', default='port/run/P48/build-headless/saturn_stv')
parser.add_argument('--label', default='headless')
parser.add_argument('--schedules', default='nocoin,gameover,combat,legacy-combat,fc1c')
parser.add_argument('--reference', type=Path, help='manifest from another build/host')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
os.chdir(root)
combat = '200:COIN,340:START1,1350:P1RIGHT:100,' + ','.join(f'{v}:P1A:2' for v in range(1450, 1851, 50))
combat_shots = [1320, 1440, 1560, 1800, 2000]
# The old run/*.sh files were excluded from this copy. Reconstruct the four
# documented schedules; the 16 game-over capture times are explicit here.
schedules = {
    'nocoin': (9000, '99999:COIN', list(range(300, 9001, 300)), {}),
    'gameover': (9000, '600:COIN,700:START1,6000:COIN,6100:START1',
                 [600, 700, 1000, 1500, 2000, 3000, 4000, 4500, 4893, 5000, 5195, 5300, 6000, 6050, 6100, 6926], {}),
    'combat': (2100, combat, combat_shots, {}),
    'legacy-combat': (2100, combat, combat_shots, {'STV_BOOTMODE': '0', 'STV_CFG798': '40'}),
    'fc1c': (7600, '99999:COIN,7104:P1UP:400', [7104, 7350, 7600],
             {'STV_AUTOPLAY': '100', 'STV_COVDUMP': '1'}),
    'fc1c-retimed': (8500, '99999:COIN,7104:P1UP:400,7850:P1UP:500', [7850, 8100, 8500],
                     {'STV_AUTOPLAY': '100', 'STV_COVDUMP': '1'}),
}
dest = Path('port/run/P48') / args.label
dest.mkdir(parents=True, exist_ok=True)
env = {k: v for k, v in os.environ.items() if not k.startswith('STV_')}
env['STV_SCENETRACE'] = '1'
manifest = {}
for name in args.schedules.split(','):
    limit, timeline, shots, overrides = schedules[name]
    out = dest / name
    out.mkdir(exist_ok=True)
    for vb in shots:
        (out / f'shot-{vb}.png').unlink(missing_ok=True)
    cmd = [args.binary, '--cart', 'port/diehard_main.bin', '--cartrom', 'cart.bin',
           '--vblanks', str(limit), '--stvinput', timeline, '--shots', ','.join(map(str, shots)), '--out', str(out)]
    start = time.monotonic()
    with (out / 'run.log').open('w') as log:
        run = subprocess.run(cmd, env=dict(env, **overrides), stdout=log, stderr=subprocess.STDOUT, timeout=600)
    elapsed = time.monotonic() - start
    log = (out / 'run.log').read_text()
    # Keep deterministic guest events; elapsed host time and output paths are excluded.
    trace = '\n'.join(line for line in log.splitlines() if re.search(
        r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples', line)) + '\n'
    (out / 'trace.txt').write_text(trace)
    hashes = {}
    for vb in shots:
        p = out / f'shot-{vb}.png'
        if p.exists():
            data = p.read_bytes()
            hashes[p.name] = {'md5': hashlib.md5(data).hexdigest(), 'sha256': hashlib.sha256(data).hexdigest()}
    manifest[name] = dict(command=cmd, overrides=overrides, exit_code=run.returncode,
                          seconds=round(elapsed, 3), vblanks_per_second=round(limit / elapsed, 2),
                          expected_shots=len(shots), png=hashes, trace_sha256=hashlib.sha256(trace.encode()).hexdigest())
    print(f'{name}: exit={run.returncode}, {len(hashes)}/{len(shots)} PNGs, {elapsed:.3f}s, {limit / elapsed:.2f} VB/s', flush=True)
path = dest / 'manifest.json'
path.write_text(json.dumps(manifest, indent=2) + '\n')
if args.reference:
    ref = json.loads(args.reference.read_text())
    differences = []
    for name, result in manifest.items():
        for key in ('png', 'trace_sha256'):
            if ref.get(name, {}).get(key) != result[key]:
                differences.append(f'{name}: {key}')
    print('Comparison: ' + (', '.join(differences) if differences else 'all hashes match'))
    if differences:
        raise SystemExit(1)
if any(r['exit_code'] or len(r['png']) != r['expected_shots'] for r in manifest.values()):
    raise SystemExit(1)
