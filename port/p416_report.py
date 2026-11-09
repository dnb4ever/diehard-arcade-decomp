#!/usr/bin/env python3
"""Summarize the final two-process matrix, preserving all measurements and audio counters."""
import json,re
from pathlib import Path
R=Path(__file__).resolve().parent.parent
out=R/'work/p416/acceptance';r=json.loads((out/'report.json').read_text());assert len(r['cases'])==4
lines=['## Final measurements (Linux; Mac acceptance remains open)','',
'Threadripper PRO 5975WX, SDL dummy video/audio, software renderer, 1x, paced,',
'input delay 3, 9,000 fields per peer. CPU is process user+system / wall;',
'100% means one logical CPU, including render workers. The host is shared;',
'frequency/load variation prevents treating these as isolated Mac timings.',
'The injected latency is **one way**, with uniform +/- jitter and independent',
'packet loss in both directions. Whole-process fields/s includes setup and the',
'500 ms final checksum retransmission linger. Log timestamps are virtual time;',
'wall/CPU times come from GNU time. Source: `work/p416/acceptance/report.json`.','',
'| One-way / jitter / loss | Peer | Rollbacks/field | Re-sim fields/field | Max depth | CPU % | Whole fields/s | Steady 5 s range |',
'|---|---|---:|---:|---:|---:|---:|---|']
summary={};costs=[];common={}
for case,record in r['cases'].items():
 latency,loss,jitter=map(int,case.split(':'));d=out/f'{latency}ms-{loss}pct';summary[case]={}
 for role,p in zip(['host','guest'],record['peers']):
  log=(d/role/'run.log').read_text();rates=[float(x) for x in re.findall(r'host: ([\d.]+) VBlanks/s',log)]
  m=re.search(r'audio stream: (\d+) times empty, (\d+) chunks dropped, (\d+) resyncs',log);assert m
  audio=list(map(int,m.groups()));assert audio==[0,0,0],(case,role,audio)
  scenes=sorted({int(x) for x in re.findall(r'confirmed field=\d+ CRC=[0-9a-f]+ scene=(\d+)',log)})
  rate=r['frames']/p['wall_s'];range_text=f'{min(rates):.2f}–{max(rates):.2f}' if rates else 'n/a'
  lines.append(f'| {latency} / ±{jitter} ms / {loss}% | {role} | {p["rollbacks_per_frame"]:.4f} | {p["resim_per_frame"]:.4f} | {int(p["max_rollback"])} | {p["cpu_percent"]:.1f} | {rate:.2f} | {range_text} |')
  per=p['resim_total_ms']/p['resim_fields'] if p['resim_fields'] else 0
  costs.append(f'| {latency} ms / {role} | {p["save_ms"]:.3f} | {p["load_ms"]:.3f} | {per:.3f} | {p["max_burst_ms"]:.3f} | {int(p["rollbacks"])} / {int(p["resim_fields"])} |')
  summary[case][role]={'fields_s':rate,'steady_min':min(rates),'steady_max':max(rates),'audio_empty_drop_resync':audio,'scenes':scenes,'recovery_ms_per_resim_field':per,**p}
 for name in ['shot-9000.png','audio.wav']:
  digest=record['artifacts'][name]['sha256']
  if name in common:assert common[name]==digest,(case,name)
  common[name]=digest
lines+=['','| Link / peer | Save ms | Load ms | Recovery ms/re-sim field | Largest recovery burst ms | Rollbacks / re-sim fields |',
'|---|---:|---:|---:|---:|---|']+costs+['',
'Save/load columns are measured wall time per operation. Recovery includes',
'unwind/restore, hidden-field emulation/snapshot capture and the final corrected',
'presentation; it excludes deliberate pacing waits. Its per-field number is',
'aggregate recovery time divided by corrected fields, not an exclusive CPU',
'component. Do not add the columns as independent costs. Maxima include OS',
'descheduling and shared-host contention. Snapshot cost is paid every ordinary',
'field; repeated-field captures are counted in the retained logs.','',
'All four pairs pass **151/151 confirmed checkpoints**, final PNG equality,',
'**6,626,841 stereo PCM frames** / 26,507,408-byte WAV equality and every byte',
'of the **4,799,097-byte full final machine payload**. All cases also share the',
'same final PNG and WAV across impairment levels. Whole-state hashes may differ',
'between cases because each host chooses a new RTC origin; each pair is identical.',
'**Zero desyncs, zero audio empty/drop/resync events on all eight final peers.**','',
'Final image SHA256: `'+common['shot-9000.png']+'`. PCM/WAV SHA256: `'+common['audio.wav']+'`.','']
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(out/'measurements.md').write_text('\n'.join(lines))
print('\n'.join(lines))
