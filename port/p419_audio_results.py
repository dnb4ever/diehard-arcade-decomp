#!/usr/bin/env python3
"""Check every impairment against the P4.18 confirmed baseline; summarize playout."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tag',default='release-acceptance');a=p.parse_args()
root=Path('work/p419/audio')
baseline=json.loads((root/'baseline/report.json').read_text())['cases']['0:0:0']
report=json.loads((root/a.tag/'report.json').read_text())
summary={}
for name,case in report['cases'].items():
 assert case['artifacts']==baseline['artifacts'],(name,'baseline artifacts')
 for peer in case['peers']:
  assert peer['confirmed']==baseline['peers'][0]['confirmed'],(name,'baseline checkpoints')
 summary[name]=[dict(rollbacks=p['rollbacks'],**p['audio']) for p in case['peers']]
(root/'measurements.json').write_text(json.dumps(summary,indent=2)+'\n')
print('All cases match P4.18: 151 checkpoints, complete machine, PNG and confirmed WAV')
for name,peers in summary.items():
 print(name,[(p['corrections'],p['submitted_differences'],p['discontinuities'],p['empty_blocks'],p['submit_mean_ms']) for p in peers])
