#!/usr/bin/env python3
"""Validate all round-2 evidence and export repo-relative portable measurements."""
import argparse,collections,hashlib,json,re,statistics,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];WORK=ROOT/'work/converge/round2';HERE=Path(__file__).resolve().parent
SCENARIOS=('nocoin','gameover','combat','legacy-combat')
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clean(x):
 if isinstance(x,str):return x.replace(str(ROOT)+'/', '')
 if isinstance(x,list):return [clean(v) for v in x]
 if isinstance(x,dict):return {k:clean(v) for k,v in x.items()}
 return x
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--include-fullgame',action='store_true');a=p.parse_args()
 manifest=read(WORK/'generated/overrides.json');selected={r['address']:r for r in manifest};profiles=read(WORK/'profile/profile-functions.json')
 result=dict(snapshot=read(WORK/'snapshot-time.json'),snapshot_manifest_sha256=sha(WORK/'snapshot.sha256.json'),functions=manifest,scenarios={},coverage={},profile_functions=profiles)
 for name in SCENARIOS:
  stock=read(WORK/f'runs/stock-clock/{name}/result.json');actual=read(WORK/f'runs/round2-202-clock/{name}/result.json');profile=read(WORK/f'runs/profile/{name}/result.json')
  for item in (stock,actual,profile):
   assert item['exit_code']==0 and not item['errors'] and len(item['png'])==item['expected_shots']
  assert actual['audit']
  for key in ('png','trace_sha256','frame_count','frames_sha256'):
   assert stock[key]==actual[key]==profile[key],(name,key)
  assert stock['frame_count']==(9000 if name in ('nocoin','gameover') else 2100)
  result['scenarios'][name]=dict(stock=stock,override=actual,profile=profile)
  groups=collections.defaultdict(lambda:[0,0,0,0]);fn={}
  for row in re.findall(r'GUEST_PROFILE (\d+) (\d+) (\d+) (\d+) (\d+)',(WORK/f'runs/profile/{name}/run.log').read_text()):
   index,*counts=map(int,row);f=profiles[index];kind=selected.get(f['address'],{}).get('bridge','compiled')
   groups[kind]=[v+n for v,n in zip(groups[kind],counts)]
   fn[f['address']]=dict(master_calls=counts[0],slave_calls=counts[1],master_instructions=counts[2],slave_instructions=counts[3],bridge=kind)
  total=[sum(v[i] for v in groups.values()) for i in range(4)]
  for r in manifest:
   address=r['address'];profile_counts=fn.get(address,{})
   observed=actual['counts'].get(str(r['index']),[0,0])
   assert observed==[profile_counts.get('master_calls',0),profile_counts.get('slave_calls',0)],(name,address,'entry counters differ')
  numerator=[sum(v[i] for k,v in groups.items() if k!='compiled') for i in range(4)]
  independent=[sum(v[i] for k,v in groups.items() if k in ('direct','resumable-C')) for i in range(4)]
  result['coverage'][name]=dict(order=['master_calls','slave_calls','master_instructions','slave_instructions'],total=total,semantic_c=numerator,residue_free_c=independent,by_class=dict(groups),by_function=fn,calls_percent=100*sum(numerator[:2])/sum(total[:2]),instructions_percent=100*sum(numerator[2:])/sum(total[2:]))
 agg={k:[sum(v[k][i] for v in result['coverage'].values()) for i in range(4)] for k in ('total','semantic_c','residue_free_c')}
 agg['calls_percent']=100*sum(agg['semantic_c'][:2])/sum(agg['total'][:2]);agg['instructions_percent']=100*sum(agg['semantic_c'][2:])/sum(agg['total'][2:]);agg['residue_free_instructions_percent']=100*sum(agg['residue_free_c'][2:])/sum(agg['total'][2:]);result['aggregate']=agg
 if a.include_fullgame:
  stock=read(WORK/'runs/stock-fullgame/fullgame/result.json');actual=read(WORK/'runs/round2-204-fullgame/fullgame/result.json')
  assert stock['exit_code']==actual['exit_code']==0 and not stock['errors'] and not actual['errors']
  assert stock['png']==actual['png'] and stock['trace_sha256']==actual['trace_sha256']
  log=(WORK/'runs/round2-204-fullgame/fullgame/run.log').read_text();assert 'scene=73 ' in log
  stock_log=(WORK/'runs/stock-fullgame/fullgame/run.log').read_text()
  state_lines=[line for line in log.splitlines() if 'AUTOPLAY' in line or 'SCENETRACE' in line]
  assert state_lines==[line for line in stock_log.splitlines() if 'AUTOPLAY' in line or 'SCENETRACE' in line]
  result['fullgame']=dict(stock=stock,override=actual,autoplay_scene_lines=len(state_lines),
   autoplay_scene_sha256=hashlib.sha256(('\n'.join(state_lines)+'\n').encode()).hexdigest(),
   ending_scenes=[line for line in log.splitlines() if re.search(r'SCENETRACE.*scene=(73|79|80|81) ',line)])
 result['exclusions']=read(HERE/'round2_exclusions.json')
 result['autoplay_attempts']={label:read(WORK/f'runs/{label}/fullgame/result.json') for label in ('round2-final-fullgame','round2-204-fullgame')}
 result['stock_ending']=read(WORK/'runs/stock-fullgame/fullgame/result.json')
 result['autoplay_limit']=dict(final_cohort_ending_proven=False,last_candidate=204,last_matching_autoplay_vblank=146340,reason='Second live bus-model wall; final cohort excludes vector helpers and ending is not claimed')
 result['size']={name:dict(bytes=(WORK/f'build-{name}/saturn_stv').stat().st_size,binary_sha256=sha(WORK/f'build-{name}/saturn_stv')) for name in ('stock','round2','mod','profile')}
 result['elf_sections']=subprocess.check_output(['size',str(WORK/'build-stock/saturn_stv'),str(WORK/'build-round2/saturn_stv')],text=True)
 for r in result['scenarios'].values():assert r['override']['binary_sha256']==result['size']['round2']['binary_sha256']
 perf=read(WORK/'performance.json');perf=[r for r in perf if r['variant']=='stock']+read(WORK/'performance202.json')
 for r in perf:
  assert r['binary_sha256']==result['size'][r['variant']]['binary_sha256']
  s=result['scenarios']['combat']['stock'];assert r['png']==s['png'] and r['trace_sha256']==s['trace_sha256']
 result['performance']=dict(runs=perf,median_cpu={name:statistics.median(r['cpu_seconds'] for r in perf if r['variant']==name) for name in ('stock','round2')},median_wall={name:statistics.median(r['seconds'] for r in perf if r['variant']==name) for name in ('stock','round2')})
 frozen=read(WORK/'snapshot.sha256.json');drift=[p for p,h in frozen.items() if sha(WORK/'snapshot'/p)!=h];assert not drift;result['snapshot_rechecked_files']=len(frozen)
 result['clock_input']=read(HERE/'evidence/round2-clock-input.json')
 result['qualification']=read(WORK/'qualification.json');result['poll_qualification']=read(WORK/'qualification-polls/report.json');result['mod_boundaries']=read(WORK/'playfield-mod-boundaries.json')
 result['probes']={name:(WORK/name).read_text() for name in ('abi-probe.log','poll-probe.log','pilot-abi-probe.log')}
 assert all('PASS:' in v and 'FAIL' not in v for v in result['probes'].values())
 # Every selected source is still exactly the one certified at freeze time.
 for r in manifest:assert sha(WORK/'snapshot/project/src_functional'/f'{r["address"]}.c')==r['source_sha256']
 result['implementation_hashes']={str(p.relative_to(ROOT)):sha(p) for p in sorted(HERE.rglob('*')) if p.is_file() and 'evidence' not in p.parts and '__pycache__' not in p.parts}
 output=HERE/'evidence/round2-equivalence.json';output.write_text(json.dumps(clean(result),indent=2)+'\n')
 print(json.dumps(dict(classes=dict(collections.Counter(r['bridge'] for r in manifest)),aggregate=agg,size=result['size'],performance={k:v for k,v in result['performance'].items() if k!='runs'},evidence=str(output.relative_to(ROOT))),indent=2))
if __name__=='__main__':main()
