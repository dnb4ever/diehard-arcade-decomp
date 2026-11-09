#!/usr/bin/env python3
"""Compare the current release against every P4.13–P4.16 offline oracle."""
import argparse
import hashlib
import json
import os
import re
import subprocess
import time
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary', default='work/p417/build-linux/saturn_stv')
p.add_argument('--tag', default='linux')
a = p.parse_args()
os.chdir(Path(__file__).resolve().parent.parent)
out = Path('work/p418/regression') / a.tag
out.mkdir(parents=True, exist_ok=True)
reference = json.loads(Path('work/p411/regression/after/manifest.json').read_text())
history = {n: json.loads((Path('work') / n / 'regression/manifest.json').read_text())
           for n in ['p413', 'p414', 'p415', 'p416']}
sha = lambda b: hashlib.sha256(b).hexdigest()
report = {}
for name, ref in reference.items():
    d = out / name
    d.mkdir(exist_ok=True)
    cmd = ref['command'].copy()
    cmd[0] = a.binary
    cmd[cmd.index('--out') + 1] = str(d)
    cmd[cmd.index('--wav') + 1] = str(d / 'audio.wav')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('STV_', 'SATURN_'))}
    env.update(ref['env'])
    env['APPIMAGE_EXTRACT_AND_RUN'] = '1'
    start = time.monotonic()
    with (d / 'run.log').open('w') as log:
        r = subprocess.run(cmd, env=env, stdout=log, stderr=log, timeout=1200)
    text = (d / 'run.log').read_text()
    assert r.returncode == 0 and 'FATAL' not in text, name
    trace = '\n'.join(line for line in text.splitlines() if re.search(
        r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples', line)) + '\n'
    guest = '\n'.join(line for line in text.splitlines() if re.search(
        r'SCENETRACE|ST-V coin:|FATAL|stopped after', line)) + '\n'
    actual = dict(png={n: sha((d / n).read_bytes()) for n in ref['png']},
                  trace_sha256=sha(trace.encode()), guest_trace_sha256=sha(guest.encode()),
                  wav_sha256=sha((d / 'audio.wav').read_bytes()))
    for milestone, manifest in {'p411': reference, **history}.items():
        for key, value in actual.items():
            assert value == manifest[name][key], (milestone, name, key)
    report[name] = dict(command=cmd, seconds=time.monotonic() - start, **actual)
    (out / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    print(name, 'PASS', len(actual['png']), 'PNGs, full/guest traces, WAV; P4.13–P4.16 identical', flush=True)
