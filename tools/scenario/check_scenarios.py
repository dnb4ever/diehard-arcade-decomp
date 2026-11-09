#!/usr/bin/env python3
"""Die Hard Arcade port/MAME frame checker v1.01 (independent implementation)."""
from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import hashlib
import json
import math
import os
from pathlib import Path
import re
import select
import struct
import subprocess
import sys
import threading
import time

VERSION = '1.01'
ROOT = Path(__file__).resolve().parents[2]
HERE = Path('tools/scenario')
MIB = 1 << 20
PANEL = {1:'A', 2:'B', 4:'C', 0x40:'UP', 0x80:'DOWN', 0x100:'LEFT', 0x200:'RIGHT'}
SYSTEM = {1:'COIN', 2:'COIN2', 4:'TEST', 8:'SERVICE', 16:'START1', 32:'START2'}
FIELDS = ('scene', 'join1', 'join2', 'arm', 'mode', 'progress', 'cw', 'rng', 'counter')
P41 = re.compile(r'P41 VB=(\d+) scene=(\d+) mode=(\d+) arm=(\d+) progress=(\d+) join=(\d+)/(\d+) cw=([0-9A-Fa-f]+)')
SLOT = re.compile(r'P41S VB=(\d+) slot=(\d+) task=([0-9A-Fa-f]+) ent=([0-9A-Fa-f]+) id=(\d+) hp=(\d+) xyz=([0-9A-Fa-f]+)/([0-9A-Fa-f]+)/([0-9A-Fa-f]+)')
STORE = re.compile(r'store(8|16|32)\s+([0-9A-Fa-f]+) = ([0-9A-Fa-f]+).*\bVB (\d+)\)')

@dataclasses.dataclass
class Scenario:
    name: str
    frames: int
    rows: list[tuple[int, int, int]]
    expect: list[str]


def read_scenario(path):
    frames = None
    spans, expected = [], []
    for line_no, line in enumerate(Path(path).read_text().splitlines(), 1):
        words = line.split('#')[0].split()
        if not words:
            continue
        try:
            if words[0] == 'frames' and len(words) == 2 and frames is None:
                frames = int(words[1])
            elif words[0] == 'expect' and len(words) == 2:
                expected.append(words[1])
            elif len(words) == 5:
                lo, hi = map(int, words[:2])
                p1, p2, sysbits = (int(w, 16) for w in words[2:])
                if lo < 1 or hi < lo or p1 & ~0x3c7 or p2 & ~0x3c7 or sysbits & ~0x3f:
                    raise ValueError('invalid range or panel mask')
                spans.append((lo, hi, p1, p2, sysbits))
            else:
                raise ValueError('expected frames N, expect CONDITION, or FIRST LAST P1 P2 SYSTEM')
        except ValueError as e:
            raise ValueError(f'{path}:{line_no}: {e}') from e
    if frames is None or not 1 <= frames <= 200000:
        raise ValueError(f'{path}: frames must be in 1..200000')
    rows = [(0, 0, 0)] * (frames + 1)
    for lo, hi, *masks in spans:
        if hi > frames:
            raise ValueError(f'{path}: range extends past frames')
        for f in range(lo, hi+1):
            rows[f] = tuple(a | b for a, b in zip(rows[f], masks))
    allowed = {'paid', 'game_over', 'attract_return'}
    if any(e not in allowed and not re.fullmatch(r'scene=\d+', e) for e in expected):
        raise ValueError(f'{path}: unknown expectation')
    return Scenario(Path(path).stem, frames, rows, expected)


def port_input(scenario, allow_test=False):
    if not allow_test and any(row[2] & 4 for row in scenario.rows):
        raise ValueError('Existing --stvinput ignores TEST; use the proposed TEST patch before testing it')
    events = []
    for col, mapping in enumerate((PANEL, PANEL, SYSTEM)):
        for bit, token in mapping.items():
            start = None
            for f in range(1, scenario.frames+2):
                held = f <= scenario.frames and bool(scenario.rows[f][col] & bit)
                if held and start is None:
                    start = f
                elif not held and start is not None:
                    name = f'P{col+1}{token}' if col < 2 else token
                    events.append((start, name, f-start))
                    start = None
    # An empty string would activate the port's built-in coin/start timeline.
    return ','.join(f'{f}:{token}:{duration}' for f, token, duration in sorted(events)) or f'{scenario.frames+1000}:COIN:1'


def clean_env():
    env = {k:v for k,v in os.environ.items() if not k.startswith(('STV_', 'SCENARIO_'))}
    return env


def sha(data):
    return hashlib.sha256(data).hexdigest()


def ram_hashes(low, high, mask):
    raw = low + high
    masked = bytearray(raw)
    for r in mask['ranges']:
        lo, hi = int(r['start'], 0), int(r['end'], 0)
        for base, dest in ((0x00200000, 0), (0x06000000, MIB)):
            first, last = max(lo, base), min(hi, base+MIB)
            if first < last:
                a, b = dest+first-base, dest+last-base
                masked[a:b] = bytes(b-a)
    regions = {}
    for base, dest in ((0x00200000, 0), (0x06000000, MIB)):
        for off in range(0, MIB, 0x10000):
            regions[f'{base+off:08X}-{base+off+0xffff:08X}'] = sha(masked[dest+off:dest+off+0x10000])
    return {'raw':sha(raw), 'masked':sha(masked), 'regions':regions}


def image_record(img):
    img = img.convert('RGB')
    return {'width':img.width, 'height':img.height,
            'sha256':sha(struct.pack('<II', img.width, img.height)+img.tobytes())}


def normalize_record(record):
    """Keep pointers out of semantic comparison; actor slot identity is stable."""
    record.setdefault('ram', None)
    record.setdefault('screen', None)
    record.setdefault('rng', None)
    record.setdefault('counter', None)
    semantic = {k:record.get(k) for k in FIELDS}
    semantic['actors'] = record['actors']
    record['state_sha256'] = sha(json.dumps(semantic, sort_keys=True, separators=(',', ':')).encode())
    return record


def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True)+'\n')


def run_process(cmd, env, log, timeout, transport_errors=None):
    with Path(log).open('wb') as stream:
        proc = subprocess.Popen(cmd, env=env, stdout=stream, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + timeout
            while True:
                if transport_errors:
                    raise RuntimeError(f'capture reader failed: {transport_errors[0]}')
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise subprocess.TimeoutExpired(cmd, timeout)
                try:
                    code = proc.wait(timeout=min(1, remaining))
                    break
                except subprocess.TimeoutExpired:
                    pass
        except BaseException:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            raise
    if code:
        raise RuntimeError(f'exit {code}: {cmd[0]}; see {log}')


def capture_port(args, scenario, out, video=True):
    if args.port_hook:
        mask = json.loads((HERE/'ram_mask.json').read_text())
        return capture_binary(args, scenario, out, 0, mask, memory=video, video=video, engine='port')
    out.mkdir(parents=True, exist_ok=True)
    env = clean_env()
    env['STV_P41TRACE'] = '1'
    env['STV_WATCH_CPU'] = '0'
    cmd = [args.port, '--cart', args.cart, '--cartrom', args.cartrom, '--vblanks', str(scenario.frames),
           '--stvinput', port_input(scenario), '--watch', '060987BC:0609957F', '--out', str(out)]
    if video:
        shots = ','.join(map(str, range(1, scenario.frames+1)))
        if len(shots) > 100000:
            raise ValueError('per-frame --shots argument too long; reduce frames or use --port-hook')
        cmd += ['--shots', shots]
    write_json(out/'command.json', {'argv':cmd, 'env':{'STV_P41TRACE':'1','STV_WATCH_CPU':'0'}, 'version':VERSION})
    run_process(cmd, env, out/'run.log', args.timeout)
    records = []
    # The store watch prints BEFORE the store; the P41 line samples after those stores.
    # Track bytes, not just whole-long writes. A known reset write establishes validity.
    tracked = {}
    current = None
    slots = []
    def finish():
        if current is None:
            return
        if len(slots) != 6:
            raise RuntimeError(f'incomplete P41 frame {current["frame"]} in {out}')
        current['actors'] = list(slots)
        f = current['frame']
        png = out/f'shot-{f}.png'
        if video:
            if not png.exists():
                raise RuntimeError(f'missing screen capture: {png}')
            from PIL import Image
            with Image.open(png) as img:
                current['screen'] = image_record(img)
            if f not in args.png:
                png.unlink()
        records.append(normalize_record(current))
    with (out/'run.log').open() as stream:
        for line in stream:
            m = STORE.search(line)
            if m:
                size, address, value = int(m[1])//8, int(m[2],16), int(m[3],16)
                for i, byte in enumerate(value.to_bytes(size, 'big')):
                    tracked[address+i] = byte
                continue
            m = P41.search(line)
            if m:
                finish()
                f, scene, mode, arm, progress, j1, j2 = map(int, m.groups()[:7])
                current = dict(frame=f, scene=scene, mode=mode, arm=arm, progress=progress,
                               join1=j1, join2=j2, cw=int(m[8],16), version=VERSION, source='port-p41')
                for key, address in (('rng',0x0609957c), ('counter',0x060987bc)):
                    current[key] = int.from_bytes(bytes(tracked[address+i] for i in range(4)), 'big') if all(address+i in tracked for i in range(4)) else None
                slots = []
                continue
            m = SLOT.search(line)
            if m and current is not None:
                f, slot = int(m[1]), int(m[2])
                if f != current['frame']:
                    raise RuntimeError(f'mismatched P41S frame in {out}')
                entity = int(m[4],16)
                slots.append(dict(slot=slot, active=bool(entity), id=int(m[5]), hp=int(m[6]),
                                  x=int(m[7],16), y=int(m[8],16), z=int(m[9],16)))
    finish()
    save_records(out/'frames.jsonl', records)
    validate_frames(records, scenario.frames)
    return records


def save_records(path, records):
    with Path(path).open('w') as stream:
        for record in records:
            stream.write(json.dumps(record, sort_keys=True, separators=(',', ':'))+'\n')


def validate_frames(records, expected):
    if [r['frame'] for r in records] != list(range(1, expected+1)):
        raise RuntimeError(f'frame capture incomplete: expected 1..{expected}, got {len(records)} records')


def capture_binary(args, scenario, out, offset, mask, memory=True, video=True, engine="mame"):
    """Stream raw data over a FIFO to hash in C-backed Python, avoiding Lua byte loops."""
    out.mkdir(parents=True, exist_ok=True)
    fifo = out/'capture.fifo'
    if fifo.exists():
        fifo.unlink()
    os.mkfifo(fifo)
    # A temporary RDWR descriptor makes reader startup independent of MAME startup.
    fd = os.open(fifo, os.O_RDWR | os.O_NONBLOCK)
    finished = threading.Event()
    errors, records = [], []
    total = scenario.frames + offset
    input_path = out/'input.txt'
    with input_path.open('w') as stream:
        for f, row in enumerate(scenario.rows[1:], 1):
            if any(row):
                stream.write(f'{f} {row[0]:x} {row[1]:x} {row[2]:x}\n')
    def receive():
        pending = bytearray()
        def read_exact(n):
            while len(pending) < n:
                if select.select([fd], [], [], 0.2)[0]:
                    chunk = os.read(fd, min(1<<20, n-len(pending)))
                    if chunk:
                        pending.extend(chunk)
                        continue
                if finished.is_set():
                    raise EOFError(f'{engine} transport ended after {len(records)}/{total} frames')
            data = bytes(pending[:n])
            del pending[:n]
            return data
        try:
            with (out/'frames.jsonl').open('w') as stream:
                for _ in range(total):
                    length, = struct.unpack('<I', read_exact(4))
                    if not 1 <= length < 65536:
                        raise RuntimeError(f'invalid capture header length {length}')
                    record = json.loads(read_exact(length))
                    if record.get('version') != VERSION or record['frame'] != len(records)+1:
                        raise RuntimeError('capture version or frame sequence mismatch')
                    if len(record['actors']) != 6 or [a['slot'] for a in record['actors']] != list(range(1,7)):
                        raise RuntimeError('capture actor slots incomplete')
                    w, h = record['width'], record['height']
                    if not (0 <= w <= 2048 and 0 <= h <= 2048) or record['pixel_bytes'] != w*h*4:
                        raise RuntimeError('invalid pixel payload size')
                    record['source'] = engine
                    if record['ram_bytes']:
                        if record['ram_bytes'] != 2*MIB:
                            raise RuntimeError('unexpected RAM size')
                        low, high = read_exact(MIB), read_exact(MIB)
                        record['ram'] = ram_hashes(low, high, mask)
                        if record['frame']-offset in args.ram_frames:
                            (out/f'ram-{record["frame"]}.bin').write_bytes(low+high)
                    pixels = read_exact(record['pixel_bytes'])
                    if pixels:
                        from PIL import Image
                        if sys.byteorder != 'little':
                            raise RuntimeError('MAME pixel transport currently requires a little-endian host')
                        img = Image.frombytes('RGB', (record['width'],record['height']), pixels, 'raw', 'BGRX')
                        record['screen'] = image_record(img)
                        if record['frame']-offset in args.png:
                            img.save(out/f'shot-{record["frame"]}.png')
                    normalize_record(record)
                    records.append(record)
                    stream.write(json.dumps(record, sort_keys=True, separators=(',', ':'))+'\n')
        except BaseException as e:
            errors.append(e)
    reader = threading.Thread(target=receive, daemon=True)
    reader.start()
    env = clean_env()
    env.update(SCENARIO_PIPE=str(fifo), SCENARIO_FRAMES=str(total), SCENARIO_OFFSET=str(offset),
               SCENARIO_INPUT=str(input_path), SCENARIO_MEMORY='1' if memory else '0',
               SCENARIO_VIDEO='1' if video else '0')
    if args.mame_lib:
        env['LD_LIBRARY_PATH'] = args.mame_lib + (':'+env['LD_LIBRARY_PATH'] if env.get('LD_LIBRARY_PATH') else '')
    cmd = [args.mame, 'diehard', '-bios', 'us', '-rompath', args.rompath, '-video', 'none', '-sound', 'none',
           '-nothrottle', '-skip_gameinfo', '-noreadconfig', '-autoboot_delay', '0',
           '-nvram_directory', str(out/'nvram'), '-cfg_directory', str(out/'cfg'),
           '-snapshot_directory', str(out), '-seconds_to_run', str(math.ceil(total/59)+30),
           '-autoboot_script', str(HERE/'mame_capture.lua')]
    if engine == 'port':
        env.update(STV_SCENARIO_PIPE=str(fifo), STV_SCENARIO_MEMORY='1' if memory else '0',
                   STV_SCENARIO_VIDEO='1' if video else '0')
        cmd = [args.port, '--cart', args.cart, '--cartrom', args.cartrom,
               '--vblanks', str(total), '--stvinput', port_input(scenario, allow_test=True), '--out', str(out)]
    write_json(out/'command.json', {'argv':cmd, 'env':{k:v for k,v in env.items() if k.startswith(('SCENARIO_', 'STV_SCENARIO_')) or k=='LD_LIBRARY_PATH'}, 'version':VERSION})
    try:
        run_process(cmd, env, out/'run.log', args.timeout, transport_errors=errors)
    finally:
        finished.set()
        reader.join(timeout=10)
        os.close(fd)
        fifo.unlink(missing_ok=True)
    if reader.is_alive():
        raise RuntimeError('MAME transport did not finish')
    if errors:
        raise RuntimeError(str(errors[0]))
    validate_frames(records, total)
    return records


def transitions(records):
    previous = None
    result = []
    for row in records:
        if row['scene'] != previous:
            result.append((row['frame'], row['scene']))
            previous = row['scene']
    return result


def boot_offset(port, mame):
    # Use the first 1 -> 2 scene edge after boot. Never optimize against later gameplay.
    def edge(records):
        trans = transitions(records)
        for (a, old), (b, new) in zip(trans, trans[1:]):
            if old == 1 and new == 2 and b-a > 30:
                return b
        raise RuntimeError('no stable attract 1 -> 2 edge; cannot align boot')
    p, m = edge(port), edge(mame)
    offset = m-p
    if not 0 <= offset <= 2000:
        raise RuntimeError(f'implausible boot offset {offset}')
    return {'offset':offset, 'port_anchor':p, 'mame_anchor':m, 'anchor':'first attract scene 1 -> 2'}


def differences(p, m):
    result = []
    for key in FIELDS:
        if p.get(key) is not None and m.get(key) is not None and p[key] != m[key]:
            result.append((key,p[key],m[key]))
    for pa, ma in zip(p['actors'], m['actors']):
        for key in ('active','id','hp','x','y','z'):
            if pa[key] != ma[key]:
                result.append((f'actor[{pa["slot"]}].{key}',pa[key],ma[key]))
    return result


def first_item(frame, mframe, diff, classification):
    return {'frame':frame, 'mame_frame':mframe, 'field':diff[0], 'port':diff[1], 'mame':diff[2], 'classification':classification}


def expectations(records, expected):
    scenes = {r['scene'] for r in records}
    joins = [r['join1'] for r in records]
    trans = transitions(records)
    result = {}
    for e in expected:
        if e.startswith('scene='):
            ok = int(e.split('=')[1]) in scenes
        elif e == 'paid':
            ok = 2 in joins
        elif e == 'game_over':
            # Credit-state 3 (continue countdown) -> 0; game-over display follows.
            ok = any(a==3 and b==0 for a,b in zip(joins, joins[1:]))
        else:
            seen = False
            ok = False
            for _, scene in trans:
                if 3 <= scene <= 81:
                    seen = True
                if scene == 1 and seen:
                    ok = True
        result[e] = ok
    return result


def compare(scenario, port, mame, alignment):
    offset = alignment['offset']
    mi = {r['frame']:r for r in mame}
    result = dict(name=scenario.name, frames=scenario.frames, offset=offset,
                  first_field=None, first_gameplay=None, first_ram=None, first_screen=None, first_rng_after_pattern=None, first_by_field={},
                  matched_fields=0, compared_frames=0, matched_screens=0, compared_screens=0, compared_ram=0, matched_ram=0, missing_port_fields=[],
                  port_expect=expectations(port, scenario.expect), mame_expect=expectations(mame, scenario.expect),
                  port_transitions=transitions(port), mame_transitions=transitions(mame))
    for key in ('rng','counter','ram','screen'):
        if any(r.get(key) is None for r in port):
            result['missing_port_fields'].append(key)
    # Port-only initialization before scene 1 is outside the boot-aligned comparison.
    start = next((r['frame'] for r in port if r['scene']==1), None)
    if start is None:
        raise RuntimeError('port never entered attract scene 1')
    result['compare_start'] = start
    for p in port:
        f = p['frame']
        if f < start:
            continue
        if f+offset not in mi:
            raise RuntimeError(f'missing aligned MAME frame {f+offset}')
        m = mi[f+offset]
        result['compared_frames'] += 1
        diffs = differences(p,m)
        if (result['first_rng_after_pattern'] is None and p.get('rng') is not None and m.get('rng') is not None
                and p['rng'] != m['rng'] and m['rng'] != 0x53454741):
            result['first_rng_after_pattern'] = first_item(f,m['frame'],('rng',p['rng'],m['rng']),'RNG/timer seed candidate')
        if not diffs:
            result['matched_fields'] += 1
        for key,pv,mv in diffs:
            classification = 'game-state candidate'
            if key == 'counter':
                classification = 'timing/counter'
            elif key in ('mode','progress','rng') and mv in (0x53454741, 0x4741):
                classification = 'boot memory initialization'
            elif key == 'scene' and f == start:
                classification = 'boot boundary timing'
            elif key == 'rng':
                classification = 'RNG/timer seed candidate'
            elif key == 'cw':
                classification = 'input sample timing candidate'
            item = first_item(f, m['frame'], (key,pv,mv), classification)
            result['first_by_field'].setdefault(key, item)
            if result['first_field'] is None:
                result['first_field'] = item
            if key != 'counter' and result['first_gameplay'] is None:
                result['first_gameplay'] = item
        if p.get('ram') and m.get('ram'):
            result['compared_ram'] += 1
            if p['ram']['masked'] == m['ram']['masked']:
                result['matched_ram'] += 1
        if p.get('ram') and m.get('ram') and result['first_ram'] is None:
            for region, ph in p['ram']['regions'].items():
                mh = m['ram']['regions'][region]
                if ph != mh:
                    result['first_ram'] = first_item(f,m['frame'],(region,ph,mh),'masked RAM diagnostic')
                    break
        if p.get('screen') and m.get('screen'):
            result['compared_screens'] += 1
            if p['screen'] == m['screen']:
                result['matched_screens'] += 1
        if p.get('screen') and m.get('screen') and p['screen'] != m['screen'] and result['first_screen'] is None:
            result['first_screen'] = first_item(f,m['frame'],('screen',p['screen'],m['screen']),'render/timing diagnostic')
    result['status'] = ('DIVERGED' if any(result[k] for k in ('first_field','first_ram','first_screen')) else 'INCOMPLETE' if result['missing_port_fields'] else 'MATCH')
    if not all(result['port_expect'].values()) or not all(result['mame_expect'].values()):
        result['coverage'] = 'UNMET'
    else:
        result['coverage'] = 'PASS'
    return result


def load_records(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines() if line.strip()]


def summary(results):
    lines = ['| Scenario | Frames | Offset | First divergence (port/MAME frame) | Values (port / MAME) | Coverage | Unavailable port | Status |',
             '|---|---:|---:|---|---|---|---|---|']
    for r in results:
        d = r['first_field'] or r['first_ram'] or r['first_screen']
        where = f'{d["field"]} @ {d["frame"]}/{d["mame_frame"]}' if d else 'none'
        values = f'{d["port"]} / {d["mame"]}' if d else '—'
        missing = ', '.join(r['missing_port_fields']) or 'none'
        lines.append(f'| {r["name"]} | {r["frames"]} | {r["offset"]} | {where} | {values} | {r["coverage"]} | {missing} | {r["status"]} |')
    return '\n'.join(lines)+'\n'


def csv_ints(value):
    return set(map(int, value.split(','))) if value else set()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('scenarios', nargs='*', help='scenario .txt files (default: tests/scenarios/*.txt)')
    ap.add_argument('--version', action='version', version=VERSION)
    ap.add_argument('--port', default='port/run/P41/build-generated/saturn_stv')
    ap.add_argument('--cart', default='port/diehard_main.bin')
    ap.add_argument('--cartrom', default='cart.bin')
    ap.add_argument('--mame', default='port/run/P45/mamepkg/x/usr/games/mame')
    ap.add_argument('--mame-lib', default='port/run/P45/mamepkg/x/usr/lib/x86_64-linux-gnu')
    ap.add_argument('--rompath', default='port/run/P45/mame/roms')
    ap.add_argument('--out', default='work/scenario_v101/results')
    ap.add_argument('--timeout', type=float, default=1800, help='timeout per child process in seconds')
    ap.add_argument('--png', type=csv_ints, default=set(), help='retain PNG at these scenario frames, comma separated')
    ap.add_argument('--ram-frames', type=csv_ints, default=set(), help='retain raw MAME RAM at these scenario frames')
    ap.add_argument('--port-hook', action='store_true', help='use the proposed binary stream hook (requires an independently patched port)')
    ap.add_argument('--jobs', type=int, choices=range(1,5), default=1, help='independent scenario processes in parallel (1..4)')
    ap.add_argument('--reuse', action='store_true', help='compare existing captures; no emulators launched')
    ap.add_argument('--fields-only', action='store_true', help='skip image and MAME RAM capture (quick diagnostic)')
    ap.add_argument('--offset', type=int, help='explicit override; otherwise calibrate boot before replay')
    args = ap.parse_args()
    os.chdir(ROOT)
    # All persisted paths and outputs stay repository-relative, including command manifests.
    for key in ('port','cart','cartrom','mame','mame_lib','rompath','out'):
        path = Path(getattr(args,key))
        if path.is_absolute() or '..' in path.parts:
            ap.error(f'--{key.replace("_","-")} must be repository-relative')
    if not (args.out.startswith('work/scenario') and Path(args.out).parts[0]=='work'):
        ap.error('--out must be under work/scenario*')
    paths = args.scenarios or sorted(str(p) for p in Path('tests/scenarios').glob('*.txt'))
    if any(Path(path).is_absolute() or '..' in Path(path).parts for path in paths):
        ap.error('scenario paths must be repository-relative')
    scenarios = [read_scenario(path) for path in paths]
    if not scenarios:
        ap.error('no scenarios')
    out = Path(args.out)
    if out.exists() and any(out.iterdir()) and not args.reuse:
        ap.error('output already contains files; choose a fresh --out or use --reuse')
    out.mkdir(parents=True, exist_ok=True)
    mask = json.loads((HERE/'ram_mask.json').read_text())
    if len({s.name for s in scenarios}) != len(scenarios):
        ap.error('scenario file stems must be unique')
    if args.reuse:
        saved_manifest = json.loads((out/'manifest.json').read_text())
        if saved_manifest['mask'] != mask:
            ap.error('RAM mask changed since capture; use the captured mask or recapture')
        for path in paths:
            if saved_manifest['inputs'].get(str(path)) != sha(Path(path).read_bytes()):
                ap.error(f'scenario changed since capture: {path}')
        alignment = json.loads((out/'alignment.json').read_text())
    elif args.offset is not None:
        if not 0 <= args.offset <= 2000:
            ap.error('--offset must be in 0..2000')
        alignment = {'offset':args.offset,'anchor':'manual override'}
        write_json(out/'alignment.json', alignment)
    else:
        print('Calibrating first attract scene 1 -> 2 edge...', flush=True)
        calibration = Scenario('calibration',500,[(0,0,0)]*501,[])
        p = capture_port(args,calibration,out/'calibration-port',video=False)
        mcal = Scenario('calibration',1500,[(0,0,0)]*1501,[])
        m = capture_binary(args,mcal,out/'calibration-mame',0,mask,memory=False,video=False)
        alignment = boot_offset(p,m)
        write_json(out/'alignment.json', alignment)
    if not args.reuse:
        manifest = {'version':VERSION,'alignment':alignment,'fields_only':args.fields_only,'mask':mask,
                    'tools':{str(p):sha(p.read_bytes()) for p in sorted(HERE.glob('*')) if p.suffix in ('.py','.lua','.json')},
                    'inputs':{str(p):sha(Path(p).read_bytes()) for p in paths},
                    'binaries':{p:sha(Path(p).read_bytes()) for p in (args.port,args.cart,args.cartrom,args.mame)}}
        write_json(out/'manifest.json',manifest)
    def run_one(s):
        print(f'{s.name}: {s.frames} VBlanks; MAME offset +{alignment["offset"]}', flush=True)
        base = out/s.name
        if args.reuse:
            p = load_records(base/'port/frames.jsonl')
            m = load_records(base/'mame/frames.jsonl')
            validate_frames(p,s.frames)
            validate_frames(m,s.frames+alignment['offset'])
        else:
            p = capture_port(args,s,base/'port',video=not args.fields_only)
            m = capture_binary(args,s,base/'mame',alignment['offset'],mask,
                             memory=not args.fields_only,video=not args.fields_only)
        result = compare(s,p,m,alignment)
        write_json(base/'comparison.json',result)
        print(f'{s.name}: {result["status"]}; coverage {result["coverage"]}', flush=True)
        return result
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_one,s):s.name for s in scenarios}
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())
            results.sort(key=lambda r:r['name'])
            write_json(out/'results.json',{'version':VERSION,'scenarios':results})
            (out/'summary.md').write_text(summary(results))
    print(summary(results))
    # CI: 0 complete match, 1 divergence/coverage failure, 2 incomplete or harness error.
    return 1 if any(r['status']=='DIVERGED' or r['coverage']=='UNMET' for r in results) else 2 if any(r['status']=='INCOMPLETE' for r in results) else 0

if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, RuntimeError, OSError, subprocess.TimeoutExpired) as e:
        print(f'scenario checker: {e}', file=sys.stderr)
        raise SystemExit(2)
