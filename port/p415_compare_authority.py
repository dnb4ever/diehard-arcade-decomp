#!/usr/bin/env python3
"""Compare unmasked guest fields and all machine-state payload bytes before/after."""
import argparse,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('before');p.add_argument('after');a=p.parse_args()
root=Path(__file__).resolve().parent.parent;out=root/'work/p415/authority';report={}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def parts(path):
 b=path.read_bytes();assert b[:8]==b'STVQUICK';at=48;result={}
 names=['machine','video','core','mmio','bios','stv','player','scu','scudsp','onchip','clock','smpc','stv-smpc','cdblock','cddrive','cdfs','cdpos','vdp1','vdp2','sound','scsp','scspdsp']
 for name in names:
  assert b[at:at+len(name)]==name.encode(),(name,at)
  at+=len(name);n=struct.unpack_from('<Q',b,at)[0];at+=8;result[name]=b[at:at+n];at+=n
 assert at==len(b);return result
for scale in [1,2,3]:
 before=out/a.before/f'authority-{scale}x';after=out/a.after/f'authority-{scale}x'
 x=json.loads((before/'report.json').read_text());y=json.loads((after/'report.json').read_text())
 assert x['rows']==y['rows'],('full RAM/semantic fields',scale,next((u['frame'] for u,v in zip(x['rows'],y['rows']) if u!=v),None))
 assert x['wav']==y['wav'],('WAV',scale)
 for vb in [1800,1900,2100]:assert sha(before/f'shot-{vb}.png')==sha(after/f'shot-{vb}.png'),('PNG',scale,vb)
 for vb in [1800,2100]:assert sha(before/f'dump-{vb}.bin')==sha(after/f'dump-{vb}.bin'),('hardware',scale,vb)
 s,t=parts(before/'state.quickstate'),parts(after/'state.quickstate')
 for name in s:assert s[name]==t[name],('snapshot',scale,name)
 report[str(scale)]=dict(fields=len(x['rows']),ram_and_semantics=True,wav=True,pngs=[1800,1900,2100],hardware_dumps=[1800,2100],snapshot_parts=list(s))
 print(scale,'PASS',len(x['rows']),'unmasked RAM/semantic fields, PNGs, WAV, hardware dumps, all 22 machine-state parts',flush=True)
(out/f'{a.before}-vs-{a.after}.json').write_text(json.dumps(report,indent=2)+'\n')
