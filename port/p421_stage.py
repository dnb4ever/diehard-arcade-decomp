#!/usr/bin/env python3
"""Reproduce integration from the retained snapshot without touching shared sources."""
import argparse,json,hashlib,shutil,subprocess
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file())
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',default='work/p421/snapshot');p.add_argument('--destination',required=True);a=p.parse_args()
s=ROOT/a.snapshot;d=ROOT/a.destination
assert s.is_dir() and not d.exists(),'snapshot must exist; destination must be new'
d.mkdir(parents=True)
for source,dest in [(s/'runtime',d/'runtime'),(s/'port',d/'port'),(s/'converge-generated',d/'generated')]:shutil.copytree(source,dest,symlinks=True)
for name in ('runtime','convergence','harness'):
 with (ROOT/f'port/runtime-patches/p421-mod-{name}.patch').open() as f:
  r=subprocess.run(['patch','--batch','--fuzz=0','-p1','-d',str(d)],stdin=f,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,check=True)
  assert 'offset' not in r.stdout.lower() and 'fuzz' not in r.stdout.lower(),r.stdout
shutil.copytree(ROOT/'port/mods',d/'port/mods',dirs_exist_ok=True)
for f in (ROOT/'port').glob('p421_*'):
 if f.is_file():shutil.copy2(f,d/'port'/f.name)
report=json.loads((ROOT/'work/p421/patch-report.json').read_text())
for path,h in report['changed_files'].items():assert hashlib.sha256((d/path).read_bytes()).hexdigest()==h,path
print('Staged',d.relative_to(ROOT),'; build with P421_WORK='+str(d.relative_to(ROOT))+' bash port/p421_build.sh')
