#!/usr/bin/env python3
"""Rebase the existing Windows-only portability delta onto the P4.14 source tree."""
import difflib,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
out=ROOT/'work/p414/windows-export';out.mkdir(parents=True,exist_ok=True)
base=out/'base';after=out/'after'
files=['runtime/runtime.cmake','runtime/src/host/windows/saturn.manifest','runtime/src/host/windows/saturn.rc','runtime/src/sound/sound.cpp','port/main_stv.cpp','port/stv.cmake']
oldbase=out/'previous-base';oldafter=out/'previous-windows'
if not oldafter.exists():
 for name in files:
  target=oldbase/name;target.parent.mkdir(parents=True,exist_ok=True)
  if name.startswith('port/'):
   target.write_bytes(subprocess.check_output(['git','show','c60168b:'+name],cwd=ROOT))
  else:
   source=ROOT/'work/p414/before'/name
   if source.exists():shutil.copy2(source,target)
 shutil.copytree(oldbase,oldafter)
 subprocess.run(['patch','--batch','--forward','-p1','-d',str(oldafter)],input=(ROOT/'work/p414/p412-before.patch').read_bytes(),check=True,capture_output=True)
for name in files:
 current=ROOT/('tools/saturn-recomp/'+name if name.startswith('runtime/') else name)
 for dest in [base,after]:
  target=dest/name;target.parent.mkdir(parents=True,exist_ok=True)
  if current.exists():shutil.copy2(current,target)
  elif target.exists():target.unlink()
 previous=oldbase/name;patched=oldafter/name
 if not patched.exists():raise SystemExit('Missing previous Windows source '+str(patched))
 if previous.exists():
  result=subprocess.run(['git','merge-file','-p',str(current),str(previous),str(patched)],capture_output=True)
  if result.returncode:
   conflict=out/(Path(name).name+'.merge');conflict.write_bytes(result.stdout)
   if name=='port/main_stv.cpp':
    merged=result.stdout.decode()
    import re
    def resolve(m):
     current_side,windows_side=m.group(1),m.group(2)
     assert 'int scale = 0' in current_side and 'if (packaged) window = launcher = true;' in windows_side,m.group()
     return '#ifdef _WIN32\n    if (packaged) window = launcher = true;\n#endif\n'+current_side
    result.stdout=re.sub(r'<<<<<<<[^\n]*\n(.*?)=======\n(.*?)>>>>>>>[^\n]*\n',resolve,merged,flags=re.S).encode()
   else:raise SystemExit('Windows merge conflict: '+str(conflict))
  (after/name).write_bytes(result.stdout)
 else:shutil.copy2(patched,after/name)
chunks=[]
for name in files:
 a=base/name;b=after/name
 x=a.read_text().splitlines(True) if a.exists() else [];y=b.read_text().splitlines(True)
 if x!=y:chunks.extend(difflib.unified_diff(x,y,fromfile='a/'+name if a.exists() else '/dev/null',tofile='b/'+name))
patch=ROOT/'port/runtime-patches/p412-windows-portability.patch'
patch.write_text(''.join(chunks));print(patch,len(chunks),'lines')
