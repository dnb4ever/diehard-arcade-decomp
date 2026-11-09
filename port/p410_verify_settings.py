#!/usr/bin/env python3
"""P4.10: MAME EEPROM evidence and read-only native settings/effect captures."""
import argparse, hashlib, json, os, struct, subprocess, threading
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv')
a=p.parse_args()
os.chdir(Path(__file__).resolve().parent.parent)
out=Path('work/p410/settings');out.mkdir(parents=True,exist_ok=True)
factory=Path('work/p410/mame/factory/nvram/diehard/eeprom').read_bytes()
report={}
for name,flags in [('factory',0),('pause',1),('violence',2)]:
 eeprom=Path(f'work/p410/mame/{name}/nvram/diehard/eeprom').read_bytes()
 assert eeprom[0x20]==flags and eeprom[0x5c]==flags
 d=out/name;d.mkdir(exist_ok=True)
 ini=d/'stv.ini';ini.write_text(f'[stv]\npause_button = {str(flags==1).lower()}\nviolence = {str(flags==2).lower()}\n')
 fifo=d/'capture.fifo';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
 records={};errors=[]
 def capture():
  try:
   with fifo.open('rb') as f:
    while b:=f.read(4):
     r=json.loads(f.read(struct.unpack('<I',b)[0]));ram=f.read(r['ram_bytes']);f.read(r['pixel_bytes'])
     def u(addr,n=4):return int.from_bytes(ram[0x100000+addr-0x06000000:0x100000+addr-0x06000000+n],'big')
     r.update(flags=u(0x060987ef,1),paused=u(0x06098828))
     if r['frame']>=1200:records[r['frame']]=r
  except Exception as e:errors.append(repr(e))
 t=threading.Thread(target=capture,daemon=True);t.start()
 env={k:v for k,v in os.environ.items() if not k.startswith('STV_')}
 env.update(STV_SCENARIO_PIPE=str(fifo),STV_SCENARIO_VIDEO='0',STV_AUTOPLAY='100')
 shots=','.join(map(str,range(1250,2001,10)))
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--settings',str(ini),'--vblanks','2200','--stvinput','99999:COIN,1300:PAUSE,1500:PAUSE','--shots',shots,'--out',str(d)]
 with (d/'run.log').open('w')as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
 t.join(5);fifo.unlink()
 assert r.returncode==0 and not errors and not t.is_alive(),(name,errors)
 assert records[1250]['flags']==flags,(name,records[1250])
 if flags==1:
  assert all(records[n]['paused']==1 for n in range(1305,1495))
  assert all(records[n]['actors']==records[1320]['actors'] for n in range(1320,1495))
  assert records[1520]['paused']==0
 else:assert all(r['paused']==0 for r in records.values())
 report[name]=dict(command=cmd,eeprom_sha256=hashlib.sha256(eeprom).hexdigest(),eeprom_diff=[dict(offset=f'{i:02X}',before=x,after=y)for i,(x,y)in enumerate(zip(factory,eeprom))if x!=y],records=records)
 print(name,'PASS',flush=True)
# Blood adds graphics while preserving actor trajectory and PRNG consumption.
f=report['factory']['records'];v=report['violence']['records']
assert all(f[n]['actors']==v[n]['actors'] and f[n]['rng']==v[n]['rng'] for n in f)
from PIL import Image, ImageChops
changed=[]
for n in range(1250,2001,10):
 x=Image.open(out/'factory'/f'shot-{n}.png').convert('RGB')
 y=Image.open(out/'violence'/f'shot-{n}.png').convert('RGB')
 box=ImageChops.difference(x,y).getbbox()
 if box:changed.append(dict(frame=n,bounds=box))
assert len(changed)==7
assert (out/'pause'/'shot-1350.png').read_bytes()==(out/'pause'/'shot-1450.png').read_bytes()
report['effects']=dict(violence_differences=changed,actors_and_rng_equal_frames=len(f))
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
