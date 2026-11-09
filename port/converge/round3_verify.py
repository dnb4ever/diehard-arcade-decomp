#!/usr/bin/env python3
"""Replay the P4.5 schedules against frozen stock/override ports, hash streamed frames.

Reuses the port's existing read-only scenario FIFO. RAM/video bytes are consumed
and hashed in memory, never retained as enormous dumps. All paths are repo-relative.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import threading
import time

ROOT=Path(__file__).resolve().parents[2]
WORK=Path('work/converge/round3')
combat='200:COIN,340:START1,1350:P1RIGHT:100,'+','.join(f'{v}:P1A:2' for v in range(1450,1851,50))
SCHEDULES={
    'nocoin':(9000,'99999:COIN',list(range(300,9001,300)),{}),
    'gameover':(9000,'600:COIN,700:START1,6000:COIN,6100:START1',
        [600,700,1000,1500,2000,3000,4000,4500,4893,5000,5195,5300,6000,6050,6100,6926],{}),
    'combat':(2100,combat,[1320,1440,1560,1800,2000],{}),
    'legacy-combat':(2100,combat,[1320,1440,1560,1800,2000],{'STV_BOOTMODE':'0','STV_CFG798':'40'}),
    'autoplay':(12000,'99999:COIN',[1800,4000,7350,9000,12000],{'STV_AUTOPLAY':'100'}),
    'fullgame':(170000,'99999:COIN',list(range(5000,170001,5000)),{'STV_AUTOPLAY':'100'}),
}


def sha(data):return hashlib.sha256(data).hexdigest()


def run(binary,label,name,frames,audit=False):
    limit,timeline,shots,overrides=SCHEDULES[name]
    dest=WORK/'runs'/label/name;dest.mkdir(parents=True,exist_ok=True)
    env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','CONVERGE_'))}
    env.update(overrides,STV_SCENETRACE='1',CONVERGE_COUNTS='1')
    fixture=WORK/'fixtures';fixture.mkdir(exist_ok=True)
    env['CONVERGE_SAVE_FRAMES']=str(fixture)
    if audit:env['CONVERGE_AUDIT']='1'
    errors=[];reader=None
    record_path=dest/'frames.jsonl'
    if frames:
        fifo=dest/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
        env.update(STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO='0' if name=='fullgame' else '1',CONVERGE_RAM_INTERVAL='300' if name=='fullgame' else '1')
        def read():
            try:
                with fifo.open('rb') as f,record_path.open('w') as output:
                    def exact(n):
                        b=f.read(n)
                        if len(b)!=n:raise EOFError('short scenario record')
                        return b
                    while b:=f.read(4):
                        r=json.loads(exact(struct.unpack('<I',b)[0]))
                        ram=exact(r.pop('ram_bytes'));pixels=exact(r.pop('pixel_bytes'))
                        output.write(json.dumps(dict(frame=r['frame'],pixels=sha(pixels) if pixels else None,ram=sha(ram) if ram else None,
                            semantic=sha(json.dumps(r,sort_keys=True).encode())),sort_keys=True)+'\n')
            except Exception as error:errors.append(repr(error))
        reader=threading.Thread(target=read,daemon=True);reader.start()
    cmd=[str(binary),'--cart',str(WORK/'snapshot/port/diehard_main.bin'),
        '--cartrom',str(WORK/'snapshot/cart.bin'),'--vblanks',str(limit),
        '--stvinput',timeline,'--shots',','.join(map(str,shots)),'--out',str(dest)]
    start=time.monotonic()
    binary_sha256=sha(Path(binary).read_bytes())
    with (dest/'run.log').open('w') as log:
        result=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=7200)
    if reader:
        reader.join(10);fifo.unlink(missing_ok=True)
        if reader.is_alive():errors.append('capture reader did not finish')
    log=(dest/'run.log').read_text()
    trace='\n'.join(line for line in log.splitlines() if re.search(
        r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples',line))+'\n'
    (dest/'trace.txt').write_text(trace)
    png={f'shot-{v}.png':sha((dest/f'shot-{v}.png').read_bytes())
        for v in shots if (dest/f'shot-{v}.png').exists()}
    counts={int(m[0]):[int(m[1]),int(m[2])] for m in re.findall(
        r'CONVERGE count (\d+) master=(\d+) slave=(\d+)',log)}
    manifest=dict(command=cmd,binary_sha256=binary_sha256,overrides=overrides,exit_code=result.returncode,
        seconds=round(time.monotonic()-start,3),expected_shots=len(shots),png=png,
        trace_sha256=sha(trace.encode()),counts=counts,errors=errors,audit=audit)
    if frames and record_path.exists():
        lines=record_path.read_bytes().splitlines()
        manifest.update(frame_count=len(lines),frames_sha256=sha(record_path.read_bytes()),
            pixel_stream_sha256=sha(''.join(json.loads(line)['pixels'] or '' for line in lines).encode()),
            ram_stream_sha256=sha(''.join(json.loads(line)['ram'] or '' for line in lines).encode()))
    (dest/'result.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'{label}/{name}: exit={result.returncode}, PNG={len(png)}/{len(shots)}, '
          f'frames={manifest.get("frame_count",0)}, {manifest["seconds"]}s',flush=True)
    if result.returncode or 'FATAL' in log or errors or len(png)!=len(shots):
        raise RuntimeError(f'{label}/{name} failed; see {dest}/run.log')
    if frames and manifest.get('frame_count')!=limit:
        raise RuntimeError('incomplete frame capture')
    return manifest


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--variant',default='pilot')
    p.add_argument('--schedules',default='nocoin,gameover,combat,legacy-combat')
    p.add_argument('--reference',type=Path)
    p.add_argument('--frames',action='store_true')
    p.add_argument('--audit',action='store_true')
    p.add_argument('--label')
    a=p.parse_args();os.chdir(ROOT)
    label=a.label or a.variant
    dest=WORK/'runs'/label;dest.mkdir(parents=True,exist_ok=True)
    manifest={}
    for name in a.schedules.split(','):
        manifest[name]=run(WORK/f'build-{a.variant}/saturn_stv',label,name,a.frames,a.audit)
        (dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    if a.reference:
        reference=json.loads(a.reference.read_text());diff=[]
        for name,r in manifest.items():
            for key in ('png','trace_sha256','frame_count','frames_sha256'):
                if (key in r or key in reference[name]) and r.get(key)!=reference[name].get(key):
                    diff.append(f'{name}: {key}')
        (dest/'comparison.json').write_text(json.dumps(dict(reference=str(a.reference),differences=diff),indent=2)+'\n')
        print('Comparison: '+(', '.join(diff) if diff else 'all hashes match'),flush=True)
        if diff:raise SystemExit(1)


if __name__=='__main__':
    import round2_verify
    round2_verify.verify.WORK=WORK
    round2_verify.clock_input()
    main()
