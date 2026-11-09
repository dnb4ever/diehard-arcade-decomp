#!/usr/bin/env python3
"""Sequential, CPU-only rewind timing/RSS measurements, without per-field capture overhead."""
import json,os,re,subprocess,time
from pathlib import Path
os.chdir(Path(__file__).resolve().parent.parent)
out=Path('work/p413/performance');out.mkdir(parents=True,exist_ok=True);report={}
for window in [False,True]:
 for enabled in [False,True]:
  name=('window' if window else 'headless')+('-rewind' if enabled else '-baseline');d=out/name;d.mkdir(exist_ok=True)
  env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','SDL_'))}
  env.update(STV_AUTOPLAY='100',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
  cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks','1800','--stvinput','99999:COIN','--out',str(d)]
  if window:cmd+=['--window','--settings','-','--stats','5']
  if enabled:cmd+=['--rewind-seconds','20']
  with (d/'run.log').open('w') as f:
   r=subprocess.run(['/usr/bin/time','-f','%e %M','-o',str(d/'time.txt'),*cmd],env=env,stdout=f,stderr=subprocess.STDOUT,timeout=300)
  text=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in text,name
  seconds,rss=map(float,(d/'time.txt').read_text().split());m=re.search(r'rewind: (\d+) captures, ([\d.]+) ms/capture, peak (\d+) bytes, (\d+) snapshots retained',text)
  report[name]=dict(command=cmd,seconds=seconds,fields_per_second=1800/seconds,max_rss_kib=int(rss),capture_count=int(m[1]) if m else 0,ms_per_capture=float(m[2]) if m else 0,ring_peak_bytes=int(m[3]) if m else 0,snapshots=int(m[4]) if m else 0)
  (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
  print(name,json.dumps(report[name]),flush=True)
for mode in ['headless','window']:
 a=report[mode+'-baseline'];b=report[mode+'-rewind']
 report[mode+'-cost']=dict(extra_wall_percent=100*(b['seconds']/a['seconds']-1),extra_peak_rss_mib=(b['max_rss_kib']-a['max_rss_kib'])/1024,estimated_ms_per_field=b['ms_per_capture']/2)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
