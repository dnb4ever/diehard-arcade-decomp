#!/usr/bin/env python3
"""Export the incremental P4.24 runtime change from the preserved P4.22 baseline."""
from pathlib import Path
import difflib
import hashlib
import json
root=Path(__file__).resolve().parent.parent
before=root/'work/p424/baseline/runtime'
after=root/'tools/saturn-recomp/runtime'
paths={p.relative_to(d) for d in [before,after] for p in d.rglob('*')
       if p.is_file() and p.suffix not in ('.orig','.rej') and '__pycache__' not in p.parts}
patch=[];changed={}
for rel in sorted(paths):
 a=(before/rel).read_bytes() if (before/rel).exists() else b''
 b=(after/rel).read_bytes() if (after/rel).exists() else b''
 if a==b:continue
 patch.extend(difflib.unified_diff(a.decode().splitlines(True),b.decode().splitlines(True),
   fromfile='a/runtime/'+str(rel) if a else '/dev/null',tofile='b/runtime/'+str(rel) if b else '/dev/null'))
 changed[str(rel)]=hashlib.sha256(b).hexdigest()
(root/'port/runtime-patches/p424-vsync.patch').write_text(''.join(patch))
(root/'work/p424/patch-report.json').write_text(json.dumps(changed,indent=2)+'\n')
print('Exported',len(changed),'runtime files')
