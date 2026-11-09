#!/usr/bin/env python3
"""Read-only live prefix comparison for the ending replays."""
from pathlib import Path
import json,re,hashlib
w=Path('work/converge/round3/runs')
a=w/'stock-fullgame/fullgame';b=w/'final-fullgame/fullgame'
def status(p):
 log=(p/'run.log').read_text();vbs=re.findall(r'(?:VB |VB=)(\d+)',log);scenes=re.findall(r'scene=(\d+)',log)
 return dict(vblank=int(vbs[-1]) if vbs else 0,scene=int(scenes[-1]) if scenes else None,failed='CONVERGE FAIL' in log or 'FATAL' in log)
result=dict(stock=status(a),override=status(b));compared=ram=0;bad=[]
if (a/'frames.jsonl').exists() and (b/'frames.jsonl').exists():
 with (a/'frames.jsonl').open() as x,(b/'frames.jsonl').open() as y:
  for left,right in zip(x,y):
   if not left.endswith('\n') or not right.endswith('\n'):break
   compared+=1;row=json.loads(left);ram+=bool(row['ram'])
   if left!=right and len(bad)<4:bad.append(dict(stock=row,override=json.loads(right)))
png=[];png_bad=[]
for p in sorted(b.glob('shot-*.png')):
 q=a/p.name
 if q.exists():
  png.append(p.name)
  if hashlib.sha256(p.read_bytes()).digest()!=hashlib.sha256(q.read_bytes()).digest():png_bad.append(p.name)
result.update(common_records=compared,common_ram_hashes=ram,mismatches=bad,common_pngs=len(png),png_mismatches=png_bad)
print(json.dumps(result,indent=2))
