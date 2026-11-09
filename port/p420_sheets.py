#!/usr/bin/env python3
"""Make labelled physical-aspect sheets; the raw game pixels are retained too."""
from PIL import Image,ImageDraw
from pathlib import Path
import hashlib,json
root=Path('work/p420/visual');report={}
for scene,field in [('attract',1200),('fight',1800),('floor',1200),('boss',1200),('cutscene',680)]:
 sheet=Image.new('RGB',(2450,510),'#171b22');d=ImageDraw.Draw(sheet);x=0
 for variant,w,label in [('original',600,'4:3 original'),('wide',800,'16:9 experiment; wider culling'),('ultrawide',1050,'21:9 experiment; wider culling')]:
  f=root/scene/variant/f'shot-{field}.png';im=Image.open(f).convert('RGB');sheet.paste(im.resize((w,450),Image.Resampling.NEAREST),(x,45));d.text((x+8,10),f'{scene} VB{field} | {label}',fill='white');x+=w
 path=root/f'{scene}-comparison.png';sheet.save(path);report[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 # Equal-size three actual subfields, including honest fallback labels from the log.
 folder=root/scene/'hfr';files=sorted(folder.glob(f'sample-{field}-*.png'))[:2]+sorted(folder.glob(f'sample-{field+1}-*.png'))[:1]
 if files:
  pair=Image.new('RGB',(1800,485),'#171b22');draw=ImageDraw.Draw(pair)
  for i,f in enumerate(files):
   im=Image.open(f).convert('RGB');pair.paste(im.resize((600,450),Image.Resampling.NEAREST),(i*600,35));draw.text((i*600+8,8),f'{scene}: {f.name} | real fallback',fill='white')
  f=root/f'{scene}-hfr-samples.png';pair.save(f);report[str(f)]=hashlib.sha256(f.read_bytes()).hexdigest()
(root/'sheets.json').write_text(json.dumps(report,indent=2)+'\n');print('Wrote labelled sheets; inspect before accepting')
