#!/usr/bin/env python3
"""Export and reconstruct only the P4.20 snapshot edits; shared runtime is read-only."""
from pathlib import Path
import difflib,hashlib,json,subprocess,shutil
root=Path('work/p420');report={}
for name,baseline,current,prefix in [
 ('runtime',root/'runtime-base/runtime',root/'saturn-recomp/runtime','runtime'),
 ('harness',root/'port-base',root/'port','port')]:
 files=['src/video/vdp1.cpp','src/video/vdp1_raster.inc','src/video/vdp2.cpp','src/video/video.cpp','src/video/video.h','src/host/host.cpp'] if name=='runtime' else ['main_stv.cpp','stv.cmake']
 patch='';check=root/'patch-check'/name
 for f in files:
  a=(baseline/f).read_text();b=(current/f).read_text()
  patch+=''.join(difflib.unified_diff(a.splitlines(True),b.splitlines(True),fromfile=f'a/{prefix}/{f}',tofile=f'b/{prefix}/{f}'))
  target=check/prefix/f;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(baseline/f,target)
 out=Path(f'port/runtime-patches/p420-{name}-prototype.patch');out.write_text(patch)
 p=subprocess.run(['patch','--batch','--fuzz=0','-p1','-d',str(check)],input=patch,text=True,capture_output=True)
 assert p.returncode==0 and not any(x in p.stdout.lower() for x in ['offset','fuzz']),p.stdout+p.stderr
 for f in files:assert (check/prefix/f).read_bytes()==(current/f).read_bytes()
 report[name]={'patch':str(out),'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'files':files,'reconstructed':True,'patch_log':p.stdout}
(root/'patches.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS exact snapshot patch reconstruction, no offset/fuzz')
