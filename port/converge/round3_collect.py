#!/usr/bin/env python3
"""Validate exact final round-3 evidence and export honest C-work coverage."""
from pathlib import Path
import argparse,collections,hashlib,json,re
HERE=Path(__file__).resolve().parent
WORK=Path('work/converge/round3')
ROOT=HERE.parents[1]
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clean(x):
 if isinstance(x,str):return x.replace(str(ROOT)+'/', '')
 if isinstance(x,list):return [clean(v) for v in x]
 if isinstance(x,dict):return {k:clean(v) for k,v in x.items()}
 return x

def collect(fullgame=True):
 selected=read(WORK/'generated/overrides.json');byaddr={r['address']:r for r in selected}
 profiles=read(WORK/'profile/profile-functions.json');cert=read(WORK/'qualification.json')
 drift=[name for name,digest in read(WORK/'snapshot.sha256.json').items() if sha(WORK/'snapshot'/name)!=digest]
 assert not drift,drift
 for row in selected:
  a=row['address'];assert row['source_sha256']==sha(WORK/f'snapshot/project/src_functional/{a}.c')==cert['functions'][a]['source_sha256']
  assert cert['functions'][a]['status'] in ('PASS','PASS-void')
 runtime_checked=0
 for source in (WORK/'snapshot/tools/saturn-recomp/runtime').rglob('*'):
  if not source.is_file() or source.is_symlink():continue
  relative=source.relative_to(WORK/'snapshot/tools/saturn-recomp/runtime');expected=source.read_bytes()
  if str(relative)=='src/video/video.cpp':
   old=b'return !value || std::strcmp(value, "0");'
   new=b'const char* interval = std::getenv("CONVERGE_RAM_INTERVAL");\n    unsigned every = interval ? std::strtoul(interval, nullptr, 10) : 1;\n    return (!value || std::strcmp(value, "0")) && (!every || g_vblanks % every == 0);'
   assert expected.count(old)==1;expected=expected.replace(old,new)
  assert (WORK/'capture-runtime'/relative).read_bytes()==expected,('diagnostic runtime drift',str(relative))
  runtime_checked+=1
 result=dict(diagnostic_runtime_rechecked_files=runtime_checked,snapshot=read(WORK/'snapshot-time.json'),snapshot_manifest_sha256=sha(WORK/'snapshot.sha256.json'),snapshot_rechecked_files=len(read(WORK/'snapshot.sha256.json')),functions=selected,classes=dict(collections.Counter(r['bridge'] for r in selected)),coverage={},scenarios={})
 for name in ('nocoin','gameover','combat','legacy-combat'):
  stock=read(WORK/f'runs/stock/{name}/result.json');actual=read(WORK/f'runs/final/{name}/result.json');profile=read(WORK/f'runs/profile/{name}/result.json')
  for item in (stock,actual,profile):assert item['exit_code']==0 and not item['errors'] and len(item['png'])==item['expected_shots']
  for key in ('png','trace_sha256','frame_count','frames_sha256'):assert stock[key]==actual[key]==profile[key],(name,key)
  assert actual['audit'];assert stock['frame_count']==(9000 if name in ('nocoin','gameover') else 2100)
  indices=[json.loads(line)['frame'] for line in (WORK/f'runs/final/{name}/frames.jsonl').read_text().splitlines()]
  assert indices==list(range(1,stock['frame_count']+1))
  assert actual['binary_sha256']==sha(WORK/'build-round3/saturn_stv')
  groups=collections.defaultdict(lambda:[0,0,0,0]);fns={}
  for line in re.findall(r'GUEST_PROFILE (\d+) (\d+) (\d+) (\d+) (\d+)',(WORK/f'runs/profile/{name}/run.log').read_text()):
   i,*counts=map(int,line);f=profiles[i];kind=byaddr.get(f['address'],{}).get('bridge','compiled')
   groups[kind]=[v+n for v,n in zip(groups[kind],counts)]
   fns[f['address']]=dict(master_calls=counts[0],slave_calls=counts[1],master_instructions=counts[2],slave_instructions=counts[3],bridge=kind)
  for row in selected:
   counts=fns.get(row['address'],{});assert actual['counts'].get(str(row['index']),[0,0])==[counts.get('master_calls',0),counts.get('slave_calls',0)],(name,row['address'],'entry count mismatch')
  totals=[sum(v[i] for v in groups.values()) for i in range(4)]
  semantic=[sum(v[i] for k,v in groups.items() if k!='compiled') for i in range(4)]
  independent=[sum(v[i] for k,v in groups.items() if k in ('direct','resumable-C')) for i in range(4)]
  result['scenarios'][name]=dict(stock=stock,override=actual,profile=profile)
  result['coverage'][name]=dict(order=['master_calls','slave_calls','master_instructions','slave_instructions'],total=totals,semantic_c=semantic,residue_free_c=independent,by_class=dict(groups),by_function=fns,calls_percent=100*sum(semantic[:2])/sum(totals[:2]),instructions_percent=100*sum(semantic[2:])/sum(totals[2:]))
 aggregate={k:[sum(v[k][i] for v in result['coverage'].values()) for i in range(4)] for k in ('total','semantic_c','residue_free_c')}
 aggregate['calls_percent']=100*sum(aggregate['semantic_c'][:2])/sum(aggregate['total'][:2]);aggregate['instructions_percent']=100*sum(aggregate['semantic_c'][2:])/sum(aggregate['total'][2:]);aggregate['residue_free_instructions_percent']=100*sum(aggregate['residue_free_c'][2:])/sum(aggregate['total'][2:])
 result['aggregate']=aggregate
 if fullgame:
  stock=read(WORK/'runs/stock-fullgame/fullgame/result.json');actual=read(WORK/'runs/final-fullgame/fullgame/result.json')
  for item in (stock,actual):assert item['exit_code']==0 and not item['errors'] and len(item['png'])==item['expected_shots']==34 and item['frame_count']==170000
  for key in ('png','trace_sha256','frame_count','frames_sha256'):assert stock[key]==actual[key],('fullgame',key)
  assert actual['binary_sha256']==sha(WORK/'build-round3/saturn_stv')
  log=(WORK/'runs/final-fullgame/fullgame/run.log').read_text();original=(WORK/'runs/stock-fullgame/fullgame/run.log').read_text()
  states=lambda s:[l for l in s.splitlines() if 'AUTOPLAY' in l or 'SCENETRACE' in l]
  assert states(log)==states(original)
  for scene in (73,79,80,81):assert re.search(rf'scene={scene} ',log)
  records=[json.loads(l) for l in (WORK/'runs/final-fullgame/fullgame/frames.jsonl').read_text().splitlines()]
  assert [r['frame'] for r in records]==list(range(1,170001))
  ram=[r['frame'] for r in records if r['ram']];assert ram==list(range(300,170001,300))
  result['fullgame']=dict(stock=stock,override=actual,semantic_records=len(records),ram_samples=len(ram),ram_interval=300,autoplay_scene_lines=len(states(log)),ending_scenes=[73,79,80,81])
 result['fixtures']=read(HERE/'evidence/round3-saved-frames.json')
 for item in result['fixtures']:
  assert sha(Path(item['path']))==item['sha256'] and 'PASS' in item['replay']
 normal=read(WORK/'runs/normal/combat/result.json');stock=result['scenarios']['combat']['stock']
 assert not normal['audit'] and normal['exit_code']==0 and all(normal[k]==stock[k] for k in ('png','trace_sha256','frame_count','frames_sha256'))
 result['normal_combat']=normal
 result['implementation_sha256']={str(p):sha(p) for p in sorted(HERE.glob('round3_*')) if p.is_file()}
 result['capture_patch_sha256']=sha(HERE/'round3_capture.patch')
 result['probes']={name:(WORK/name).read_text() for name in ('contracts.log','pulse.log','pilot-abi.log','round2-abi.log','round2-poll.log','bios.log')}
 for text in result['probes'].values():assert 'PASS' in text and 'FAIL' not in text
 result['binary']={n:dict(bytes=(WORK/f'build-{n}/saturn_stv').stat().st_size,sha256=sha(WORK/f'build-{n}/saturn_stv')) for n in ('stock','round3')}
 result=clean(result);dest=HERE/'evidence/round3-equivalence.json';dest.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(dict(classes=result['classes'],coverage=aggregate,fullgame=result.get('fullgame',{}).get('semantic_records'),fixtures=result['fixtures']),indent=2))
 return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--reference-only',action='store_true');a=p.parse_args();collect(not a.reference_only)
