#!/usr/bin/env python3
"""Export incremental P4.13 runtime patches from explicit local source snapshots."""
import argparse,difflib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('before',type=Path);p.add_argument('after',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
files={x.relative_to(d) for d in [a.before,a.after] for x in d.rglob('*') if x.is_file() and x.suffix!='.orig'}
chunks=[]
for f in sorted(files):
 old=a.before/f;new=a.after/f
 x=old.read_text().splitlines(True) if old.exists() else []
 y=new.read_text().splitlines(True) if new.exists() else []
 if x!=y:chunks.extend(difflib.unified_diff(x,y,fromfile='a/runtime/'+str(f) if old.exists() else '/dev/null',tofile='b/runtime/'+str(f) if new.exists() else '/dev/null'))
a.output.write_text(''.join(chunks))
print(a.output,len(chunks),'lines')
