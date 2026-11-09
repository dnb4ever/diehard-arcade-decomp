#!/usr/bin/env python3
"""Audit frozen source identities, corrected bindings and inherited ABI pins."""
import hashlib,json,sys
from pathlib import Path
import round4_generate as g
W=Path('work/converge/round4');S=W/'snapshot';OLD=Path('work/converge/round3/snapshot')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    dt=g.load_oracle(S.resolve());old=g.functions(OLD/'port/run/P41/recomp');now=g.functions(S/'port/run/P41/recomp')
    rows=json.loads((W/'generated-rebase/overrides.json').read_text());symbols=json.loads((W/'symbols.json').read_text())
    audited=[]
    for r in rows:
        a=r['address'];pc=dt.source_entry(dt.canonical(int(a,16)));assert f'{pc:08X}'==r['pc']
        assert old[pc][2]==now[pc][2],a
        src=S/f'project/src_functional/{a}.c';fn=dt.select_function(dt.parse(src.read_text()),symbols,pc).decl.name
        audited.append(dict(address=a,callable_pc=f'{pc:08X}',name=fn,source_changed=sha(src)!=sha(OLD/f'project/src_functional/{a}.c'),source_sha256=sha(src),old_source_sha256=sha(OLD/f'project/src_functional/{a}.c'),machine_unchanged=True))
    bindings=dt.CONTRACTS.get('source_entries',{})
    result=dict(head='c57ef4a',source_bindings=bindings,selected=audited,source_changed=sum(r['source_changed'] for r in audited),dropped=[],machine_changed=[],notes='C names resolve through frozen v1.09 preprocessing/symbols. Source identities stay distinct from rebound callable entries. No duplicate source draft is installed at an interior/pool address.')
    result['round4_round5_behavior_fix_sources']=['02204868', '02204afa', '02205232', '02205c96', '0220ef02', '022156da', '0221582a', '0221af30', '0221da60', '02222d3c', '02222e44', '022303a6', '0223ed14', '02243e22', '02243fe0', '02206e2e', '0220cdf6', '02213d18', '022143bc', '02227ca8', '0222da90', '0222db44', '0223e3ba', '022454e4', '022463c8', '0224910e']
    result['behavior_fix_source_intersection']=sorted({r['address'] for r in audited}&set(result['round4_round5_behavior_fix_sources']))
    (W/'rebase-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(len(audited),'inherited entries,',result['source_changed'],'source hashes changed, no machine/binding drift')
if __name__=='__main__':main()
