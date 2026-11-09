#!/usr/bin/env python3
"""Linux software SDL captures and active-session key-lock integration test."""
import os,subprocess,time
from pathlib import Path
R=Path(__file__).resolve().parent.parent;os.chdir(R)
d=Path('work/p416/ui');wire=(d/'datagrams').resolve();wire.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
env.update(LD_PRELOAD=str((d/'datagram-shim.so').resolve())+':'+str((d/'ui-driver.so').resolve()),P416_DATAGRAM_DIR=str(wire),SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
subprocess.run(['cc','-shared','-fPIC','-O2','port/p419_datagram_shim.c','-ldl','-o',str(d/'datagram-shim.so')],check=True)
subprocess.run(['c++','-std=c++17','-shared','-fPIC','-Iport/run/P46/sdl3deb/x/usr/include','port/p416_ui_driver.cpp','-ldl','-o',str(d/'ui-driver.so')],check=True)
for role,net in [('host',['--netplay-host','7845']),('join',['--netplay-join','127.0.0.1:7845'])]:
 shotenv=dict(env,SATURN_NETPLAY_SHOT='1',SATURN_LAUNCHER_SHOT=str(d/f'launcher-{role}.png'))
 subprocess.run(['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--netplay-no-map','--netplay-no-stun','--launcher','--settings','-','--out',str(d)]+net,env=shotenv,check=True,timeout=20,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
procs=[]
for who,net in [('host',['--netplay-host','19786']),('guest',['--netplay-join','127.0.0.1:19786'])]:
 dest=d/who;dest.mkdir(exist_ok=True);f=(dest/'run.log').open('w')
 cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--netplay-no-map','--netplay-no-stun','--window','--settings','-','--vblanks','300','--input-delay','2','--stvinput','100:COIN,130:START1,101:COIN2,131:START2','--out',str(dest)]+net
 e=dict(env,SATURN_WINDOW_SHOT=str(dest/'overlay.png'),SATURN_WINDOW_SHOT_VB='200')
 procs.append((subprocess.Popen(cmd,env=e,stdout=f,stderr=f),f,dest));time.sleep(.05)
try:
 for p,f,dest in procs:
  assert p.wait(timeout=25)==0;f.close();text=(dest/'run.log').read_text()
  assert 'Quick states disabled during netplay' in text
  assert 'fast forward on' not in text
  assert not (dest/'diehard.quickstate').exists()
  assert 'stopped after 300 VBlanks' in text
  print(dest,'PASS',flush=True)
finally:
 for p,f,dest in procs:
  if p.poll() is None:p.terminate();p.wait()
  f.close()
