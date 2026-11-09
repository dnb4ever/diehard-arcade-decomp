#!/usr/bin/env python3
"""Two real game processes, packet impairments, checksums, full final state, PNG and PCM.

Tries real IPv4 UDP first. If denied by the sandbox, explicitly uses the test
file-datagram shim, never claims kernel UDP coverage. Linux test tool only.
"""
import argparse, hashlib, json, os, re, shutil, socket, struct, subprocess, time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv',help='executable used for recording and both peers')
p.add_argument('--frames',type=int,default=9000)
p.add_argument('--cases',default='0:0:0,50:1:10,100:3:20,150:5:30',help='one-way ms:loss percent:jitter ms')
p.add_argument('--corruption',type=int,default=0,help='percent of datagrams with a deliberately damaged CRC')
p.add_argument('--delay',type=int,default=3)
p.add_argument('--tag',default='matrix')
p.add_argument('--transport',choices=['files','udp'],default='files',help='udp requires unrestricted loopback and a zero-impairment case')
p.add_argument('--window',action='store_true',help='SDL dummy, paced, every presented field')
a=p.parse_args();root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p416')/a.tag;out.mkdir(parents=True,exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
binary=a.binary
base=[binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin']
try:
 s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);s.bind(('127.0.0.1',0));s.close();real_udp=True
except OSError as e:
 real_udp=False;print('Kernel UDP unavailable:',str(e),'; using file-datagram transport test shim',flush=True)
if a.transport=='udp' and (not real_udp or a.cases!='0:0:0'):
 raise SystemExit('--transport udp needs working IPv4 UDP and --cases 0:0:0')
# For repeatable impaired tests use the datagram shim even if sockets work;
# a separate direct-UDP run is required on an unrestricted machine.
shim=out/'datagram-shim.so'
subprocess.run(['cc','-shared','-fPIC','-O2','port/p416_datagram_shim.c','-ldl','-o',str(shim)],check=True,capture_output=True)
script=out/'panels.txt'
if not script.exists():
 # Capture an original gameplay-driven P1 script once, then replay those physical
 # pins for both peers. P2 has independently phased attacks/movement/continues.
 timeline=['100:COIN','130:START1','101:COIN2','131:START2']
 for f in range(180,a.frames+1,24):
  timeline += [f'{f}:P2A:2',f'{f+12}:P2B:2']
  if f%120<24:timeline.append(f'{f}:P2RIGHT:20')
  if 240<=f%600<264:timeline.append(f'{f}:P2LEFT:20')
 for f in range(1300,a.frames,600):timeline += [f'{f}:COIN2',f'{f+30}:START2']
 movie=out/'script.inputs';capture=out/'script-origin';capture.mkdir(exist_ok=True)
 cmd=base+['--vblanks',str(a.frames),'--record-inputs',str(movie),'--stvinput',','.join(timeline),'--out',str(capture)]
 with (capture/'run.log').open('w') as log:
  subprocess.run(cmd,env=dict(env,STV_AUTOPLAY='100'),stdout=log,stderr=log,check=True,timeout=600)
 b=movie.read_bytes();meta,state,n=struct.unpack_from('<QQQ',b,56)[0],struct.unpack_from('<Q',b,32)[0],struct.unpack_from('<Q',b,40)[0]
 panels=[struct.unpack_from('<HHH',b,64+meta+state+f*6) for f in range(n)]
 mapping={0:'P1A',1:'P1B',2:'P1C',6:'P1DOWN',7:'P1UP',8:'P1RIGHT',9:'P1LEFT'}
 masks=[]
 for player in (0,1):
  for bit,name in mapping.items():masks.append((player,1<<bit,name.replace('P1',f'P{player+1}')))
 for bit,name in [(0,'COIN'),(1,'COIN2'),(4,'START1'),(5,'START2')]:masks.append((2,1<<bit,name))
 tokens=[]
 for word,mask,name in masks:
  start=None
  for f in range(len(panels)+1):
   held=f<len(panels) and panels[f][word]&mask
   if held and start is None:start=f+1
   if not held and start is not None:tokens.append((start,f'{start}:{name}:{f+1-start}'));start=None
 script.write_text(','.join(t for _,t in sorted(tokens)))
 print('Recorded',len(panels),'fields,',len(tokens),'panel events',flush=True)
report={'kernel_udp_available':real_udp,'transport':'kernel UDP' if a.transport=='udp' else 'test-only file-backed datagrams','frames':a.frames,'delay':a.delay,'corruption_percent':a.corruption,'window':a.window,'cases':{}}
for index,case in enumerate(a.cases.split(',')):
 latency,loss,jitter=map(int,case.split(':'));d=out/f'{latency}ms-{loss}pct';d.mkdir(exist_ok=True)
 datagrams=(d/'datagrams').resolve()
 if datagrams.exists():shutil.rmtree(datagrams)
 datagrams.mkdir()
 testenv=dict(env,LD_PRELOAD=str(shim.resolve()),P416_DATAGRAM_DIR=str(datagrams),P416_LATENCY_MS=str(latency),P416_LOSS_BP=str(loss*100),P416_JITTER_MS=str(jitter),P416_CORRUPT_BP=str(a.corruption*100),STV_NETPLAY_DUMP='1')
 if a.transport=='udp':testenv.pop('LD_PRELOAD',None)
 if a.window:testenv.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
 processes=[];start=time.monotonic()
 try:
  for who,net in [('host',['--netplay-host',str(18000+index)]),('guest',['--netplay-join',f'127.0.0.1:{18000+index}'])]:
   dest=d/who;dest.mkdir(exist_ok=True);f=(dest/'run.log').open('w')
   cmd=['/usr/bin/time','-o',str(dest/'time.txt'),'-f','%e %U %S']+base+['--vblanks',str(a.frames),'--input-delay',str(a.delay),'--stvinput','@'+str(script),'--shots',str(a.frames),'--wav',str(dest/'audio.wav'),'--out',str(dest)]+net
   if a.window:cmd += ['--window','--settings','-','--stats','5']
   processes.append((subprocess.Popen(cmd,env=testenv,stdout=f,stderr=f),f,dest));time.sleep(.05)
  for proc,f,dest in processes:
   rc=proc.wait(timeout=max(180,a.frames*.12+60));f.close()
   assert rc==0,(case,dest,rc,(dest/'run.log').read_text()[-3000:])
 finally:
  for proc,f,dest in processes:
   if proc.poll() is None:
    # Only our /usr/bin/time child and its direct game child.
    children=subprocess.run(['pgrep','-P',str(proc.pid)],capture_output=True,text=True).stdout.split()
    for pid in children:
     try:os.kill(int(pid),15)
     except ProcessLookupError:pass
    proc.terminate();proc.wait()
   f.close()
 records=[]
 for proc,f,dest in processes:
  log=(dest/'run.log').read_text();assert 'FATAL' not in log and 'desync/failure=0' in log,dest
  crc={int(f):c for f,c in re.findall(r'confirmed field=(\d+) CRC=([0-9a-f]+)',log)}
  stats=re.search(r'netplay: (\d+) rollbacks, (\d+) resim fields, max (\d+); captures=(\d+) ([\d.]+) ms/save, loads=(\d+) ([\d.]+) ms/load; resim total ([\d.]+) ms max burst ([\d.]+) ms;',log)
  assert stats,dest
  wall,user,system=map(float,(dest/'time.txt').read_text().split())
  stats={k:float(v) for k,v in zip(['rollbacks','resim_fields','max_rollback','captures','save_ms','loads','load_ms','resim_total_ms','max_burst_ms'],stats.groups())}
  stats.update(rollbacks_per_frame=stats['rollbacks']/a.frames,resim_per_frame=stats['resim_fields']/a.frames,wall_s=wall,user_s=user,system_s=system,cpu_percent=100*(user+system)/wall,confirmed=crc)
  records.append(stats)
 assert records[0]['confirmed']==records[1]['confirmed'],(case,'confirmed checksums differ')
 artifacts={}
 for name in [f'shot-{a.frames}.png','audio.wav','final-machine.bin']:
  sums=[hashlib.sha256((d/who/name).read_bytes()).hexdigest() for who in ['host','guest']]
  assert sums[0]==sums[1],(case,name,sums)
  artifacts[name]={'sha256':sums[0],'bytes':(d/'host'/name).stat().st_size}
 report['cases'][case]={'peers':records,'artifacts':artifacts,'elapsed_s':time.monotonic()-start}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(case,'PASS',len(records[0]['confirmed']),'checksums; PNG/PCM/full machine equal;',[(r['rollbacks'],r['max_rollback'],round(r['cpu_percent'],1)) for r in records],flush=True)
