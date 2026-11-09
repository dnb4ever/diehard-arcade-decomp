#!/usr/bin/env python3
"""Recheck the frozen override list with the original difftest v1.06 oracle."""
import argparse
import hashlib
import json
from pathlib import Path
from generate import load_oracle

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vectors',type=int,default=64)
    p.add_argument('--jobs',type=int,choices=range(1,5),default=4)
    p.add_argument('--overrides',type=Path,default=Path(__file__).resolve().parent/'overrides.txt')
    a=p.parse_args()
    root=Path(__file__).resolve().parents[2]
    here=Path(__file__).resolve().parent
    snapshot=root/'work/converge/snapshot'
    oracle=load_oracle(snapshot)
    cache=snapshot.parent/'symbols.json'
    if cache.exists():oracle.load_symbols=lambda:json.loads(cache.read_text())
    addresses=[line.split('#')[0].strip() for line in a.overrides.read_text().splitlines()
               if line.split('#')[0].strip()]
    dest=snapshot.parent/'qualification';dest.mkdir(exist_ok=True)
    report=dest/'report.json'
    code=oracle.main(addresses+['--vectors',str(a.vectors),'--jobs',str(a.jobs),
        '--output-dir',str(dest),'--json',str(report)])
    data=json.loads(report.read_text())
    if code or any(row['status']!='PASS' for row in data['results']):
        raise SystemExit('Override list contains an unqualified function; see '+str(report))
    # This small certificate pins each opted-in readable source, not the live tree.
    certificate=dict(version=data['version'],vectors=a.vectors,
        harness_sha256=data['harness_sha256'],cart_sha256=data['cart_sha256'],
        functions={row['address']:dict(status=row['status'],vectors=row['vectors'],
            source_sha256=row['source_sha256'],branch_coverage=row['branch_coverage'])
            for row in data['results']})
    (here/'qualification.json').write_text(json.dumps(certificate,indent=2)+'\n')

if __name__=='__main__':main()
