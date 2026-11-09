#!/usr/bin/env python3
"""Verify P4.9 settings via real MAME EEPROM deltas and headless guest effects.

Requires the MAME menu runs recorded in work/p49/mame. Reads its existing
scenario stream; it never writes guest state or modifies the scenario tools.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import threading

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', default='port/run/P49/build-headless/saturn_stv')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
os.chdir(root)
out = Path('work/p49/settings-effects')
out.mkdir(parents=True, exist_ok=True)
factory = Path('work/p49/mame/assign/nvram/diehard/eeprom').read_bytes()
variants = {
    'factory': ('factory-play', '', 0x1b, False),
    'difficulty': ('difficulty-save', 'difficulty = 5\n', 0x1c, False),
    'vitality': ('vitality-save', 'vitality = 4\n', 0x23, False),
    'lives': ('lives-save', 'lives = 2\n', 0x5b, False),
    'free': ('free-save', 'free_play = true\n', 0x1b, True),
}
report = {}

def extract(ram):
    def u(a, n=4):
        i = a - 0x06000000
        return int.from_bytes(ram[i:i+n], 'big')
    actors = []
    for slot in range(1, 7):
        task = u(0x060995b0 + slot * 4)
        ent = u(task + 8) if 0x06000000 <= task < 0x060fff00 else 0
        if 0x06000000 <= ent < 0x060ffe00:
            actors.append(dict(slot=slot, hp=u(ent+0x56,2), difficulty=u(ent+0xe8),
                               x=u(ent+8), z=u(ent+16)))
    return dict(scene=u(0x06098668), playing=u(0x06098674,1),
                packed=u(0x060987ee,1), max_vitality=u(0x06099794,2),
                initial_lives=u(0x06099796,2), lives=u(0x0609978c,2), actors=actors)

for name, (mame, settings, packed, free) in variants.items():
    dest = out / name
    dest.mkdir(exist_ok=True)
    ini = dest / 'stv.ini'
    ini.write_text('[stv]\n' + settings)
    eeprom = Path('work/p49/mame', mame, 'nvram/diehard/eeprom').read_bytes()
    assert eeprom[0x21] == packed and eeprom[0x5d] == packed, name
    if free:
        assert eeprom[0x14:0x16] == bytes([1,25]) and eeprom[0x50:0x52] == bytes([1,25])
    fifo = dest / 'capture.fifo'
    fifo.unlink(missing_ok=True)
    os.mkfifo(fifo)
    records = {}
    errors = []
    def capture():
        try:
            with fifo.open('rb') as stream:
                def read(n):
                    b = stream.read(n)
                    if len(b) != n:
                        raise EOFError('short scenario record')
                    return b
                while header := stream.read(4):
                    rec = json.loads(read(struct.unpack('<I', header)[0]))
                    ram = read(rec['ram_bytes'])
                    read(rec['pixel_bytes'])
                    if rec['frame'] in (220, 1050, 1150, 1800):
                        high = ram[0x100000:]
                        (dest / f'ram-{rec["frame"]}.bin').write_bytes(high)
                        records[rec['frame']] = extract(high)
        except Exception as e:
            errors.append(str(e))
    reader = threading.Thread(target=capture, daemon=True)
    reader.start()
    env = {k:v for k,v in os.environ.items() if not k.startswith('STV_')}
    env.update(STV_SCENARIO_PIPE=str(fifo), STV_SCENARIO_MEMORY='1', STV_SCENARIO_VIDEO='0')
    timeline = '200:START1' if free else '100:COIN,200:START1'
    cmd = [args.binary, '--cart', 'port/diehard_main.bin', '--cartrom', 'cart.bin',
           '--settings', str(ini), '--vblanks', '1800', '--stvinput', timeline,
           '--shots', '1050,1150,1800', '--out', str(dest)]
    with (dest / 'run.log').open('w') as log:
        run = subprocess.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=180)
    reader.join(5)
    fifo.unlink()
    assert not errors and not reader.is_alive() and run.returncode == 0, (name, errors)
    assert records[1150]['packed'] == packed, (name, records)
    assert records[1150]['playing'] == 2, (name, records)
    assert records[1150]['max_vitality'] == (102 if name == 'vitality' else 114)
    assert records[1150]['initial_lives'] == (2 if name == 'lives' else 1)
    mame_ram = Path('work/p49/mame', mame, 'ram-1650.bin').read_bytes()
    mame_state = extract(mame_ram)
    assert mame_state['packed'] == packed and mame_state['playing'] == 2
    assert mame_state['max_vitality'] == records[1150]['max_vitality']
    assert mame_state['initial_lives'] == records[1150]['initial_lives']
    report[name] = dict(command=cmd, eeprom_sha256=hashlib.sha256(eeprom).hexdigest(),
                        eeprom_diff=[dict(offset=i, before=a, after=b) for i,(a,b) in enumerate(zip(factory,eeprom)) if a != b],
                        mame=mame_state, port=records)
    print(name, json.dumps(records[1150]), flush=True)
# Enemy action-level fields change when the guest consumes the new difficulty.
assert report['factory']['port'][1150]['actors'] != report['difficulty']['port'][1150]['actors']
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
