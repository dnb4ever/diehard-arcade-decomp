#!/usr/bin/env python3
"""Capture the new flows and exercise SDL copy/paste buttons with mouse events."""
import base64,os,re,struct,subprocess,zlib
from pathlib import Path
os.chdir(Path(__file__).resolve().parent.parent)
d=Path('work/p419/traversal/ui');d.mkdir(parents=True,exist_ok=True)
subprocess.run(['cc','-shared','-fPIC','-O2','port/p419_datagram_shim.c','-ldl','-o',str(d/'datagram-shim.so')],check=True)
subprocess.run(['c++','-std=c++17','-shared','-fPIC','-Iport/run/P46/sdl3deb/x/usr/include','port/p419_ui_driver.cpp','-ldl','-o',str(d/'ui-driver.so')],check=True)
fingerprint=int(re.search(r'STV_NETPLAY_CODE 0x([0-9a-f]+)',Path('port/run/P46/build-sdl/p410_identity.h').read_text())[1],16)&0xffffffff
b=b'DH\x01\x01'+struct.pack('!IQIH',fingerprint,0x123456789abcdef0,0x7f000001,7846);b+=struct.pack('!I',zlib.crc32(b))
remote='DHA1-'+base64.urlsafe_b64encode(b).decode().rstrip('=')
for mode,args in [('host',['--netplay-host','7845']),('join',['--netplay-join','127.0.0.1:7845']),('code',['--netplay-code','7845'])]:
 wire=(d/(mode+'-datagrams')).resolve();wire.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
 env.update(LD_PRELOAD=str((d/'datagram-shim.so').resolve()),P416_DATAGRAM_DIR=str(wire),STV_NETPLAY_LAN='127.0.0.1',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',SATURN_NETPLAY_SHOT='1',SATURN_LAUNCHER_SHOT=str(d/(mode+'.png')),SATURN_LAUNCHER_SHOT_FRAME='65')
 if mode=='code':env.update(LD_PRELOAD=env['LD_PRELOAD']+':'+str((d/'ui-driver.so').resolve()),P419_REMOTE_CODE=remote)
 command=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--launcher','--settings','-','--out',str(d),'--netplay-no-map','--netplay-no-stun']+args
 with (d/(mode+'.log')).open('w') as log:subprocess.run(command,env=env,stdout=log,stderr=log,check=True,timeout=15)
 if mode=='code':assert 'P419 clipboard copy: DHA1-' in (d/'code.log').read_text()
 print(mode,'PASS',flush=True)
