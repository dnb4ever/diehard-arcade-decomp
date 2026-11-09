#!/usr/bin/env python3
"""Negative integration cases for handshake/desync, plus host-origin settings override."""
import json,os,shutil,subprocess,time
from pathlib import Path
R=Path(__file__).resolve().parent.parent;os.chdir(R)
out=Path('work/p416/protocol');out.mkdir(parents=True,exist_ok=True)
baseenv={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
shim=(out/'datagram-shim.so').resolve()
subprocess.run(['cc','-shared','-fPIC','-O2','port/p419_datagram_shim.c','-ldl','-o',str(shim)],check=True)
report={}
for name in ['delay-mismatch','desync','host-origin']:
 d=out/name;d.mkdir(exist_ok=True);wire=(d/'datagrams').resolve()
 if wire.exists():shutil.rmtree(wire)
 wire.mkdir()
 env=dict(baseenv,LD_PRELOAD=str(shim),P416_DATAGRAM_DIR=str(wire))
 procs=[];rows=[]
 try:
  for role in ['host','guest']:
   dest=d/role;dest.mkdir(exist_ok=True);settings=dest/'settings.ini'
   settings.write_text('[stv]\ndifficulty = '+('8' if role=='host' else '1')+'\nlives = '+('4' if role=='host' else '1')+'\n')
   childenv=env.copy()
   if name=='desync' and role=='guest':childenv['STV_SCENECHAR']='1'
   cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--netplay-no-map','--netplay-no-stun','--vblanks','300','--stvinput','100:COIN,130:START1,101:COIN2,131:START2','--shots','300','--settings',str(settings),'--out',str(dest)]
   cmd += ['--netplay-host','19785'] if role=='host' else ['--netplay-join','127.0.0.1:19785']
   cmd += ['--input-delay','2' if name=='delay-mismatch' and role=='guest' else '3']
   f=(dest/'run.log').open('w');procs.append((subprocess.Popen(cmd,env=childenv,stdout=f,stderr=f),f,dest));time.sleep(.05)
  for p,f,dest in procs:
   rc=p.wait(timeout=35);f.close();text=(dest/'run.log').read_text();rows.append({'exit':rc,'log':str(dest/'run.log')})
   if name=='delay-mismatch':assert rc==2 and 'ROM, delay or frame-limit mismatch' in text,(name,dest,rc,text[-2000:])
   elif name=='desync':assert rc==2 and 'desync at field' in text,(name,dest,rc,text[-2000:])
   else:assert rc==0 and 'desync/failure=0' in text,(name,dest,rc,text[-2000:])
  if name=='host-origin':assert (d/'host/shot-300.png').read_bytes()==(d/'guest/shot-300.png').read_bytes()
  report[name]=rows;print(name,'PASS',flush=True)
 finally:
  for p,f,dest in procs:
   if p.poll() is None:p.terminate();p.wait()
   f.close()
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
