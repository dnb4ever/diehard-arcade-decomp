#!/usr/bin/env python3
"""Rebase the existing isolated Windows delta without changing its edits."""
import difflib, shutil, subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent
out=root/'work/p419/windows-export';before=out/'before';after=out/'after'
files=['runtime/runtime.cmake','runtime/src/host/windows/saturn.manifest','runtime/src/host/windows/saturn.rc','runtime/src/sound/sound.cpp','port/main_stv.cpp','port/stv.cmake']
for dest in [before,after]:
 if dest.exists():shutil.rmtree(dest)
 dest.mkdir(parents=True)
 for f in files:
  source=root/('tools/saturn-recomp/'+f if f.startswith('runtime/') else f)
  if source.exists():
   target=dest/f;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
patch=root/'port/runtime-patches/p412-windows-portability.patch'
original=patch.read_text()
applied=subprocess.run(['patch','--batch','--fuzz=0','-p1','-d',str(after)],input=original.encode(),capture_output=True)
if applied.returncode:
 for f in files:
  base=root/'work/win/p413-baseline'/f; windows=root/'work/win/saturn-recomp'/f; current=before/f; target=after/f
  if not base.exists():
   if windows.exists():shutil.copy2(windows,target)
  elif current.exists():
   merged=subprocess.run(['git','merge-file','-p',str(current),str(base),str(windows)],capture_output=True)
   if merged.returncode:
    (out/'conflict.txt').write_bytes(merged.stdout);raise SystemExit('Windows rebase conflict: '+f)
   target.write_bytes(merged.stdout)
new=''
for f in files:
 a=(before/f).read_text() if (before/f).exists() else ''
 b=(after/f).read_text() if (after/f).exists() else ''
 if a!=b:new+=''.join(difflib.unified_diff(a.splitlines(True),b.splitlines(True),fromfile='a/'+f if a else '/dev/null',tofile='b/'+f))
def edits(s):return [l for l in s.splitlines() if l.startswith(('+','-')) and not l.startswith(('+++','---'))]
assert edits(original)==edits(new),'Windows patch changed beyond context'
patch.write_text(new)
print('Windows delta re-exported; added/removed lines identical')
