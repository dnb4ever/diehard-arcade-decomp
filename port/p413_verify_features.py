#!/usr/bin/env python3
"""CPU-only P4.13 checks; evidence is retained under work/p413/verify."""
import argparse, hashlib, json, os, struct, subprocess, threading, time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('feature', choices=['rewind','speed'])
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv')
a=p.parse_args()
out=Path('work/p413/verify');out.mkdir(parents=True,exist_ok=True)
def run(name, extra, fields=2100, capture=True, window=False):
 d=out/name;d.mkdir(exist_ok=True);records=[];errors=[]
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 env.update(STV_AUTOPLAY='100',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
 if capture:
  fifo=d/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
  env.update(STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO='1')
  def read():
   try:
    with fifo.open('rb') as f:
     def exact(n):
      b=f.read(n)
      if len(b)!=n:raise EOFError('short scenario record')
      return b
     while b:=f.read(4):
      r=json.loads(exact(struct.unpack('<I',b)[0]));ram=exact(r.pop('ram_bytes'));px=exact(r.pop('pixel_bytes'))
      records.append(dict(frame=r['frame'],pixels=hashlib.sha256(px).hexdigest(),ram=hashlib.sha256(ram).hexdigest(),semantic=hashlib.sha256(json.dumps(r,sort_keys=True).encode()).hexdigest()))
   except Exception as ex:errors.append(repr(ex))
  t=threading.Thread(target=read,daemon=True);t.start()
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks',str(fields),'--stvinput','99999:COIN','--out',str(d)]+extra
 if window:cmd+=['--window','--settings','-','--stats','5']
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=360)
 elapsed=time.monotonic()-start
 if capture:t.join(5);fifo.unlink()
 assert r.returncode==0 and not errors and (not capture or not t.is_alive()),(name,errors,r.returncode)
 log=(d/'run.log').read_text();assert 'FATAL' not in log,log[-2000:]
 result=dict(command=cmd,seconds=elapsed,records=records,log=log)
 (d/'report.json').write_text(json.dumps(result,indent=2)+'\n')
 print(name,round(elapsed,3),'s',len(records),'records',flush=True)
 return result
if a.feature=='rewind':
 r=run('rewind',['--rewind-seconds','20','--rewind-at','1800','--rewind-frames','300'])
 by={};compared=[]
 for x in r['records']:
  if x['frame'] in by:
   assert x==by[x['frame']],('rewind diverged',x['frame'])
   compared.append(x['frame'])
  else:by[x['frame']]=x
 assert len(compared)==300,len(compared)
 (out/'rewind-equality.json').write_text(json.dumps(dict(equal_frames=len(compared),first=min(compared),last=max(compared),ram=True,semantics=True),indent=2)+'\n')
 print('PASS: 300 rewound fields are bit-identical (pixels, full RAM, semantics)')
else:
 for name,extra in [('baseline',[]),('history',['--rewind-seconds','20'])]:
  run('speed-'+name,extra,fields=1800,capture=False,window=True)
