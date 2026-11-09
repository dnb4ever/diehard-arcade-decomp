#!/usr/bin/env python3
"""Capture unmasked before/after fields, hardware, PCM and full state at a controlled RTC origin."""
import argparse,hashlib,json,os,struct,subprocess,threading,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',required=True);p.add_argument('--tag',required=True);a=p.parse_args()
out=Path('work/p415/authority')/a.tag;out.mkdir(parents=True,exist_ok=True)
if not (ROOT/'work/p415/fixed-time.so').exists():subprocess.run(['cc','-shared','-fPIC','port/p415_fixed_time.c','-o','work/p415/fixed-time.so'],check=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
def run(name,scale,extra=(),video=False,fields=2100):
 d=out/name;d.mkdir(exist_ok=True);rows=[];errors=[]
 fifo=d/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 env.update(LD_PRELOAD=str(ROOT/'work/p415/fixed-time.so'),TZ='UTC',STV_AUTOPLAY='100',STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO=str(int(video)))
 def read():
  try:
   with fifo.open('rb') as f:
    def exact(n):
     b=f.read(n)
     if len(b)!=n:raise EOFError('short capture')
     return b
    while b:=f.read(4):
     r=json.loads(exact(struct.unpack('<I',b)[0]));ram=exact(r.pop('ram_bytes'));px=exact(r.pop('pixel_bytes'))
     rows.append(dict(frame=r['frame'],ram=sha(ram),semantic=sha(json.dumps(r,sort_keys=True).encode()),pixels=sha(px)))
  except Exception as e:errors.append(repr(e))
 t=threading.Thread(target=read,daemon=True);t.start()
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--internal-resolution',str(scale),'--vblanks',str(fields),'--stvinput','99999:COIN','--out',str(d),'--shots','1800,1900,2100','--wav',str(d/'audio.wav')]+list(extra)
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=1200)
 t.join(10);fifo.unlink();assert r.returncode==0 and not errors and not t.is_alive(),(name,r.returncode,errors)
 result=dict(command=cmd,seconds=time.monotonic()-start,rows=rows,wav=sha((d/'audio.wav').read_bytes()))
 (d/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(name,len(rows),'fields',round(result['seconds'],3),'s',flush=True)
 return result

for scale in [1,2,3]:
 d=out/f'authority-{scale}x'
 run(f'authority-{scale}x',scale,['--record-inputs',str(d/'panel.inputs'),'--dump','1800,2100','--save-at','2000','--quick-state',str(d/'state.quickstate')])
print('Captured 2100 fields, PCM, shots, hardware and full machine state at each scale')
