#!/usr/bin/env python3
"""Summarize measured midpoints, raw endpoint changes and labelled fallback strips."""
from pathlib import Path
from collections import Counter
from PIL import Image,ImageDraw
import re,json,struct,hashlib
root=Path('work/p423');report={}
for scene,lo,hi in [('fight',901,2100),('floor',901,1500),('attract',901,1500)]:
 d=root/'visual'/scene/'hfr';rows=[]
 for line in (d/'run.log').read_text().splitlines():
  if 'P423_SAMPLE' in line:rows.append({k:float(v) if '.' in v else int(v) for k,v in re.findall(r'(\w+)=([\d.-]+)',line)})
 mid=[r for r in rows if lo<=r['VB']<=hi and r['age']==1 and r['interval']==2]
 summary=dict(window=[lo,hi],presented_fields=hi-lo+1,midpoints=len(mid),matched=sum(r['matched'] for r in mid),genuine=sum(r['genuine'] for r in mid),fallback=sum(not r['matched'] for r in mid),bg_reasons=dict(Counter(r['bg_reason'] for r in mid)),actor_reasons=dict(Counter(r['actor_reason'] for r in mid)))
 summary['genuine_ratio']=summary['genuine']/len(mid) if mid else None
 summary['bg_fields_range']=[min((r['bg_fields'] for r in mid),default=0),max((r['bg_fields'] for r in mid),default=0)]
 diag=[]
 size=0x200+0x1000+0x80000
 for f in sorted(d.glob('bg-*.bin')):
  data=f.read_bytes()
  if len(data)!=size*2:continue
  a,b=data[:size],data[size:];cr0,cr1=a[0x200:0x1200],b[0x200:0x1200]
  pal=[i//2 for i in range(0,0x1000,2) if cr0[i:i+2]!=cr1[i:i+2]]
  reg=lambda data,at,n=2:int.from_bytes(data[at:at+n],'big')
  rpt=(((reg(b,0xbc)&7)<<16|reg(b,0xbe))&~0x41)*2
  diag.append(dict(vb=int(f.stem.split('-')[1]),palette_changed_words=len(pal),palette_first_words=pal[:20],palette_first_changes=[{'entry':i,'old':reg(cr0,i*2),'new':reg(cr1,i*2)} for i in pal[:6]],rpt=rpt,rotation_changed_bytes=sum(x!=y for x,y in zip(a[0x1200+rpt:0x1200+rpt+0x60],b[0x1200+rpt:0x1200+rpt+0x60]))))
 summary['endpoint_diagnostics']=diag
 cmd_diag=[]
 for f in sorted(d.glob('commands-*.bin')):
  data=f.read_bytes();at=0;frames=[]
  for _ in range(2):
   n=struct.unpack_from('<I',data,at)[0];at+=4;draws=[]
   for _ in range(n):
    count=struct.unpack_from('<I',data,at)[0];at+=4;cmds=[]
    for _ in range(count):cmds.append(data[at:at+32]);at+=36
    draws.append(cmds)
   frames.append(draws)
  attrs=lambda c:bytes([c[0]&15,c[1]])+c[4:12]+c[28:32]
  counters=[Counter(attrs(c) for draw in frame for c in draw if (c[1]&15)<=7) for frame in frames]
  shared=counters[0]&counters[1]
  cmd_diag.append(dict(vb=int(f.stem.split('-')[1]),counts=[[len(draw) for draw in frame] for frame in frames],unique_shared_signatures=sum(counters[0][k]==counters[1][k]==1 for k in shared),shared_signatures=len(shared)))
 summary['command_diagnostics']=cmd_diag
 # Three rows of real / presented midpoint (often fallback) / real endpoints.
 chosen=[r for r in rows if r['age']==1 and (d/f"real-{r['VB']-3}.png").exists() and (d/f"sample-{r['VB']}.png").exists()][:3]
 if chosen:
  sheet=Image.new('RGB',(1440,len(chosen)*392),'#171b22');draw=ImageDraw.Draw(sheet)
  for row,r in enumerate(chosen):
   vb=r['VB'];matched=r['matched'];before=vb-3;after=vb-1
   for col,(f,label) in enumerate([(d/f'real-{before}.png',f'previous real VB{before}'),(d/f'sample-{vb}.png',f'VB{vb}: '+('coordinated midpoint' if matched else 'REAL FALLBACK; no interpolation')),(d/f'real-{after}.png',f'next real VB{after}')]):
    im=Image.open(f).convert('RGB').resize((480,360),Image.Resampling.NEAREST);sheet.paste(im,(col*480,row*392+32));draw.text((col*480+6,row*392+8),f'{scene} | {label}',fill='white')
  dst=root/'visual'/f'{scene}-strips.png';sheet.save(dst);summary['strips']=str(dst)
  # Retain raw-coordinate close crops with no interpolation/resampling blur.
  crop=Image.new('RGB',(704*3,160*len(chosen)),'#171b22')
  for row,r in enumerate(chosen):
   for col,f in enumerate([d/f"real-{r['VB']-3}.png",d/f"sample-{r['VB']}.png",d/f"real-{r['VB']-1}.png"]):crop.paste(Image.open(f).convert('RGB').crop((0,80,704,240)),(704*col,160*row))
  crop.save(root/'visual'/f'{scene}-floor-actor-crops.png')
 equal=[]
 for r in rows:
  f=d/f"sample-{r['VB']}.png";real=d/f"real-{r['VB']}.png"
  if f.exists():equal.append(dict(vb=r['VB'],matched=r['matched'],equal=f.read_bytes()==real.read_bytes()))
 summary['sample_equality']=equal
 report[scene]=summary;print(scene,summary['midpoints'],summary['matched'],summary['fallback'],summary['genuine_ratio'])
(root/'coverage.json').write_text(json.dumps(report,indent=2)+'\n')
