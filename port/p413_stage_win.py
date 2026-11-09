#!/usr/bin/env python3
"""Refresh only owned Windows staging from the current runtime; preserve historical P4.12 baseline."""
import hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
WIN=ROOT/'work/win';base=WIN/'p413-baseline';staged=WIN/'saturn-recomp'
names=['main_stv.cpp','stv.cmake','p410_continuations.py','p415_idle_loops.py','p413_roms.c','p413_roms.h','p413_frontend_win.h']
inputs={}
for p in sorted((ROOT/'tools/saturn-recomp/runtime').rglob('*')):
 if p.is_file() and p.suffix!='.orig' and '__pycache__' not in p.parts:inputs[str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
for name in names:
 p=ROOT/'port'/name;inputs['port/'+name]=hashlib.sha256(p.read_bytes()).hexdigest()
if (ROOT/'port/assets').exists():
 for p in sorted((ROOT/'port/assets').glob('p413-*')):
  if p.is_file():inputs[str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
patch=ROOT/'port/runtime-patches/p412-windows-portability.patch'
if patch.exists():inputs[str(patch.relative_to(ROOT))]=hashlib.sha256(patch.read_bytes()).hexdigest()
marker=WIN/'p413-snapshot.json'
if marker.exists() and json.loads(marker.read_text())==inputs:
 print('Using current P4.13 Windows snapshot.');raise SystemExit(0)
for destination in [base,staged]:
 destination.mkdir(parents=True,exist_ok=True)
 for directory in ['runtime','port']:
  target=destination/directory
  if target.exists():shutil.rmtree(target)
 shutil.copytree(ROOT/'tools/saturn-recomp/runtime',destination/'runtime',ignore=shutil.ignore_patterns('*.orig','__pycache__'))
 (destination/'port').mkdir()
 for name in names:shutil.copy2(ROOT/'port'/name,destination/'port'/name)
 if (ROOT/'port/assets').exists():
  (destination/'port/assets').mkdir()
  for p in (ROOT/'port/assets').glob('p413-*'):
   if p.is_file():shutil.copy2(p,destination/'port/assets'/p.name)
# The generated program is copied separately; never run the pass on shared inputs here.
shutil.copytree(ROOT/'port/run/P41/recomp',WIN/'recomp',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__'))
marker.write_text(json.dumps(inputs,indent=2)+'\n')
print('Refreshed P4.13 Windows snapshot; historical work/win/baseline remains intact.')
