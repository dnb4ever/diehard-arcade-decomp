#!/usr/bin/env python3
"""Fail-closed round-4 acceptance, logical/emitted coverage and portable evidence."""
import collections,hashlib,json,re
from pathlib import Path
W=Path('work/converge/round4');HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
NAMES=('nocoin','gameover','combat','legacy-combat')
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clean(x):
    if isinstance(x,str):return x.replace(str(ROOT)+'/', '')
    if isinstance(x,list):return [clean(v) for v in x]
    if isinstance(x,dict):return {clean(k):clean(v) for k,v in x.items()}
    return x
def valid(label,name,binary=None):
    d=W/'runs'/label/name;r=read(d/'result.json');limit=170000 if name=='fullgame' else 9000 if name in ('nocoin','gameover') else 2100
    assert r['exit_code']==0 and not r['errors'] and len(r['png'])==r['expected_shots'] and r['frame_count']==limit,(label,name)
    log=(d/'run.log').read_text();assert not re.search(r'FATAL|CONVERGE FAIL|rejected bus',log)
    records=[json.loads(l) for l in (d/'frames.jsonl').read_text().splitlines()]
    assert [v['frame'] for v in records]==list(range(1,limit+1))
    assert r['frames_sha256']==sha(d/'frames.jsonl')
    if name=='fullgame':
        assert len(r['png'])==34 and [v['frame'] for v in records if v['ram']]==list(range(300,170001,300))
        assert all(not v['pixels'] for v in records)
        for scene in (73,79,80,81):assert re.search(rf'scene={scene} ',log)
    else:assert all(v['pixels'] and v['ram'] for v in records)
    if binary:assert r['binary_sha256']==sha(binary),(label,'wrong executable')
    return r
def compare(a,b):
    for k in ('png','trace_sha256','frames_sha256','frame_count'):assert a[k]==b[k],k
def main():
    pins=read(W/'snapshot.sha256.json');assert all(sha(W/'snapshot'/p)==v for p,v in pins.items())
    for label,folder in [('rebase','generated-rebase'),('final','generated')]:
        assert all(sha(W/folder/p)==h for p,h in read(W/f'generated-{label}.sha256.json').items())
    # Observer-only runtime copy must still be exactly the recorded source plus sampling edit.
    checked=0
    for p in (W/'snapshot/tools/saturn-recomp/runtime').rglob('*'):
        if not p.is_file() or p.is_symlink():continue
        relative=p.relative_to(W/'snapshot/tools/saturn-recomp/runtime');expected=p.read_bytes()
        if str(relative)=='src/video/video.cpp':
            a=b'return !value || std::strcmp(value, "0");';b=b'const char* interval = std::getenv("CONVERGE_RAM_INTERVAL");\n    unsigned every = interval ? std::strtoul(interval, nullptr, 10) : 1;\n    return (!value || std::strcmp(value, "0")) && (!every || g_vblanks % every == 0);'
            assert expected.count(a)==1;expected=expected.replace(a,b)
        assert (W/'capture-runtime'/relative).read_bytes()==expected,relative
        checked+=1
    for p,h in read(W/'perf-deps.sha256.json').items():assert sha(W/'perf-deps'/p)==h
    assert read(W/'generation-check.json')['changed_files']==[]
    selected=read(W/'generated/overrides.json');byaddr={r['address']:r for r in selected};base=read(W/'generated-rebase/overrides.json');baseaddrs={r['address'] for r in base}
    assert len({r['pc'] for r in selected})==len(selected)
    assert not (set(byaddr)&set(read(HERE/'round4_exclusions.json')))
    cert=read(W/'qualification.json')['functions'];cert.update(read(W/'qualification-compute.json')['functions'])
    for r in selected:
        assert cert[r['address']]['status'] in ('PASS','PASS-void')
        assert r['source_sha256']==cert[r['address']]['source_sha256']==sha(W/f'snapshot/project/src_functional/{r["address"]}.c')
    prof=read(W/'profile/profile-functions.json');coverage={};scenarios={};totals=[0]*4;batched=[0]*2;baseline=[0]*4;final=[0]*4;independent=[0]*4;aggregate=collections.defaultdict(lambda:[0]*4)
    for name in NAMES:
        stock=valid('stock',name,W/'build-stock/saturn_stv');rebased=valid('rebase',name,W/'build-rebase/saturn_stv');actual=valid('final',name,W/'build-round4/saturn_stv');profile=valid('profile-logical',name,W/'build-profile/saturn_stv')
        for item in (rebased,actual,profile):compare(stock,item)
        assert rebased['audit'] and actual['audit']
        log=(W/f'runs/profile-logical/{name}/run.log').read_text();rows={};batch={}
        for row in re.findall(r'GUEST_PROFILE (\d+) (\d+) (\d+) (\d+) (\d+)',log):
            i,*v=map(int,row);a=prof[i]['address'];rows[a]=v;aggregate[a]=[x+y for x,y in zip(aggregate[a],v)]
        for i,m,s in re.findall(r'GUEST_BATCH_PROFILE (\d+) (\d+) (\d+)',log):batch[prof[int(i)]['address']]=[int(m),int(s)]
        for r in selected:
            assert actual['counts'].get(str(r['index']),[0,0])==rows.get(r['address'],[0]*4)[:2],(name,r['address'],'wrapper/profile entry mismatch')
        for r in base:assert rebased['counts'].get(str(r['index']),[0,0])==rows.get(r['address'],[0]*4)[:2]
        total=[sum(v[i] for v in rows.values()) for i in range(4)]
        before=[sum(v[i] for a,v in rows.items() if a in baseaddrs) for i in range(4)]
        after=[sum(v[i] for a,v in rows.items() if a in byaddr) for i in range(4)]
        direct=[sum(v[i] for a,v in rows.items() if byaddr.get(a,{}).get('bridge') in ('direct','resumable-C')) for i in range(4)]
        batches=[sum(v[i] for v in batch.values()) for i in range(2)]
        for dest,src in [(totals,total),(baseline,before),(final,after),(independent,direct),(batched,batches)]:
            for i,n in enumerate(src):dest[i]+=n
        coverage[name]=dict(total_emitted=total,batched_idle_instructions=batches,rebase=before,final=after,residue_free=direct,by_function=rows)
        scenarios[name]=dict(stock=stock,rebase=rebased,override=actual,profile=profile)
    full={};stock=valid('stock-fullgame','fullgame',W/'build-stock/saturn_stv')
    for label,binary in [('rebase-fullgame','rebase'),('final-fullgame','round4')]:
        r=valid(label,'fullgame',W/f'build-{binary}/saturn_stv');compare(stock,r);assert r['audit']
        lines=lambda p:[l for l in p.read_text().splitlines() if 'AUTOPLAY' in l or 'SCENETRACE' in l]
        assert lines(W/f'runs/{label}/fullgame/run.log')==lines(W/'runs/stock-fullgame/fullgame/run.log')
        full[label]=r
    normal=valid('normal-final','combat',W/'build-round4/saturn_stv');assert not normal['audit'];compare(scenarios['combat']['stock'],normal)
    logical=sum(totals[2:])+sum(batched)
    summary=dict(order=['master_calls','slave_calls','master_instructions','slave_instructions'],total_emitted=totals,batched_idle_instructions=batched,total_logical_instructions=logical,rebase=baseline,final=final,residue_free=independent,rebase_calls_percent=100*sum(baseline[:2])/sum(totals[:2]),final_calls_percent=100*sum(final[:2])/sum(totals[:2]),rebase_logical_percent=100*sum(baseline[2:])/logical,final_logical_percent=100*sum(final[2:])/logical,residue_free_logical_percent=100*sum(independent[2:])/logical,rebase_emitted_percent=100*sum(baseline[2:])/sum(totals[2:]),final_emitted_percent=100*sum(final[2:])/sum(totals[2:]))
    probes={p.name:p.read_text() for p in W.glob('*delivery*.log')}
    for name in ('pilot-abi-final.log','round2-abi-final.log','contracts-final.log','round2-poll.log','pulse.log'):probes[name]=(W/name).read_text()
    assert probes and all('PASS' in v and 'FAIL' not in v for v in probes.values())
    fixtures=read(W/'fixture-evidence.json')
    for f in fixtures:assert sha(Path(f['path']))==f['sha256'] and 'PASS' in f['replay'] and f['capture_binary_sha256']==sha(W/'build-round4/saturn_stv')
    perf=read(W/'performance-summary.json');assert perf['correctness_jobs_finished_before_measurement']
    result=clean(dict(snapshot=read(W/'snapshot-time.json'),snapshot_manifest_sha256=sha(W/'snapshot.sha256.json'),pinned_files=len(pins),capture_runtime_files=checked,rebase_audit=read(W/'rebase-audit.json'),classes=dict(collections.Counter(r['bridge'] for r in selected)),selected=selected,coverage=coverage,aggregate=summary,aggregate_by_function=dict(aggregate),scenarios=scenarios,fullgame=dict(stock=stock,**full),normal_combat=normal,probes=probes,fixtures=fixtures,performance=perf,implementation_sha256={str(p):sha(p) for p in sorted(HERE.glob('round4_*')) if p.is_file()},binary={k:dict(bytes=(W/f'build-{k}/saturn_stv').stat().st_size,sha256=sha(W/f'build-{k}/saturn_stv')) for k in ('stock','rebase','round4')}))
    (HERE/'evidence/round4-equivalence.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(classes=result['classes'],coverage=summary),indent=2))
if __name__=='__main__':main()
