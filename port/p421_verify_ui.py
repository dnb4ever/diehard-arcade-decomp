#!/usr/bin/env python3
"""Actual SDL mouse toggles, open-folder URI and dropped ZIP installation."""
import json,os,shutil,subprocess
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file());os.chdir(ROOT);W=Path('work/p421');D=W/'ui-delivery';D.mkdir(exist_ok=True)
mods=D/'mods space ü';shutil.copytree(W/'data/mods',mods,dirs_exist_ok=True)
# Every initial mod is disabled; the driver enables only the first checkbox.
(mods/'enabled.txt').unlink(missing_ok=True)
shim=D/'ui.so'
subprocess.run(['c++','-std=c++20','-shared','-fPIC','-Iport/run/P46/sdl3deb/x/usr/include',str(W/'port/p421_ui_driver.cpp'),'-ldl','-L/usr/lib/x86_64-linux-gnu','-l:libSDL3.so.0','-o',str(shim)],check=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))};env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',SATURN_MODS_SHOT='1',SATURN_LAUNCHER_SHOT=str((D/'mods-after-drop.png').resolve()),SATURN_LAUNCHER_SHOT_FRAME='20',P421_DROP_ZIP=str((W/'loader-tests-final/valid.zip').resolve()),LD_PRELOAD=str(shim.resolve()))
cmd=[str(W/'build/saturn_stv'),'--cart',str(W/'snapshot/diehard_main.bin'),'--cartrom',str(W/'snapshot/cart.bin'),'--window','--launcher','--settings','-','--mods-dir',str(mods),'--mods','none','--out',str(D)]
with (D/'run.log').open('w') as f:subprocess.run(cmd,env=env,stdout=f,stderr=f,check=True,timeout=30)
log=(D/'run.log').read_text();assert (mods/'enabled.txt').read_text()=='frame-hud\n'
assert 'P421 OPEN file://' in log and '%20' in log and '%C3%BC' in log
assert (mods/'probe/manifest.ini').is_file() and (mods/'probe/mod.so').is_file()
assert not (mods/'enabled.txt').read_text().count('probe')
assert (D/'mods-after-drop.png').exists()
(D/'report.json').write_text(json.dumps({'checkbox_saved':['frame-hud'],'open_folder_uri_encoded':True,'dropped_deflate_zip_installed_disabled':True,'screenshot':str(D/'mods-after-drop.png')},indent=2)+'\n')
print('PASS: real SDL checkbox persists, Open folder emits encoded URI, dropped ZIP installs disabled')
