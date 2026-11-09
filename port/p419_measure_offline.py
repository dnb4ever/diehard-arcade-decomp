#!/usr/bin/env python3
"""Offline paced queue/cadence comparison, same 9,000-field panel script."""
import os, subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent;os.chdir(root)
d=Path('work/p419/audio/offline');d.mkdir(parents=True,exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
with (d/'run.log').open('w') as log:
 subprocess.run(['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks','9000','--stvinput','@work/p419/audio/matrix/panels.txt','--window','--settings','-','--stats','5','--out',str(d)],env=env,stdout=log,stderr=log,check=True,timeout=240)
