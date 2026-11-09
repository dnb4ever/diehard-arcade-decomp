#!/usr/bin/env python3
"""Record/replay from power-on and an embedded quick state, comparing every field and full RAM."""
import argparse,hashlib,json,os,struct,subprocess,threading,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv');a=p.parse_args()
os.chdir(Path(__file__).resolve().parent.parent)
out=Path('work/p413/inputs');out.mkdir(parents=True,exist_ok=True);report={}
def run(name,extra,record=False,capture=True):
 d=out/name;d.mkdir(exist_ok=True);rows=[];errors=[]
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 if record:env['STV_AUTOPLAY']='100'
 if capture:
  fifo=d/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
  env.update(STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO='1')
  def read():
   try:
    with fifo.open('rb') as f:
     def exact(n):
      b=f.read(n)
      if len(b)!=n:raise EOFError('short record')
      return b
     while b:=f.read(4):
      r=json.loads(exact(struct.unpack('<I',b)[0]));ram=exact(r.pop('ram_bytes'));px=exact(r.pop('pixel_bytes'))
      rows.append(dict(frame=r['frame'],pixels=hashlib.sha256(px).hexdigest(),ram=hashlib.sha256(ram).hexdigest(),semantic=hashlib.sha256(json.dumps(r,sort_keys=True).encode()).hexdigest()))
   except Exception as e:errors.append(repr(e))
  thread=threading.Thread(target=read,daemon=True);thread.start()
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks','2100','--stvinput','99999:COIN','--out',str(d),'--wav',str(d/'audio.wav')]+extra
 if name=='replay-power':
  i=cmd.index('--vblanks');del cmd[i:i+2]
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=300)
 if capture:thread.join(5);fifo.unlink();assert not thread.is_alive() and not errors,(name,errors)
 log=(d/'run.log').read_text();assert 'FATAL' not in log,(name,log[-2000:])
 report[name]=dict(command=cmd,seconds=time.monotonic()-start,exit=r.returncode,records=rows,log=log)
 print(name,r.returncode,len(rows),'fields',round(report[name]['seconds'],3),'s',flush=True)
 return rows,log,r.returncode
slot=out/'origin.quickstate';power=out/'power.inputs';state=out/'state.inputs'
one,_,rc=run('record-power',['--record-inputs',str(power),'--quick-state',str(slot),'--save-at','1500'],record=True);assert rc==0
other,_,rc=run('replay-power',['--replay-inputs',str(power)]);assert rc==0
assert hashlib.sha256((out/'record-power/audio.wav').read_bytes()).digest()==hashlib.sha256((out/'replay-power/audio.wav').read_bytes()).digest(), 'power-on PCM diverged'
assert one==other,('power-on diverged',next((x['frame'] for x,y in zip(one,other) if x!=y),None),len(one),len(other))
x,_,rc=run('record-state',['--load-state','--quick-state',str(slot),'--record-inputs',str(state)],record=True);assert rc==0
z,_,rc=run('replay-state',['--replay-inputs',str(state)]);assert rc==0
assert hashlib.sha256((out/'record-state/audio.wav').read_bytes()).digest()==hashlib.sha256((out/'replay-state/audio.wav').read_bytes()).digest(), 'embedded-state PCM diverged'
assert x==z and len(x)==600,('embedded-state diverged',next((u['frame'] for u,v in zip(x,z) if u!=v),None),len(x),len(z))
b=power.read_bytes()
for name,data in [('version',b[:8]+struct.pack('<Q',99)+b[16:]),('identity',b[:16]+bytes([b[16]^1])+b[17:]),('truncated',b[:70]),('checksum',b[:-1]+bytes([b[-1]^1]))]:
 bad=out/(name+'.inputs');bad.write_bytes(data)
 _,log,rc=run(name,['--replay-inputs',str(bad)],capture=False)
 assert rc==2 and ('rejected' in log or 'invalid length' in log),(name,rc)
report['equality']=dict(power_fields=len(one),state_fields=len(x),full_ram=True,semantic=True,power_bytes=power.stat().st_size,state_bytes=state.stat().st_size)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: power-on + embedded-state pixels/RAM/semantics match; four bad files rejected')
