#!/usr/bin/env python3
"""Export P4.23 experiment and exact committed-snapshot reconstruction patches."""
from pathlib import Path
import difflib, hashlib, json, subprocess, shutil
root=Path('work/p423');report={}
for name,baseline,current,prefix in [
 ('runtime-prototype',root/'runtime-base',root/'saturn-recomp/runtime','runtime'),
 ('harness-prototype',root/'port-base',root/'port','port'),
 ('runtime-snapshot',root/'runtime-committed/runtime',root/'saturn-recomp/runtime','runtime'),
 ('harness-snapshot',root/'committed/port',root/'port','port')]:
 files=(['main_stv.cpp','stv.cmake'] if prefix=='port' else
        [str(p.relative_to(current)) for p in current.rglob('*') if p.is_file() and
         p.suffix in ('.cpp','.h','.c','.inc','.cmake','.txt') and not p.name.endswith('.orig') and
         (not (baseline/p.relative_to(current)).exists() or p.read_bytes()!=(baseline/p.relative_to(current)).read_bytes())])
 patch='';check=root/'patch-check'/name
 for f in sorted(files):
  old=baseline/f;new=current/f
  a=old.read_text() if old.exists() else '';b=new.read_text()
  patch+=''.join(difflib.unified_diff(a.splitlines(True),b.splitlines(True),fromfile=f'a/{prefix}/{f}' if old.exists() else '/dev/null',tofile=f'b/{prefix}/{f}'))
  target=check/prefix/f;target.parent.mkdir(parents=True,exist_ok=True)
  if old.exists():shutil.copy2(old,target)
  else:target.unlink(missing_ok=True)
 out=Path(f'port/runtime-patches/p423-{name}.patch');out.write_text(patch)
 p=subprocess.run(['patch','--batch','--fuzz=0','-p1','-d',str(check)],input=patch,text=True,capture_output=True)
 assert p.returncode==0 and not any(x in p.stdout.lower() for x in ['offset','fuzz']),p.stdout+p.stderr
 for f in files:assert (check/prefix/f).read_bytes()==(current/f).read_bytes()
 report[name]={'baseline':str(baseline),'patch':str(out),'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'files':sorted(files),'reconstructed':True,'patch_log':p.stdout}
(root/'patches.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS four patches reconstruct exact bytes, zero offsets/fuzz')
