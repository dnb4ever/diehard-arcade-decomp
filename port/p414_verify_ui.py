#!/usr/bin/env python3
"""Software SDL captures of the launcher, pause controls and the three presentation filters."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cases',default='launcher-main,launcher-features,sharp,smooth,crt,pause');a=p.parse_args();cases=a.cases.split(',')
out=Path('work/p414/ui');out.mkdir(exist_ok=True);report=json.loads((out/'report.json').read_text()) if (out/'report.json').exists() else {}
base={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','SDL_'))}
base.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
def run(name,extra,envextra=None):
 d=out/name;d.mkdir(exist_ok=True);env=base.copy();env.update(envextra or {})
 cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--window','--settings',str(d/'settings.ini'),'--out',str(d)]+extra
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=600)
 assert r.returncode==0 and 'FATAL' not in (d/'run.log').read_text(),name
 report[name]=dict(command=cmd,environment={k:v for k,v in env.items() if k.startswith(('SDL_','SATURN_','STV_','P413_','LD_PRELOAD'))},screenshots={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in d.glob('*.png')})
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(name,'PASS',flush=True)
for tab in ['main','features']:
 if 'launcher-'+tab not in cases:continue
 d=out/f'launcher-{tab}';d.mkdir(exist_ok=True);(d/'settings.ini').write_text('[display]\ninternal_resolution = 2\n')
 e={'SATURN_LAUNCHER_SHOT':str(d/'window.png')}
 if tab=='features':e['SATURN_LAUNCHER_FEATURES']='1'
 run('launcher-'+tab,['--launcher'],e)
for mode,crt in [('sharp',False),('smooth',False),('sharp',True)]:
 name='crt' if crt else mode
 if name not in cases:continue
 d=out/name;d.mkdir(exist_ok=True)
 (d/'settings.ini').write_text(f'[display]\ninternal_resolution = 2\nscaling = {mode}\ncrt = {str(crt).lower()}\nframeskip = false\n')
 run(name,['--unpaced','--vblanks','1210','--stvinput','99999:COIN'],{'SATURN_WINDOW_SHOT':str(d/'window.png'),'SATURN_WINDOW_SHOT_VB':'1200'})
# The retained P4.13 driver sends actual SDL Escape/Down/Enter events to the pause menu.
d=out/'pause';d.mkdir(exist_ok=True);(d/'settings.ini').write_text('[display]\ninternal_resolution = 3\n')
driver=ROOT/'work/p414/ui-driver.so'
if 'pause' in cases:
 assert driver.exists(),'Build port/p414_ui_driver.cpp as work/p414/ui-driver.so first'
 run('pause',['--unpaced','--vblanks','180'],{'LD_PRELOAD':str(driver),'SATURN_PLAYER_SHOT':str(d/'window.png')})
