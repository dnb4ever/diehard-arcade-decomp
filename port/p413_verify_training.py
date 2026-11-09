#!/usr/bin/env python3
"""Headless training values and scene routing; retains logs/PNGs and a compact report."""
import argparse,json,os,re,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv');p.add_argument('--cases',default='normal,health,lives,credits,scene28,scene72');a=p.parse_args()
os.chdir(Path(__file__).resolve().parent.parent)
out=Path('work/p413/training');out.mkdir(parents=True,exist_ok=True);report={}
for name,setting in [('normal',''),('health','infinite_health = true'),('lives','infinite_lives = true'),('credits','infinite_credits = true'),('scene28','start_scene = 28'),('scene72','start_scene = 72')]:
 if name not in a.cases.split(','):continue
 d=out/name;d.mkdir(exist_ok=True);ini=d/'player.ini';ini.write_text('[training]\n'+setting+'\n')
 env={k:v for k,v in os.environ.items() if not k.startswith('STV_')}
 env.update(STV_SCENETRACE='1',STV_PLAYER_TRACE='1')
 stop=2100
 if name!='credits':env['STV_AUTOPLAY']='100'
 else:stop=2100
 if name.startswith('scene'):stop=1200
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--settings',str(ini),'--vblanks',str(stop),'--stvinput','631:START1' if name=='credits' else '99999:COIN','--shots','900,1200'+(',1800,2100' if stop==2100 else ''),'--peek','06000794:1,06098668:1,06099788:4','--out',str(d)]
 start=time.monotonic()
 with (d/'run.log').open('w') as f:proc=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=240)
 log=(d/'run.log').read_text();assert proc.returncode==0 and 'FATAL' not in log,name
 rows=[dict(zip(['frame','player','actor','hp','lives','inv','assignment','scene','timer'],[int(x,16 if i in (2,6) else 10) for i,x in enumerate(m)])) for m in re.findall(r'TRAIN VB=(\d+) p=(\d+) actor=([0-9A-F]+) hp=(\d+) lives=(\d+) inv=(\d+) assignment=([0-9A-F]+) scene=(\d+) timer=(\d+)',log)]
 if name in ('health','lives'):
  assert len(rows)>1000,(name,len(rows))
  key,value={'health':('hp',114),'lives':('lives',9)}[name]
  assert all(row[key]==value for row in rows),(name,key)
 if name=='credits':
  assert 'ST-V coin:' not in log
  assert re.search(r'SCENETRACE.*credit\(0x674\)=2',log),'free start failed'
 if name.startswith('scene'):
  assert re.search(r'SCENETRACE.*scene='+name[5:]+r'\b',log),'selected scene not reached'
 report[name]=dict(command=cmd,seconds=round(time.monotonic()-start,3),samples=len(rows),min_hp=min([r['hp'] for r in rows],default=None),max_hp=max([r['hp'] for r in rows],default=None),min_lives=min([r['lives'] for r in rows],default=None),max_lives=max([r['lives'] for r in rows],default=None),pngs=[str(f) for f in sorted(d.glob('shot-*.png'))])
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(name,'PASS',report[name]['samples'],'samples',report[name]['seconds'],'s',flush=True)
print('PASS: health/lives held; no-coin free start; scene 28/72 loaded')
