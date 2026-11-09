#!/usr/bin/env python3
"""Verify mod-aware P4.19 connection codes using only file-backed test transport."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', default='port/run/P46/build-sdl/saturn_stv')
parser.add_argument('--frames', type=int, default=1200)
args = parser.parse_args()
os.chdir(ROOT)
work = ROOT / 'work/p422/codes'
work.mkdir(parents=True, exist_ok=True)
mods = ROOT / 'work/p422/data/mods'
shim = work / 'datagram.so'
subprocess.run(['cc', '-shared', '-fPIC', '-O2', 'port/p419_audio_datagram.c',
                '-ldl', '-o', str(shim)], check=True)
env = {k: v for k, v in os.environ.items()
       if not k.startswith(('STV_', 'SATURN_', 'P416_'))}
env.update(LD_PRELOAD=str(shim), P419_EPOCH='1794236400',
           STV_NETPLAY_LAN='127.0.0.1', STV_NETPLAY_DUMP='1')
identity = Path(args.binary).parent / 'p410_identity.h'
base = int(re.search(r'STV_NETPLAY_CODE 0x([0-9a-f]+)', identity.read_text())[1], 16)


def expected_build(folder, selection):
    records = []
    for name in sorted(selection.split(',')) if selection != 'none' else []:
        manifest = (folder / name / 'manifest.ini').read_bytes()
        fields = dict(line.split('=', 1) for line in manifest.decode().splitlines() if '=' in line)
        native = (folder / name / fields['library']).read_bytes()
        digest = hashlib.sha256(manifest + b'\0' + native).hexdigest()
        records.append(f"{fields['id']}@{fields['version']}:{digest}\n")
    code = base
    if records:
        for byte in hashlib.sha256(''.join(records).encode()).hexdigest().encode():
            code = ((code ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return code & 0xffffffff


changed = work / 'changed-mods'
if changed.exists():
    shutil.rmtree(changed)
shutil.copytree(mods, changed)
with (changed / 'frame-hud/mod.so').open('ab') as output:
    output.write(b'p422 connection-code hash mutation')
report = {}
cases = [
    ('matching', ('frame-hud,infinite-health', 'infinite-health,frame-hud'), False),
    ('different-sets', ('none', 'identity'), False),
    ('different-native-bytes', ('frame-hud', 'frame-hud'), True),
]
for name, selections, mutate in cases:
    directory = work / name
    directory.mkdir(exist_ok=True)
    wire = directory / 'datagrams'
    if wire.exists():
        shutil.rmtree(wire)
    wire.mkdir()
    for who in ['a', 'b']:
        (directory / (who + '.code')).unlink(missing_ok=True)
    processes, builds = [], []
    mismatch = name != 'matching'
    try:
        for index, (who, peer, port) in enumerate([('a', 'b', 22421), ('b', 'a', 22422)]):
            output = directory / who
            output.mkdir(exist_ok=True)
            folder = changed if mutate and index else mods
            builds.append(expected_build(folder, selections[index]))
            command = [args.binary, '--cart', 'port/diehard_main.bin', '--cartrom', 'cart.bin',
                       '--mods-dir', str(folder), '--mods', selections[index],
                       '--netplay-code', str(port), '--netplay-code-out', str(directory / (who + '.code')),
                       '--netplay-peer-code', '@' + str(directory / (peer + '.code')),
                       '--netplay-no-map', '--netplay-no-stun', '--input-delay', '0',
                       '--vblanks', str(2 if mismatch else args.frames),
                       '--stvinput', '100:COIN,130:START1,101:COIN2,131:START2,500:P1A:80,530:P2B:90',
                       '--shots', str(args.frames), '--wav', str(output / 'audio.wav'), '--out', str(output)]
            log = (output / 'run.log').open('w')
            childenv = dict(env, P416_DATAGRAM_DIR=str(wire),
                            P416_LATENCY_MS='0' if mismatch else '30',
                            P416_LOSS_BP='0' if mismatch else '100',
                            P416_JITTER_MS='0' if mismatch else '10')
            processes.append((subprocess.Popen(command, env=childenv, stdout=log, stderr=log), log, output))
        codes = [child.wait(timeout=180) for child, _, _ in processes]
    finally:
        for child, log, _ in processes:
            if child.poll() is None:
                child.terminate()
                child.wait(timeout=10)
            log.close()
    logs = [(output / 'run.log').read_text() for _, _, output in processes]
    for index, who in enumerate(['a', 'b']):
        code = (directory / (who + '.code')).read_text().strip()
        encoded = code.removeprefix('DHA1-')
        packet = base64.urlsafe_b64decode(encoded + '=' * (-len(encoded) % 4))
        assert int.from_bytes(packet[4:8], 'big') == builds[index], (name, code, builds[index])
    result = {'returncodes': codes, 'code_builds': [f'{b:08x}' for b in builds]}
    if mismatch:
        assert codes == [2, 2], (name, codes, logs)
        assert builds[0] != builds[1]
        assert all('mod set/hash mismatch' in log and 'origin ready' not in log for log in logs), logs
        result['mismatch_rejected_before_handshake'] = True
    else:
        assert codes == [0, 0] and builds[0] == builds[1], (name, codes, logs)
        assert all('desync/failure=0' in log and 'netplay: code path ' in log for log in logs)
        checks = [dict(re.findall(r'confirmed field=(\d+) CRC=([0-9a-f]+)', log)) for log in logs]
        assert checks[0] == checks[1] and len(checks[0]) > 10
        result['confirmed_checksums'] = len(checks[0])
        result['artifacts'] = {}
        for artifact in [f'shot-{args.frames}.png', 'audio.wav', 'final-machine.bin']:
            hashes = [hashlib.sha256((output / artifact).read_bytes()).hexdigest()
                      for _, _, output in processes]
            assert hashes[0] == hashes[1], (artifact, hashes)
            result['artifacts'][artifact] = hashes[0]
    report[name] = result
    print(name, 'PASS', result, flush=True)
(work / 'report.json').write_text(json.dumps({'transport': 'test-only file-backed datagrams',
                                           'cases': report}, indent=2) + '\n')
