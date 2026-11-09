#!/usr/bin/env python3
"""Capture all 56 reference fields, preserve raw input identity and audit intended renderer changes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('phase', choices=['before', 'after', 'physical', 'compare'])
p.add_argument('--binary', default='port/run/P46/build-sdl/saturn_stv')
p.add_argument('--probe', default='work/p411/probes/p411_render_dump')
p.add_argument('--reference', type=Path, default=Path('port/run/P48/p410-final/manifest.json'))
p.add_argument('--output', type=Path, default=Path('work/p411/regression'))
p.add_argument('--schedules', default='nocoin,gameover,combat,legacy-combat')
a = p.parse_args()
os.chdir(Path(__file__).resolve().parents[1])
reference = json.loads(a.reference.read_text())
names = a.schedules.split(',')
assert names and all(n in reference for n in names), 'Unknown schedule'
sha = lambda data: hashlib.sha256(data).hexdigest()
swap = {'UP': 'DOWN', 'DOWN': 'UP', 'LEFT': 'RIGHT', 'RIGHT': 'LEFT'}

if a.phase != 'compare':
    root = a.output / a.phase
    root.mkdir(parents=True, exist_ok=True)
    report = {}
    for name in names:
        ref = reference[name]
        cmd = [a.binary] + ref['command'][1:]
        out = root / name
        out.mkdir(exist_ok=True)
        cmd[cmd.index('--out') + 1] = str(out)
        at = cmd.index('--stvinput') + 1
        old_input = cmd[at]
        if a.phase == 'after':
            # P4.1-P4.10 direction labels were reversed. Preserve their guest raw words
            # for regression; the separate physical pass uses the corrected labels.
            cmd[at] = re.sub(r'P([12])(UP|DOWN|LEFT|RIGHT)',
                             lambda m: 'P' + m[1] + swap[m[2]], cmd[at])
        shots = cmd[cmd.index('--shots') + 1]
        cmd += ['--wav', str(out / 'audio.wav')]
        env = {k: v for k, v in os.environ.items() if not k.startswith('STV_')}
        env.update(ref.get('overrides', {}))
        env.update(STV_SCENETRACE='1', STV_CAPTURE_FIELDS=shots)
        start = time.monotonic()
        with (out / 'run.log').open('w') as log:
            result = subprocess.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=1200)
        log = (out / 'run.log').read_text()
        assert result.returncode == 0 and 'FATAL' not in log, name
        trace = '\n'.join(line for line in log.splitlines() if re.search(
            r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples', line)) + '\n'
        guest = '\n'.join(line for line in log.splitlines() if re.search(
            r'SCENETRACE|ST-V coin:|FATAL|stopped after', line)) + '\n'
        (out / 'trace.txt').write_text(trace)
        (out / 'guest-trace.txt').write_text(guest)
        pngs = {f'shot-{v}.png': sha((out / f'shot-{v}.png').read_bytes()) for v in shots.split(',')}
        for vb in shots.split(','):
            assert (out / f'dump-{vb}.bin').stat().st_size == 0x141220
            assert (out / f'shown-{vb}.bin').stat().st_size == 1024 * 512 * 2
        report[name] = dict(command=cmd, env={k: v for k, v in env.items() if k.startswith('STV_')},
                            original_input=old_input, seconds=round(time.monotonic() - start, 3),
                            png=pngs, guest_trace_sha256=sha(guest.encode()), trace_sha256=sha(trace.encode()),
                            wav_sha256=sha((out / 'audio.wav').read_bytes()))
        if a.phase == 'before':
            assert pngs == {k: v['sha256'] for k, v in ref['png'].items()}, name + ': baseline PNG changed'
            assert sha(trace.encode()) == ref['trace_sha256'], name + ': baseline trace changed'
        print(a.phase, name, 'PASS', len(pngs), 'PNGs', flush=True)
        (root / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
else:
    before = json.loads((a.output / 'before/manifest.json').read_text())
    after = json.loads((a.output / 'after/manifest.json').read_text())
    result = {}
    for name in names:
        assert before[name]['guest_trace_sha256'] == after[name]['guest_trace_sha256'], name + ': guest trace changed'
        out = a.output / 'predicted' / name
        out.mkdir(parents=True, exist_ok=True)
        frames = {}
        for png, old_sha in before[name]['png'].items():
            vb = png[5:-4]
            source = a.output / 'before' / name
            base = [a.probe, str(source / f'dump-{vb}.bin'), str(source / f'shown-{vb}.bin')]
            subprocess.run(base + [str(out / png)], check=True, stdout=subprocess.DEVNULL)
            predicted = sha((out / png).read_bytes())
            assert predicted == after[name]['png'][png], f'{name}/{png}: unexpected change'
            subprocess.run(base + [str(out / ('no-lines-' + png)), 'no-lines'], check=True, stdout=subprocess.DEVNULL)
            frames[png] = dict(changed=predicted != old_sha, matches_offline_prediction=True,
                               line_scroll_changes_pixels=predicted != sha((out / ('no-lines-' + png)).read_bytes()))
        result[name] = dict(guest_trace_identical=True, frames=frames,
                            pcm_changed=before[name]['wav_sha256'] != after[name]['wav_sha256'])
    (a.output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    total = sum(len(v['frames']) for v in result.values())
    print(f'PASS: {total} fields match offline renderer predictions; all {len(names)} guest traces identical.')
