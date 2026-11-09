#!/usr/bin/env python3
"""Reconstruct the incremental runtime and Windows portability changes, with zero fuzz/offsets."""
import shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
check=ROOT/'work/p413/patch-check'
if check.exists():shutil.rmtree(check)
(check/'runtime').mkdir(parents=True)
shutil.copytree(ROOT/'work/p413/baseline',check/'runtime',dirs_exist_ok=True)
patches=['p413-rewind.patch','p413-speed-turbo.patch','p413-training.patch','p413-input-files.patch']
for name in patches:
 r=subprocess.run(['patch','--batch','--force','-p1','-d',str(check)],input=(ROOT/'port/runtime-patches'/name).read_bytes(),capture_output=True,check=True)
 text=r.stdout.decode()+r.stderr.decode();assert 'fuzz' not in text and 'offset' not in text,text
 print(name,'zero fuzz/offset',flush=True)
def compare(a,b):
 files={x.relative_to(a) for x in a.rglob('*') if x.is_file() and x.suffix!='.orig'}|{x.relative_to(b) for x in b.rglob('*') if x.is_file() and x.suffix!='.orig'}
 for f in files:assert (a/f).read_bytes()==(b/f).read_bytes(),f
 return len(files)
# Baseline contains src/include/tests/runtime.cmake, not vendored third-party files.
count=0
for name in ['src','include','tests']:
 count+=compare(check/'runtime'/name,ROOT/'tools/saturn-recomp/runtime'/name)
assert (check/'runtime/runtime.cmake').read_bytes()==(ROOT/'tools/saturn-recomp/runtime/runtime.cmake').read_bytes()
print('Runtime reconstructs',count+1,'files byte for byte')
wcheck=ROOT/'work/p413/windows-patch-check'
if wcheck.exists():shutil.rmtree(wcheck)
shutil.copytree(ROOT/'work/win/p413-baseline',wcheck)
r=subprocess.run(['patch','--batch','--force','-p1','-d',str(wcheck)],input=(ROOT/'port/runtime-patches/p412-windows-portability.patch').read_bytes(),capture_output=True,check=True)
text=r.stdout.decode()+r.stderr.decode();assert 'fuzz' not in text and 'offset' not in text,text
count=sum(compare(wcheck/name,ROOT/'work/win/saturn-recomp'/name) for name in ['runtime','port'])
print('Windows portability reconstructs',count,'files byte for byte, zero fuzz/offset')
