#!/usr/bin/env python3
"""Export P4.16 runtime delta against the design-time P4.15 source snapshot."""
import difflib
from pathlib import Path
R=Path(__file__).resolve().parent.parent
before=R/'work/p416/before/runtime';after=R/'tools/saturn-recomp/runtime'
files={p.relative_to(before) for p in before.rglob('*') if p.is_file() and p.suffix!='.orig'}|{p.relative_to(after) for p in after.rglob('*') if p.is_file() and p.suffix!='.orig'}
patch=[];changed=[]
for name in sorted(files):
 a=before/name;b=after/name
 if a.exists() and b.exists() and a.read_bytes()==b.read_bytes():continue
 x=a.read_text().splitlines(True) if a.exists() else [];y=b.read_text().splitlines(True) if b.exists() else []
 patch.extend(difflib.unified_diff(x,y,fromfile='a/runtime/'+str(name) if a.exists() else '/dev/null',tofile='b/runtime/'+str(name) if b.exists() else '/dev/null'))
 changed.append(str(name))
(R/'port/runtime-patches/p416-netplay.patch').write_text(''.join(patch))
print(len(changed),'runtime files,',len(patch),'patch lines');print('\n'.join(changed))
