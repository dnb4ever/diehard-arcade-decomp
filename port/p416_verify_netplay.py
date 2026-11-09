#!/usr/bin/env python3
"""Two real game processes, packet impairments, checksums, full final state, PNG and PCM.

File mode explicitly uses test-only file datagrams. UDP mode uses real kernel
UDP, launching the portable proxy for impairments, and fails clearly if the OS
blocks sockets. UDP never compiles or loads a datagram shim. Linux test runner.
"""
import argparse, hashlib, json, os, re, shutil, signal, socket, struct, subprocess, time, sys, wave
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv',help='executable used for recording and both peers')
p.add_argument('--frames',type=int,default=9000)
p.add_argument('--cases',default='0:0:0,50:1:10,100:3:20,150:5:30',help='one-way ms:loss percent:jitter ms')
p.add_argument('--corruption',type=int,default=0,help='percent of datagrams with a deliberately damaged CRC')
p.add_argument('--delay',type=int,default=3)
p.add_argument('--tag',default='matrix')
p.add_argument('--transport',choices=['files','udp','files-proxy'],default='files',help='udp: kernel sockets; files-proxy: same proxy over test-only file sockets')
p.add_argument('--window',action='store_true',help='SDL dummy, paced, every presented field')
a=p.parse_args();root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p416')/a.tag;out.mkdir(parents=True,exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
binary=a.binary
base=[binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--netplay-no-map','--netplay-no-stun']
try:
 s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);s.bind(('127.0.0.1',0));s.close();real_udp=True
except OSError as e:
 real_udp=False;print('Kernel UDP unavailable:',str(e),flush=True)
if a.transport=='udp' and not real_udp:
 raise SystemExit('--transport udp requires kernel IPv4 UDP; rerun outside the sandbox, or select --transport files explicitly')
# For repeatable impaired tests use the datagram shim even if sockets work;
# a separate direct-UDP run is required on an unrestricted machine.
shim=out/'datagram-shim.so'
if a.transport!='udp':
 subprocess.run(['cc','-shared','-fPIC','-O2','port/p419_datagram_shim.c','-ldl','-o',str(shim)],check=True,capture_output=True)
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
report={'kernel_udp_available':real_udp,'transport':'kernel UDP' if a.transport=='udp' else 'test-only file-backed sockets through UDP proxy' if a.transport=='files-proxy' else 'test-only file-backed datagrams','frames':a.frames,'delay':a.delay,'corruption_percent':a.corruption,'window':a.window,'cases':{}}
for index,case in enumerate(a.cases.split(',')):
 latency,loss,jitter=map(int,case.split(':'));assert latency>=0 and jitter>=0 and 0<=loss<=100 and 0<=a.corruption<=100,'invalid impairment settings';d=out/f'{latency}ms-{loss}pct';d.mkdir(exist_ok=True)
 datagrams=(d/'datagrams').resolve()
 if datagrams.exists():shutil.rmtree(datagrams)
 if a.transport!='udp':datagrams.mkdir()
 testenv=dict(env,LD_PRELOAD=str(shim.resolve()),P416_DATAGRAM_DIR=str(datagrams),P416_LATENCY_MS=str(latency),P416_LOSS_BP=str(loss*100),P416_JITTER_MS=str(jitter),P416_CORRUPT_BP=str(a.corruption*100),STV_NETPLAY_DUMP='1')
 if a.transport=='files-proxy':testenv.update(P416_LATENCY_MS='0',P416_LOSS_BP='0',P416_JITTER_MS='0',P416_CORRUPT_BP='0')
 if a.transport=='udp':
  testenv={k:v for k,v in env.items() if not k.startswith('P416_')};testenv['STV_NETPLAY_DUMP']='1';testenv.pop('LD_PRELOAD',None)
 if a.window:testenv.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
 processes=[];start=time.monotonic();proxy=None;proxy_log=None
 host_port=18000+index;join_port=host_port
 try:
  if a.transport in ('udp','files-proxy') and any([latency,loss,jitter,a.corruption]):
   join_port=19000+index;proxy_log=(d/'proxy.log').open('w')
   proxy_command=[sys.executable,'port/p416_udp_proxy.py','--listen',f'127.0.0.1:{join_port}','--host',f'127.0.0.1:{host_port}','--latency',str(latency),'--loss',str(loss),'--jitter',str(jitter),'--corruption',str(a.corruption),'--seed',str(416+index),'--report',str(d/'proxy.json')]
   proxy=subprocess.Popen(proxy_command,env=testenv,stdout=proxy_log,stderr=proxy_log,start_new_session=True)
   until=time.monotonic()+5
   while 'UDP proxy ' not in (d/'proxy.log').read_text():
    assert proxy.poll() is None and time.monotonic()<until,('proxy startup failed',(d/'proxy.log').read_text())
    time.sleep(.01)

  for who,net in [('host',['--netplay-host',str(host_port)]),('guest',['--netplay-join',f'127.0.0.1:{join_port}'])]:
   dest=d/who;dest.mkdir(exist_ok=True);f=(dest/'run.log').open('w')
   if a.window:testenv['STV_NETPLAY_PLAYED']=str(dest/'played.s16')
   cmd=['/usr/bin/time','-o',str(dest/'time.txt'),'-f','%e %U %S']+base+['--vblanks',str(a.frames),'--input-delay',str(a.delay),'--stvinput','@'+str(script),'--shots',str(a.frames),'--wav',str(dest/'audio.wav'),'--out',str(dest)]+net
   if a.window:cmd += ['--window','--settings','-','--stats','5']
   processes.append((subprocess.Popen(cmd,env=testenv,stdout=f,stderr=f,start_new_session=True),f,dest));time.sleep(.05)
  for proc,f,dest in processes:
   rc=proc.wait(timeout=max(180,a.frames*.12+60));f.close()
   assert rc==0,(case,dest,rc,(dest/'run.log').read_text()[-3000:])
 finally:
  for proc,f,dest in processes:
   if proc.poll() is None:
    # Each peer owns a process group (/usr/bin/time plus its game child).
    try:os.killpg(proc.pid,signal.SIGTERM)
    except ProcessLookupError:pass
    proc.wait()
   f.close()
  if proxy is not None:
   if proxy.poll() is None:proxy.terminate()
   proxy.wait(timeout=5)
  if proxy_log is not None:proxy_log.close()
 records=[]
 for proc,f,dest in processes:
  log=(dest/'run.log').read_text();assert 'FATAL' not in log and 'desync/failure=0' in log,dest
  crc={int(f):c for f,c in re.findall(r'confirmed field=(\d+) CRC=([0-9a-f]+)',log)}
  stats=re.search(r'netplay: (\d+) rollbacks, (\d+) resim fields, max (\d+); captures=(\d+) ([\d.]+) ms/save, loads=(\d+) ([\d.]+) ms/load; resim total ([\d.]+) ms max burst ([\d.]+) ms;',log)
  assert stats,dest
  wall,user,system=map(float,(dest/'time.txt').read_text().split())
  stats={k:float(v) for k,v in zip(['rollbacks','resim_fields','max_rollback','captures','save_ms','loads','load_ms','resim_total_ms','max_burst_ms'],stats.groups())}
  stats.update(rollbacks_per_frame=stats['rollbacks']/a.frames,resim_per_frame=stats['resim_fields']/a.frames,wall_s=wall,user_s=user,system_s=system,cpu_percent=100*(user+system)/wall,confirmed=crc)
  audio=re.search(r'corrections=(\d+) corrected=(\d+) submitted-differences=(\d+) discontinuities=(\d+) max-step=(\d+) empty-blocks=(\d+); queue mean=([\d.]+) max=([\d.]+) ms; generation-to-submit mean=([\d.]+) max=([\d.]+) ms',log)
  if audio:stats['audio']={k:float(v) for k,v in zip(['corrections','corrected_samples','submitted_differences','discontinuities','max_step','empty_blocks','queue_mean_ms','queue_max_ms','submit_mean_ms','submit_max_ms'],audio.groups())}
  if (dest/'played.s16').exists():
   with wave.open(str(dest/'played.wav'),'wb') as played:
    played.setparams((2,2,44100,0,'NONE','not compressed'));played.writeframes((dest/'played.s16').read_bytes())
  records.append(stats)
 assert records[0]['confirmed']==records[1]['confirmed'],(case,'confirmed checksums differ')
 artifacts={}
 for name in [f'shot-{a.frames}.png','audio.wav','final-machine.bin']:
  sums=[hashlib.sha256((d/who/name).read_bytes()).hexdigest() for who in ['host','guest']]
  assert sums[0]==sums[1],(case,name,sums)
  artifacts[name]={'sha256':sums[0],'bytes':(d/'host'/name).stat().st_size}
 report['cases'][case]={'peers':records,'artifacts':artifacts,'elapsed_s':time.monotonic()-start,'proxy':json.loads((d/'proxy.json').read_text()) if proxy is not None else None}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(case,'PASS',len(records[0]['confirmed']),'checksums; PNG/PCM/full machine equal;',[(r['rollbacks'],r['max_rollback'],round(r['cpu_percent'],1)) for r in records],flush=True)
