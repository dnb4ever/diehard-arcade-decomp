#!/usr/bin/env python3
"""Inventory local US ST-V firmware and optionally recompile it for P4.10 investigation.

This emits evidence only; it never changes or launches the native game/runtime.
"""
import argparse, hashlib, json, os, struct, subprocess, zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--generate',action='store_true',help='also regenerate the isolated ROM-only static recompile')
a=p.parse_args();root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p410/bios');out.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile('diehard.zip')as z:raw=z.read('epr-17952a.ic8')
assert len(raw)==0x80000
b=bytearray(raw);b[::2],b[1::2]=raw[1::2],raw[::2]
(out/'us.bin').write_bytes(b)
r=dict(member='epr-17952a.ic8',size=len(b),raw_sha256=hashlib.sha256(raw).hexdigest(),swapped_sha256=hashlib.sha256(b).hexdigest(),ipl_copies=[])
for at in range(0x4b4,0x4e4,12):
 words,dest,src=struct.unpack_from('>III',b,at)
 r['ipl_copies'].append(dict(table=f'{at:08X}',source=f'{src:08X}',destination=f'{dest:08X}',bytes=words*4))
if a.generate:
 cmd=['python3','-m','saturnrecomp.recomp','--out','../../work/p410/bios/recomp','usbios=../../work/p410/bios/us.bin@00000000+00000200,000032CE,0000413A,00004704,00008000']
 with (out/'recompile.log').open('w')as f:
  subprocess.run(cmd,cwd='tools/saturn-recomp',env=dict(os.environ,PYTHONPATH='src'),stdout=f,stderr=subprocess.STDOUT,check=True)
 report=(out/'recomp/report.txt').read_text()
 row=next(x.split()for x in report.splitlines()if x.startswith('usbios '))
 r['recompile']=dict(functions=int(row[1]),instructions=int(row[2]),unresolved_calls=int(row[8]),unresolved_jumps=int(row[9]),non_entry_targets=int(row[13]),outside_targets=int(row[15]))
 r['command']=cmd
(out/'inventory.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r,indent=2))
