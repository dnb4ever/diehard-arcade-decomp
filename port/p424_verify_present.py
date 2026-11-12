#!/usr/bin/env python3
"""Exercise the production VSync gate with slow returns, visibility and actual UI events."""
from pathlib import Path
import argparse, json, os, re, subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--netplay',action='store_true',help='also repeat the 100ms impaired pair with slow/hidden presentation')
a=p.parse_args();root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p424');out.mkdir(parents=True,exist_ok=True)
shim=out/'present-shim.so'
subprocess.run(['c++','-std=c++20','-shared','-fPIC','-Iport/run/P46/sdl3deb/x/usr/include','port/p424_present_shim.cpp','-ldl','-o',str(shim)],check=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','SDL_','P424_'))}
env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',LD_PRELOAD=str(shim.resolve()))
base=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--window','--out',str(out)]
report={}
for case,extra in [('fake-game',['--no-launcher','--vblanks','300']),('fake-launcher',['--launcher'])]:
 e=dict(env,P424_FAKE_PRESENT='1')
 if case=='fake-launcher':e.update(SATURN_LAUNCHER_SHOT=str(out/'fake-launcher.png'),SATURN_LAUNCHER_SHOT_FRAME='220')
 log=out/(case+'.log')
 with log.open('w') as f:subprocess.run(base+['--settings','-']+extra,env=e,stdout=f,stderr=f,check=True,timeout=30)
 text=log.read_text();transitions=re.findall(r'display: VSync (.*)',text)
 assert 'P424 interval=0 frame=3' in text and 'P424 hide' in text and 'P424 show' in text,text
 assert text.count('display: VSync adaptive')==2,transitions
 assert any('hidden/occluded/minimized' in t for t in transitions),transitions
 report[case]=dict(transitions=transitions,slow_fallback=True,visibility_recovery=True)
 print(case,'PASS',flush=True)
settings=out/'launcher-settings.ini';settings.unlink(missing_ok=True)
e=dict(env,P424_CLICK_VSYNC='1',SATURN_LAUNCHER_SHOT=str(out/'launcher-off.png'),SATURN_LAUNCHER_SHOT_FRAME='18')
with (out/'launcher-toggle.log').open('w') as f:subprocess.run(base+['--launcher','--settings',str(settings)],env=e,stdout=f,stderr=f,check=True,timeout=15)
assert 'vsync = false' in settings.read_text()
assert 'display: VSync off' in (out/'launcher-toggle.log').read_text()
report['checkbox']=dict(saved_off=True)
# Reopening must restore the unchecked state without injecting another click.
e=dict(env,SATURN_LAUNCHER_SHOT=str(out/'launcher-restored-off.png'))
with (out/'launcher-restore.log').open('w') as f:subprocess.run(base+['--launcher','--settings',str(settings)],env=e,stdout=f,stderr=f,check=True,timeout=15)
assert 'display: VSync on' not in (out/'launcher-restore.log').read_text()
report['checkbox']['reopened_off']=True
if a.netplay:
 # Keep the established verifier's assertions, routing only the SDL calls
 # through this additional explicit test-only fixture.
 source=Path('port/p416_verify_netplay.py').read_text().replace(
  'root=Path(__file__).resolve().parent.parent;os.chdir(root)','root=Path.cwd();os.chdir(root)').replace(
  'LD_PRELOAD=str(shim.resolve()),',"LD_PRELOAD=str(shim.resolve())+':'+str(Path('work/p424/present-shim.so').resolve()),")
 runner=out/'verify_stall_netplay.py';runner.write_text(source)
 with (out/'netplay-stall.log').open('w') as f:
  subprocess.run(['python3',str(runner),'--window','--transport','files','--frames','900','--cases','100:3:20','--tag','../p424/netplay-stall'],env=dict(os.environ,P424_FAKE_PRESENT='1'),stdout=f,stderr=f,check=True,timeout=180)
 for peer in ['host','guest']:
  text=(out/'netplay-stall/100ms-3pct'/peer/'run.log').read_text()
  assert 'P424 hide' in text and 'P424 show' in text and 'recovering' in text and 'desync/failure=0' in text
 report['netplay_stall']=dict(passed=True,fields=900,one_way_ms=100,loss_percent=3,jitter_ms=20)
(out/'present-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: production present gate, actual checkbox and persisted off setting')
