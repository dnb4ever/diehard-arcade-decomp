#!/usr/bin/env python3
"""Portable-Wine verification in a private prefix/Xvfb; no desktop/GPU use."""
import hashlib,json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;os.chdir(ROOT)
win=ROOT/'work/win';out=win/'p413-verify';out.mkdir(parents=True,exist_ok=True)
wine=win/'opt/wine-11.21-amd64-wow64/bin/wine';exe=win/'DieHardArcade-windows-x64/DieHardArcade.exe'
xlog=(out/'xvfb.log').open('w')
display=':'+str(200+os.getpid()%500)
xvfb=subprocess.Popen([str(ROOT/'port/run/P46/xdeb/x/usr/bin/Xvfb'),display,'-nolisten','local','-screen','0','1280x900x24','-nolisten','tcp','-extension','GLX'],stdout=subprocess.PIPE,stderr=xlog)
report={}
def windows(path):return 'Z:'+str(Path(path).resolve()).replace('/','\\')
try:
 time.sleep(.3)
 if xvfb.poll() is not None:raise RuntimeError('Xvfb could not bind its display socket; see xvfb.log (sandbox may prohibit bind)')
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SDL_','SATURN_','WINE'))}
 env.update(WINEPREFIX=str(win/'wineprefix-p413'),WINEARCH='win64',WINEDEBUG='-all',WINEDLLOVERRIDES='mscoree,mshtml=',DISPLAY=display,WINEPATH=windows(exe.parent),SDL_VIDEO_DRIVER='windows',SDL_RENDER_DRIVER='software',SDL_AUDIO_DRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1',__EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json')
 def run(name,args,*,native=False,extra=None,cwd=ROOT,program=exe):
  d=out/name;d.mkdir(exist_ok=True);e=env.copy();e.update(extra or {})
  cmd=[str(program),*map(str,args)] if native else [str(wine),str(program),*map(str,args)]
  start=time.monotonic()
  with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=e,cwd=cwd,stdout=f,stderr=subprocess.STDOUT,timeout=600)
  log=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in log,(name,r.returncode,log[-3000:])
  report[name]=dict(command=cmd,exit=r.returncode,seconds=round(time.monotonic()-start,3),png_md5={p.name:hashlib.md5(p.read_bytes()).hexdigest() for p in d.glob('*.png')})
  (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
  print(name,'PASS',report[name]['seconds'],'s',flush=True);return d
 source=out/'ROM folder café';source.mkdir(exist_ok=True)
 if not (source/'diehard.zip').exists():os.link(ROOT/'diehard.zip',source/'diehard.zip')
 # Actual GUI startup prepares the zip in-process, then captures/closes the launcher.
 run('prepare-launcher',['--rom-dir',windows(source)],extra={'SATURN_LAUNCHER_SHOT':windows(out/'prepare-launcher/launcher.png')})
 appdata=next((win/'wineprefix-p413/drive_c/users').glob('*/AppData/Roaming/Die Hard Arcade'))
 for name,original in [('cart.bin',ROOT/'cart.bin'),('diehard_main.bin',ROOT/'port/diehard_main.bin')]:
  assert hashlib.sha256((appdata/name).read_bytes()).digest()==hashlib.sha256(original.read_bytes()).digest(),name
 assert (appdata/'stv.ini').exists()
 run('remembered-launcher',[],extra={'SATURN_LAUNCHER_SHOT':windows(out/'remembered-launcher/launcher.png'),'SATURN_LAUNCHER_FEATURES':'1'},cwd=win)
 common=['--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks','2000','--shots','1900']
 linux=run('linux-smoke',common+['--out',str(out/'linux-smoke')],native=True,program=ROOT/'port/run/P46/build-sdl/saturn_stv')
 smoke=run('wine-smoke',common+['--out',windows(out/'wine-smoke')],extra={'SDL_VIDEO_DRIVER':'dummy'})
 assert (smoke/'shot-1900.png').read_bytes()==(linux/'shot-1900.png').read_bytes(),'Linux/Wine smoke differs'
 # Use newly prepared AppData images, with normal packaged settings/saves/output paths.
 run('save-record',['--no-launcher','--unpaced','--vblanks','2000','--save-at','1500','--shots','1900','--record-inputs',windows(appdata/'session.inputs')])
 original=(appdata/'shots/shot-1900.png').read_bytes();(out/'save-record/shot-1900.png').write_bytes(original)
 assert (appdata/'stv.ini.diehard.quickstate').exists() and (appdata/'session.inputs').exists()
 run('save-overwrite',['--no-launcher','--unpaced','--vblanks','1520','--save-at','1500'])
 run('cold-load',['--no-launcher','--unpaced','--vblanks','2000','--load-state','--shots','1900'])
 assert (appdata/'shots/shot-1900.png').read_bytes()==original,'cold quick load differs'
 replay=run('input-replay',['--cart',windows(appdata/'diehard_main.bin'),'--cartrom',windows(appdata/'cart.bin'),'--replay-inputs',windows(appdata/'session.inputs'),'--vblanks','2000','--shots','1900','--out',windows(out/'input-replay')],extra={'SDL_VIDEO_DRIVER':'dummy'})
 assert (replay/'shot-1900.png').read_bytes()==original,'Windows input replay differs'
 tests=win/'p413-tests/runtime_tests.exe'
 if tests.exists():
  d=run('runtime-tests',[],program=tests,extra={'SDL_VIDEO_DRIVER':'dummy'})
  assert '147 tests, 0 failed' in (d/'run.log').read_text()
 report['equality']=dict(smoke_md5=hashlib.md5((smoke/'shot-1900.png').read_bytes()).hexdigest(),zip_preparation=True,appdata=True,quick_overwrite_cold_load=True,input_replay=True,appdata_path=str(appdata.relative_to(ROOT)))
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: zip preparation, Unicode folder, AppData, Linux/Wine smoke, saves and input replay')
finally:
 if xvfb.poll() is None:xvfb.terminate();xvfb.wait(timeout=10)
 xlog.close()
