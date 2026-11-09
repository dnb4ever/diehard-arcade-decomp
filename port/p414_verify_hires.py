#!/usr/bin/env python3
"""P4.14 full RAM/semantic/PCM/hardware authority and enhanced rewind/replay checks."""
import argparse,hashlib,json,os,struct,subprocess,threading,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['equivalence','state'],default='equivalence');p.add_argument('--short',action='store_true');a=p.parse_args()
out=Path('work/p414/verify-short' if a.short else 'work/p414/verify');out.mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
def run(name,scale,extra=(),video=False,fields=2100):
 d=out/name;d.mkdir(exist_ok=True);rows=[];errors=[]
 fifo=d/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 env.update(STV_AUTOPLAY='100',STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO=str(int(video)))
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
 cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--internal-resolution',str(scale),'--vblanks',str(fields),'--stvinput','99999:COIN','--out',str(d),'--shots','1800,2100','--wav',str(d/'audio.wav')]+list(extra)
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=1200)
 t.join(10);fifo.unlink();assert r.returncode==0 and not errors and not t.is_alive(),(name,r.returncode,errors)
 result=dict(command=cmd,seconds=time.monotonic()-start,rows=rows,wav=sha((d/'audio.wav').read_bytes()))
 (d/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(name,len(rows),'fields',round(result['seconds'],3),'s',flush=True)
 return result

def parts(path):
 b=path.read_bytes();assert b[:8]==b'STVQUICK';at=48;result={}
 names=['machine','video','core','mmio','bios','stv','player','scu','scudsp','onchip','clock','smpc','stv-smpc','cdblock','cddrive','cdfs','cdpos','vdp1','vdp2','sound','scsp','scspdsp']
 for name in names:
  assert b[at:at+len(name)]==name.encode(),(name,at)
  at+=len(name);n=struct.unpack_from('<Q',b,at)[0];at+=8;result[name]=b[at:at+n];at+=n
 assert at==len(b)
 # Remove presentation-only companion data from VDP1. Everything else, including
 # both hardware framebuffers and the pending draw/erase state, must match.
 v=result['vdp1'];at=0x80000+2*512*256*2
 hi=v[at:at+2];at+=2+sum(bool(x) for x in hi)*1024*512*2+2
 scale=struct.unpack_from('<i',v,at)[0]
 count=4+(2*1024*512*scale*scale*2 if scale>1 else 0)
 result['vdp1']=v[:at]+v[at+count:]
 return result

if a.mode=='equivalence':
 reports=[]
 for scale in [1,2,3]:
  d=out/f'authority-{scale}x'
  inputs=['--record-inputs',str(d/'panel.inputs')] if scale==1 else ['--replay-inputs',str(out/'authority-1x/panel.inputs')]
  r=run(f'authority-{scale}x',scale,inputs+['--dump','1800,2100','--save-at','2000','--quick-state',str(d/'state.quickstate')]);reports.append(r)
  if scale>1:
   assert r['rows']==reports[0]['rows'],('guest diverged',scale)
   assert r['wav']==reports[0]['wav'],('PCM diverged',scale)
   for vb in [1800,2100]:assert (d/f'dump-{vb}.bin').read_bytes()==(out/'authority-1x'/f'dump-{vb}.bin').read_bytes(),('hardware dump',scale,vb)
 d=out/'authority-1x-replay'
 r=run('authority-1x-replay',1,['--replay-inputs',str(out/'authority-1x/panel.inputs'),'--save-at','2000','--quick-state',str(d/'state.quickstate')])
 assert r['rows']==reports[0]['rows'] and r['wav']==reports[0]['wav']
 # Compare replay against replay so host autoplay-event bookkeeping is identical too.
 states=[parts(d/'state.quickstate')]+[parts(out/f'authority-{scale}x/state.quickstate') for scale in [2,3]]
 for scale,state in zip([2,3],states[1:]):
  for name in states[0]:assert states[0][name]==state[name],('machine snapshot part',scale,name)
 (out/'equivalence.json').write_text(json.dumps(dict(fields=2100,scales=[1,2,3],full_ram=True,semantics=True,hardware_dumps=[1800,2100],pcm=True,guest_snapshot_parts=list(states[0])),indent=2)+'\n')
 print('PASS: 2100/2100 full RAM and semantic fields, hardware dumps and WAV identical at 1x/2x/3x',flush=True)
else:
 rewind_at,rewind_frames,stop,record_stop,save_at=(900,60,950,850,700) if a.short else (1800,120,1900,1700,1500)
 for scale in [2,3]:
  r=run(f'rewind-{scale}x',scale,['--rewind-seconds','20','--rewind-at',str(rewind_at),'--rewind-frames',str(rewind_frames)],video=True,fields=stop)
  seen={};equal=[]
  for row in r['rows']:
   if row['frame'] in seen:assert row==seen[row['frame']],('rewind diverged',scale,row['frame']);equal.append(row['frame'])
   else:seen[row['frame']]=row
  assert len(equal)==rewind_frames,(scale,len(equal))
  slot=out/f'origin-{scale}x.quickstate';movie=out/f'power-{scale}x.inputs';embedded=out/f'state-{scale}x.inputs'
  r=run(f'record-{scale}x',scale,['--record-inputs',str(movie),'--save-at',str(save_at),'--quick-state',str(slot)],video=True,fields=record_stop)
  z=run(f'replay-{scale}x',scale,['--replay-inputs',str(movie)],video=True,fields=record_stop)
  assert r['rows']==z['rows'] and r['wav']==z['wav'],('replay diverged',scale)
  r=run(f'record-state-{scale}x',scale,['--load-state','--quick-state',str(slot),'--record-inputs',str(embedded)],video=True,fields=record_stop)
  z=run(f'replay-state-{scale}x',scale,['--replay-inputs',str(embedded)],video=True,fields=record_stop)
  assert r['rows']==z['rows'] and r['wav']==z['wav'],('embedded replay diverged',scale)
 (out/'state-equality.json').write_text(json.dumps(dict(scales=[2,3],rewind_equal_fields=rewind_frames,power_replay_equal_fields=record_stop,embedded_replay_equal_fields=record_stop-save_at,pixels=True,full_ram=True,pcm=True),indent=2)+'\n')
 print('PASS: enhanced rewind, quick state and both input replay origins',flush=True)
