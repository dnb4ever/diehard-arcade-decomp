#!/usr/bin/env python3
"""Assert pilot evidence and export a compact, portable convergence manifest."""
import argparse
import hashlib
import json
import os
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
WORK=Path('work/converge')

def load(path):return json.loads(path.read_text())
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--include-autoplay',action='store_true')
    a=p.parse_args();os.chdir(ROOT)
    scenarios={};functions=load(WORK/'generated-pilot/overrides.json')
    for fn in functions:fn['calls']={}
    for name in ('nocoin','gameover','combat','legacy-combat'):
        stock_label='stock' if name=='combat' else 'stock-long'
        stock=load(WORK/'runs'/stock_label/name/'result.json')
        pilot=load(WORK/'runs/pilot'/name/'result.json')
        for key in ('png','trace_sha256','frame_count','frames_sha256'):
            assert stock[key]==pilot[key],(name,key)
        assert not stock['exit_code'] and not pilot['exit_code'] and pilot['audit']
        scenarios[name]={k:pilot[k] for k in ('frame_count','frames_sha256',
            'pixel_stream_sha256','ram_stream_sha256','trace_sha256','png')}
        scenarios[name].update(stock_seconds=stock['seconds'],pilot_seconds=pilot['seconds'])
        for fn in functions:fn['calls'][name]=sum(pilot['counts'].get(str(fn['index']),[0,0]))
    extras={}
    if a.include_autoplay:
        s=load(WORK/'runs/stock-autoplay/autoplay/result.json')
        r=load(WORK/'runs/pilot-autoplay/autoplay/result.json')
        assert not s['exit_code'] and not r['exit_code']
        for key in ('png','trace_sha256'):assert s[key]==r[key],key
        extras['autoplay']={k:r[k] for k in ('png','trace_sha256')}
        for fn in functions:fn['calls']['autoplay']=sum(r['counts'].get(str(fn['index']),[0,0]))
    ten=load(WORK/'runs/pilot10/combat/result.json')
    stock=load(WORK/'runs/stock/combat/result.json')
    for key in ('png','trace_sha256','frame_count','frames_sha256'):assert ten[key]==stock[key],key
    final_path=WORK/'runs/pilot-delivery/combat/result.json'
    if final_path.exists():
        final=load(final_path)
        for key in ('png','trace_sha256','frame_count','frames_sha256'):assert final[key]==stock[key],key
        assert final['binary_sha256']==sha(WORK/'build-pilot/saturn_stv')
        extras['delivery_combat']={k:final[k] for k in ('binary_sha256','frame_count','frames_sha256','trace_sha256')}
    for fn in functions:fn['total_calls']=sum(fn['calls'].values())
    old=load(WORK/'snapshot/port/run/P48/p410-final/manifest.json')
    historical={}
    for name,r in scenarios.items():
        historical[name]=dict(different_png=[k for k,v in r['png'].items() if old[name]['png'][k]['sha256']!=v],
            trace_matches=old[name]['trace_sha256']==r['trace_sha256'])
    frozen=load(WORK/'snapshot.sha256.json')
    for name,want in frozen.items():
        assert sha(WORK/'snapshot'/name)==want,('snapshot drift',name)
    certificate=load(HERE/'qualification.json')
    assert len(certificate['functions'])==len(functions)==41
    report=dict(function_count=len(functions),direct_count=sum(f['bridge']=='direct' for f in functions),
        reached_functions=sum(bool(f['total_calls']) for f in functions),
        calls=sum(f['total_calls'] for f in functions),
        compared_frames=sum(s['frame_count'] for s in scenarios.values()),
        compared_reference_pngs=sum(len(s['png']) for s in scenarios.values()),
        snapshot_manifest_sha256=sha(WORK/'snapshot.sha256.json'),
        qualification_sha256=sha(HERE/'qualification.json'),
        delivery_binary_sha256={variant:sha(WORK/f'build-{variant}/saturn_stv') for variant in ('stock','pilot','mod')},
        scenarios=scenarios,extras=extras,pilot10=ten,
        historical_p410_comparison=historical,functions=functions)
    dest=HERE/'evidence/equivalence.json';dest.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {report["compared_frames"]} frames, {report["compared_reference_pngs"]} PNGs, '
          f'{report["reached_functions"]}/{len(functions)} overrides reached, {report["calls"]} C calls')

if __name__=='__main__':main()
