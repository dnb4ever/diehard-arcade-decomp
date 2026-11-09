#!/usr/bin/env python3
"""Export an incremental runtime-only patch against an explicitly saved baseline."""
import argparse, difflib, shutil
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline',required=True)
p.add_argument('--patch',required=True)
p.add_argument('--snapshot')
p.add_argument('--source',default='tools/saturn-recomp/runtime')
a=p.parse_args()
before=Path(a.baseline);after=Path(a.source)
def inventory(root):
 return {x.relative_to(root) for folder in ['src','tests'] for x in (root/folder).rglob('*') if x.is_file() and x.suffix!='.orig'}|{Path('runtime.cmake')}
patch=''
for f in sorted(inventory(before)|inventory(after)):
 old=(before/f).read_text() if (before/f).exists() else ''
 new=(after/f).read_text() if (after/f).exists() else ''
 if old!=new:patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/runtime/'+str(f) if old else '/dev/null',tofile='b/runtime/'+str(f)))
Path(a.patch).write_text(patch)
if a.snapshot:
 dest=Path(a.snapshot);dest.mkdir(parents=True,exist_ok=True)
 for name in ['src','tests']:
  shutil.copytree(after/name,dest/name,dirs_exist_ok=True,ignore=shutil.ignore_patterns('*.orig'))
 shutil.copy2(after/'runtime.cmake',dest/'runtime.cmake')
print(a.patch,len(patch),'bytes')
