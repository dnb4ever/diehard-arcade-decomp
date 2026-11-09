#!/usr/bin/env python3
"""CPU-only resolution comparisons. Raw images retain each internal pixel; sheets use 4:3."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parent.parent;os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--part',choices=['scenes','ending','sheets'],default='scenes');a=p.parse_args()
out=Path('work/p414/visual');out.mkdir(exist_ok=True);manifest=out/'manifest.json';report=json.loads(manifest.read_text()) if manifest.exists() else {}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
if a.part=='scenes':
 for scene in ['attract','fight','floor']:
  for scale in [1,2,3]:
   d=out/f'{scene}-{scale}x';d.mkdir(exist_ok=True)
   field=1200 if scene!='fight' else 1800
   if scale==1 and scene!='floor':
    src=Path('work/p414/regression/nocoin/shot-1200.png') if scene=='attract' else Path('work/p414/route/shot-1800.png')
    shutil.copy2(src,d/f'shot-{field}.png');report[f'{scene}-{scale}x']=dict(source=str(src),sha256=sha(src));continue
   env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
   env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
   if scene!='attract':env['STV_AUTOPLAY']='100'
   cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--internal-resolution',str(scale),'--vblanks',str(field+10),'--stvinput','99999:COIN','--shots',str(field),'--out',str(d)]
   if scene=='floor':
    ini=d/'scene.ini';ini.write_text('[training]\nstart_scene = 9\n');cmd+=['--settings',str(ini)]
   with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=600)
   assert r.returncode==0 and 'FATAL' not in (d/'run.log').read_text(),(scene,scale)
   report[f'{scene}-{scale}x']=dict(command=cmd,environment={k:env[k] for k in env if k.startswith(('STV_','SDL_'))},sha256=sha(d/f'shot-{field}.png'))
   manifest.write_text(json.dumps(report,indent=2)+'\n');print(scene,scale,'PASS',flush=True)
elif a.part=='ending':
 # This ending field has an entirely zero displayed VDP1 framebuffer. Recompose
 # the retained exact video capture: only the VDP2 RBG transform produces artwork.
 dump=Path('work/p411/full-capture/dump-154992.bin');shown=Path('work/p411/full-capture/shown-154992.bin')
 assert not any(shown.read_bytes())
 for scale in [1,2,3]:
  d=out/f'ending-{scale}x';d.mkdir(exist_ok=True);dst=d/'shot-154992.png'
  cmd=['work/p414/probes/p414_render_dump',str(dump),str(shown),str(dst),'all',str(scale)]
  with (d/'run.log').open('w') as f:subprocess.run(cmd,check=True,stdout=f,stderr=subprocess.STDOUT)
  report[f'ending-{scale}x']=dict(command=cmd,offline=True,source_dump_sha256=sha(dump),source_shown_sha256=sha(shown),displayed_vdp1_nonzero=0,sha256=sha(dst))
 manifest.write_text(json.dumps(report,indent=2)+'\n')
else:
 for scene,field in [('attract',1200),('fight',1800),('floor',1200),('ending',154992)]:
  sheet=Image.new('RGB',(2880,744),'#171b22');draw=ImageDraw.Draw(sheet)
  for i,scale in enumerate([1,2,3]):
   src=out/f'{scene}-{scale}x'/f'shot-{field}.png'
   im=Image.open(src).convert('RGB');sheet.paste(im.resize((960,720),Image.Resampling.NEAREST),(i*960,24))
   draw.text((i*960+12,6),f'{scale}x | {scene} | VB {field}'+(' | offline device capture' if scene=='ending' else ''),fill='white')
  sheet.save(out/f'{scene}-comparison.png')
 print('Four 4:3 comparison sheets written; inspect the raw images and sheets.',flush=True)
