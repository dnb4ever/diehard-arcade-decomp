#!/usr/bin/env python3
"""Mod hashes in saves and file-backed two-process netplay, including rollback."""
import hashlib,json,os,re,shutil,struct,subprocess,time
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file());os.chdir(ROOT);W=Path('work/p421');D=W/'sessions-delivery';D.mkdir(exist_ok=True)
B=W/'build/saturn_stv';BASE=[str(B),'--cart',str(W/'snapshot/diehard_main.bin'),'--cartrom',str(W/'snapshot/cart.bin'),'--mods-dir',str(W/'data/mods')]
ENV={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))};ENV.update(LD_PRELOAD=str((W/'clock.so').resolve()),STV_SCENETRACE='1')
report={}
def run(name,args,mods='frame-hud,infinite-health',env=None):
 d=D/name;d.mkdir(exist_ok=True);cmd=BASE+['--mods',mods,'--out',str(d)]+args
 with (d/'run.log').open('w') as log:result=subprocess.run(cmd,env=env or ENV,stdout=log,stderr=log,timeout=180)
 text=(d/'run.log').read_text();assert result.returncode==0 and 'FATAL' not in text,(name,result.returncode,text[-2000:]);return d,text
# Save at 1500, resume to 1800, compare guest PNG and serialized HUD tick counter.
state=D/'origin.quick';d,log=run('save',['--vblanks','1800','--quick-state',str(state),'--save-at','1500','--shots','1800'])
assert state.exists() and 'saved VB 1500' in log
saved=state.read_bytes();assert b'frame-hud@1.0.0:' in saved and b'infinite-health@1.0.0:' in saved
# State mod section is explicitly length-prefixed, followed by each module's 256 bytes.
pos=saved.index(b'mods',48);length=struct.unpack_from('<Q',saved,pos+4)[0];part=saved[pos+12:pos+12+length]
n=struct.unpack_from('<I',part)[0];description=part[4:4+n].decode();counter=struct.unpack_from('<I',part,4+n)[0]
assert counter==1500,counter
assert len(part)==4+n+512
restored,text=run('restore',['--load-state','--quick-state',str(state),'--vblanks','1800','--shots','1800'])
assert 'loaded VB 1500' in text
assert (d/'shot-1800.png').read_bytes()==(restored/'shot-1800.png').read_bytes()
# Exact host overlay text/count is recovered from saved SDK state as well.
windowenv=dict(ENV,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',SATURN_WINDOW_SHOT=str((D/'restore-window.png').resolve()),SATURN_WINDOW_SHOT_VB='1800')
run('restore-window',['--load-state','--quick-state',str(state),'--vblanks','1800','--window','--no-launcher','--unpaced','--settings','-'],env=windowenv)
_,text=run('wrong-set',['--load-state','--quick-state',str(state),'--vblanks','1'],mods='none')
assert 'Rejected stale, incompatible or corrupt state' in text and 'loaded VB' not in text
# A changed library with unchanged ID/version also invalidates the state.
mod=W/'data/mods/frame-hud/mod.so';original=mod.read_bytes()
changed=D/'changed-mods';shutil.copytree(W/'data/mods',changed,dirs_exist_ok=True)
with (changed/'frame-hud/mod.so').open('ab') as f:f.write(b'p421 hash mutation')
_,text=run('wrong-library-hash',['--load-state','--quick-state',str(state),'--mods-dir',str(changed),'--vblanks','1'])
assert 'Rejected stale, incompatible or corrupt state' in text and 'loaded VB' not in text
report['saves']={'recorded_set':description,'serialized_hud_ticks':counter,'png_equal':True,'wrong_set_rejected':True,'changed_library_rejected':True}
print('save set, restore PNG, serialized HUD counter, mismatch and library mutation PASS',flush=True)
# Test-only file transport, never opens an Internet/UDP socket.
shim=D/'datagram.so';subprocess.run(['cc','-shared','-fPIC','-O2',str(W/'snapshot/port/p416_datagram_shim.c'),'-ldl','-o',str(shim)],check=True)
def peers(name,selections,frames=1200,latency=0,loss=0,jitter=0,changed_guest=False):
 d=D/name;d.mkdir(exist_ok=True);packets=d/'datagrams';packets.mkdir(exist_ok=True)
 env=dict(ENV,LD_PRELOAD=str(shim.resolve())+':'+str((W/'clock.so').resolve()),P416_DATAGRAM_DIR=str(packets.resolve()),P416_LATENCY_MS=str(latency),P416_LOSS_BP=str(loss*100),P416_JITTER_MS=str(jitter),STV_NETPLAY_DUMP='1')
 processes=[];start=time.monotonic()
 try:
  for index,who in enumerate(('host','guest')):
   dest=d/who;dest.mkdir(exist_ok=True);log=(dest/'run.log').open('w')
   args=['--netplay-host','18421'] if index==0 else ['--netplay-join','127.0.0.1:18421']
   cmd=BASE+['--mods',selections[index],'--vblanks',str(frames),'--input-delay','0','--stvinput','100:COIN,130:START1,101:COIN2,131:START2,500:P1A:80,530:P2B:90,800:P1LEFT:40','--shots',str(frames),'--wav',str(dest/'audio.wav'),'--out',str(dest)]+args
   if changed_guest and index:cmd+=['--mods-dir',str(changed)]
   processes.append((subprocess.Popen(cmd,env=env,stdout=log,stderr=log),log,dest));time.sleep(.05)
  codes=[]
  for process,log,dest in processes:codes.append(process.wait(timeout=150));log.close()
 finally:
  for process,log,dest in processes:
   if process.poll() is None:process.terminate();process.wait(timeout=10)
   log.close()
 logs=[(dest/'run.log').read_text() for _,_,dest in processes]
 mismatch=set(selections[0].split(','))!=set(selections[1].split(',')) or changed_guest
 if mismatch:
  assert codes==[2,2],(name,codes,[t[-1000:] for t in logs])
  assert all('mod set/hash mismatch' in t for t in logs)
  assert not any('origin ready' in t for t in logs)
  result={'returncodes':codes,'mod_hash_mismatch_rejected':True}
 else:
  assert codes==[0,0],(name,codes,[t[-2000:] for t in logs])
  assert all('desync/failure=0' in t and 'MODDED' in t for t in logs)
  artifacts={}
  for filename in (f'shot-{frames}.png','audio.wav','final-machine.bin'):
   sums=[hashlib.sha256((d/who/filename).read_bytes()).hexdigest() for who in ('host','guest')];assert sums[0]==sums[1],(name,filename,sums);artifacts[filename]=sums[0]
  checks=[dict(re.findall(r'confirmed field=(\d+) CRC=([0-9a-f]+)',t)) for t in logs];assert checks[0]==checks[1] and checks[0]
  stats=[re.search(r'netplay: (\d+) rollbacks, (\d+) resim fields, max (\d+)',t).groups() for t in logs]
  result={'returncodes':codes,'artifacts':artifacts,'confirmed_checksums':len(checks[0]),'rollback_stats':stats}
 result.update(seconds=time.monotonic()-start,transport='test-only file-backed datagrams',latency_ms=latency,loss_percent=loss,jitter_ms=jitter)
 report[name]=result;(D/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(name,'PASS',result,flush=True)
peers('mismatched-sets',('none','identity'),frames=2)
peers('mismatched-library-hash',('frame-hud','frame-hud'),frames=2,changed_guest=True)
peers('matching-mods',('frame-hud,infinite-health','infinite-health,frame-hud'),frames=2100)
peers('matching-mods-rollback',('frame-hud,infinite-health','infinite-health,frame-hud'),frames=1200,latency=30,loss=1,jitter=10)
(D/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS mod save identities and file-transport sessions',flush=True)
