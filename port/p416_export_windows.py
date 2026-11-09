#!/usr/bin/env python3
"""Re-export the Windows-only delta onto the P4.16 portable sources."""
import difflib,shutil,subprocess
from pathlib import Path
R=Path(__file__).resolve().parent.parent
out=R/'work/p416/windows-export';out.mkdir(parents=True,exist_ok=True)
files=['runtime/runtime.cmake','runtime/src/host/windows/saturn.manifest','runtime/src/host/windows/saturn.rc','runtime/src/sound/sound.cpp','port/main_stv.cpp','port/stv.cmake']
old=out/'old';win=out/'old-windows';new=out/'current';after=out/'windows'
for dest in [old,win,new,after]:
 if dest.exists():shutil.rmtree(dest)
 dest.mkdir()
for name in files:
 source=R/'work/p416/before'/name if name.startswith('runtime/') else R/'work/p416/before'/Path(name).name
 if source.exists():
  p=old/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,p)
shutil.copytree(old,win,dirs_exist_ok=True)
subprocess.run(['patch','--batch','-p1','-d',str(win)],input=(R/'work/p416/p412-before.patch').read_bytes(),check=True,capture_output=True)
patch=[]
for name in files:
 current=R/('tools/saturn-recomp/'+name if name.startswith('runtime/') else name)
 a=new/name;b=after/name
 a.parent.mkdir(parents=True,exist_ok=True);b.parent.mkdir(parents=True,exist_ok=True)
 if current.exists():shutil.copy2(current,a)
 if (old/name).exists():
  merged=subprocess.run(['git','merge-file','-p',str(current),str(old/name),str(win/name)],capture_output=True)
  if merged.returncode:
   (out/'conflict.txt').write_bytes(merged.stdout);raise SystemExit('Windows merge needs attention: '+name)
  b.write_bytes(merged.stdout)
 else:shutil.copy2(win/name,b)
 x=a.read_text().splitlines(True) if a.exists() else [];y=b.read_text().splitlines(True)
 patch.extend(difflib.unified_diff(x,y,fromfile='a/'+name if a.exists() else '/dev/null',tofile='b/'+name))
(R/'port/runtime-patches/p412-windows-portability.patch').write_text(''.join(patch))
print('Windows portability patch refreshed:',len(patch),'lines')
