#!/usr/bin/env python3
"""Export only P4.21 additions and diffs against the one immutable snapshot."""
import difflib,hashlib,json,shutil,subprocess
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file());W=ROOT/'work/p421';S=W/'snapshot'
changed={};patches={}
for name,old,new in [('runtime',S/'runtime',W/'runtime'),('convergence',S/'converge-generated',W/'generated'),('harness',S/'port',W/'port')]:
 paths=sorted(set(p.relative_to(old) for p in old.rglob('*') if p.is_file() and not p.is_symlink())|set(p.relative_to(new) for p in new.rglob('*') if p.is_file() and not p.is_symlink()))
 lines=[];included=[]
 for rel in paths:
  # New SDK/examples/helpers are first-class additions, not diffs to live files.
  if name=='harness' and rel!=Path('main_stv.cpp'):continue
  before=old/rel;after=new/rel
  if before.exists() and after.exists() and before.read_bytes()==after.read_bytes():continue
  prefix='generated' if name=='convergence' else 'port' if name=='harness' else 'runtime'
  lines+=list(difflib.unified_diff(before.read_text().splitlines(True) if before.exists() else [],after.read_text().splitlines(True) if after.exists() else [],fromfile='a/'+prefix+'/'+str(rel) if before.exists() else '/dev/null',tofile='b/'+prefix+'/'+str(rel) if after.exists() else '/dev/null'))
  included.append(str(rel));changed[prefix+'/'+str(rel)]=hashlib.sha256(after.read_bytes()).hexdigest()
 dest=ROOT/'port/runtime-patches'/f'p421-mod-{name}.patch';assert not dest.exists() or dest.read_text()==''.join(lines),'refusing to overwrite changed delivery patch'
 dest.write_text(''.join(lines));patches[str(dest.relative_to(ROOT))]={'files':included,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()}
# Copy only new paths; never overwrite an unrelated file in shared port/.
for source in [p for p in (W/'port/mods').rglob('*') if p.is_file()]+[p for p in (W/'port').glob('p421_*') if p.is_file() and p.name!='p421_integrate.py']:
 dest=ROOT/'port'/source.relative_to(W/'port');dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists():assert dest.read_bytes()==source.read_bytes(),f'existing destination differs: {dest}'
 else:shutil.copy2(source,dest)
# Reconstruct every patch in a separate review layout; require no fuzz/offset.
review=W/'patch-review';assert not review.exists(),'preserve earlier patch review'
review.mkdir()
for source,dest in [(S/'runtime',review/'runtime'),(S/'port',review/'port'),(S/'converge-generated',review/'generated')]:shutil.copytree(source,dest,symlinks=True)
logs=[]
for patch in patches:
 with (ROOT/patch).open() as f:r=subprocess.run(['patch','--batch','--fuzz=0','-p1','-d',str(review)],stdin=f,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,check=True)
 assert not any(word in r.stdout.lower() for word in ('offset','fuzz','failed')),r.stdout;logs.append(r.stdout)
for path,h in changed.items():assert hashlib.sha256((review/path).read_bytes()).hexdigest()==h,path
(W/'patch-validation.log').write_text('\n'.join(logs))
(W/'patch-report.json').write_text(json.dumps({'patches':patches,'changed_files':changed,'reconstructed':len(changed),'fuzz':0,'offsets':0},indent=2)+'\n')
print('PASS',len(changed),'files reconstructed exactly; only new P4.21 shared paths written')
