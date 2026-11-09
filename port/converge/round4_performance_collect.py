#!/usr/bin/env python3
"""Collect paired 1x runs without concealing individual variance."""
import argparse,hashlib,json,statistics
from pathlib import Path
W=Path('work/converge/round4')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
parser=argparse.ArgumentParser();parser.add_argument('--after',action='store_true');args=parser.parse_args()
labels=['stock-d','round4-d','round4-e','stock-e','stock-f','round4-f'] if args.after else ['stock-a','round4-a','round4-b','stock-b','stock-c','round4-c']
if args.after:
    for label in ('stock-fullgame','rebase-fullgame','final-fullgame'):
        result=json.loads((W/f'runs/{label}/fullgame/result.json').read_text());assert result['exit_code']==0 and result['frame_count']==170000
    for name in ('nocoin','gameover','combat','legacy-combat'):
        assert json.loads((W/f'runs/final/{name}/result.json').read_text())['exit_code']==0
rows=[json.loads((W/f'performance/{label}/result.json').read_text()) for label in labels]
assert len({r['png_sha256'] for r in rows})==1
for r in rows:
 assert r['fields']==3600 and r['binary_sha256']==sha(W/f'build-window-{r["variant"]}/saturn_stv')
 assert all(s['omitted']==0 for s in r['samples'])
summary={}
for variant in ('stock','round4'):
 group=[r for r in rows if r['variant']==variant]
 summary[variant]=dict(fields=sum(r['fields'] for r in group),fields_s=sum(r['fields'] for r in group)/sum(r['wall_seconds'] for r in group),cpu_ms_per_field=sum(r['user_s']+r['system_s'] for r in group)*1000/sum(r['fields'] for r in group),cpu_percent=sum(r['user_s']+r['system_s'] for r in group)*100/sum(r['wall_seconds'] for r in group),individual_fields_s=[r['fields_s'] for r in group],individual_cpu_ms_per_field=[r['cpu_ms_per_field'] for r in group])
summary['throughput_percent_change']=100*(summary['round4']['fields_s']/summary['stock']['fields_s']-1)
summary['cpu_cost_percent_change']=100*(summary['round4']['cpu_ms_per_field']/summary['stock']['cpu_ms_per_field']-1)
summary['order']=labels;summary['runs']=rows
summary['correctness_jobs_finished_before_measurement']=args.after
if args.after:summary['during_correctness']=json.loads((W/'performance-during-summary.json').read_text())
(W/'performance-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps({k:v for k,v in summary.items() if k!='runs'},indent=2))
