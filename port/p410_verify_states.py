#!/usr/bin/env python3
"""Mid-combat ST-V quick-state rewind, cold load, frame/RAM/PCM equality, and stale rejection."""
import argparse, hashlib, json, os, re, struct, subprocess, threading, time, wave
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv')
p.add_argument('--output',default='work/p410/states')
a=p.parse_args();os.chdir(Path(__file__).resolve().parent.parent)
out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
slot=out/'diehard.quickstate'
env={k:v for k,v in os.environ.items() if not k.startswith('STV_')}
env['STV_AUTOPLAY']='100'
report={}
def run(name,extra,capture=True,stop=2000):
 d=out/name;d.mkdir(exist_ok=True);records=[];errors=[]
 e=env.copy()
 if capture:
  fifo=d/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
  e.update(STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO='1')
  def read():
   try:
    with fifo.open('rb') as f:
     def exact(n):
      b=f.read(n)
      if len(b)!=n:raise EOFError('short scenario record')
      return b
     while b:=f.read(4):
      r=json.loads(exact(struct.unpack('<I',b)[0]));ram=exact(r.pop('ram_bytes'));px=exact(r.pop('pixel_bytes'))
      if r['frame']>=1501:
       records.append(dict(frame=r['frame'],pixels=hashlib.sha256(px).hexdigest(),ram=hashlib.sha256(ram).hexdigest(),semantic=hashlib.sha256(json.dumps(r,sort_keys=True).encode()).hexdigest()))
   except Exception as ex:errors.append(repr(ex))
  t=threading.Thread(target=read,daemon=True);t.start()
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks',str(stop),'--stvinput','99999:COIN','--quick-state',str(slot),'--out',str(d),'--wav',str(d/'audio.wav')]+extra
 start=time.monotonic()
 with (d/'run.log').open('w')as f:r=subprocess.run(cmd,env=e,stdout=f,stderr=subprocess.STDOUT,timeout=360)
 if capture:t.join(5);fifo.unlink()
 assert r.returncode==0 and not errors and (not capture or not t.is_alive()),(name,errors)
 log=(d/'run.log').read_text();assert 'FATAL'not in log
 report[name]=dict(command=cmd,seconds=time.monotonic()-start,records=records)
 print(name,'PASS',len(records),'frames',flush=True)
 return records,log
warm,log=run('rewind',['--save-at','1500','--load-at','1800','--shots','1501,1550,1800,1900'])
assert 'saved VB 1500' in log and 'loaded VB 1500' in log
original=warm[:300];loaded=warm[300:]
assert len(original)==300 and len(loaded)==500
assert original==loaded[:300],'warm rewind diverged'
cold,coldlog=run('cold',['--load-state','--shots','1501,1550,1800,1900'])
assert 'loaded VB 1500' in coldlog and cold==loaded,'cold restore diverged'
# Parse the recorded sound sample cursor from the versioned file's named parts.
b=slot.read_bytes();at=48;parts={}
names=['machine','video','core','mmio','bios','stv','player','scu','scudsp','onchip','clock','smpc','stv-smpc','cdblock','cddrive','cdfs','cdpos','vdp1','vdp2','sound','scsp','scspdsp']
for n in names:
 assert b[at:at+len(n)]==n.encode();at+=len(n);size=struct.unpack_from('<Q',b,at)[0];at+=8;parts[n]=b[at:at+size];at+=size
assert at==len(b)
saved=struct.unpack_from('<Q',parts['sound'],1)[0]
final=int(re.search(r'sound: (\d+) samples',log)[1])
def pcm(name):
 with wave.open(str(out/name/'audio.wav'),'rb')as w:
  assert w.getnchannels()==2 and w.getframerate()==44100 and w.getsampwidth()==2
  return w.readframes(w.getnframes())
x=pcm('rewind');y=pcm('cold');load_sample=len(x)//4-final+saved
assert len(y)//4==final-saved
assert x[load_sample*4:]==y,'PCM after restore diverged'
assert x[saved*4:load_sample*4]==y[:(load_sample-saved)*4],'original PCM diverged'
assert any(y)
report['equality']=dict(warm_frames=300,cold_frames=500,ram_and_semantics=True,saved_sample=saved,final_sample=final,pcm_stereo_frames=len(y)//4,pcm_sha256=hashlib.sha256(y).hexdigest(),slot_bytes=len(b),parts={k:len(v)for k,v in parts.items()})
# A stale or damaged file must be rejected before mutating any device, then boot normally.
for name,modified in [('version',b[:8]+struct.pack('<Q',99)+b[16:]),('build',b[:16]+bytes([b[16]^1])+b[17:]),('truncated',b[:100]),('checksum',b[:-1]+bytes([b[-1]^1]))]:
 bad=out/(name+'.quickstate');bad.write_bytes(modified)
 _,badlog=run(name,['--load-state','--quick-state',str(bad)],capture=False,stop=10)
 assert 'Rejected' in badlog and 'starting normally' in badlog,name
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: 300 warm + 500 cold frames, full RAM/semantics and PCM match; 4 invalid states rejected',flush=True)
