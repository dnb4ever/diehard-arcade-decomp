#!/usr/bin/env python3
"""Sequential unpaced CPU-only comparison. Keep identical fields, inputs and output size."""
import argparse,json,os,re,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv');p.add_argument('--tag',required=True)
p.add_argument('--fields',type=int,default=3600);p.add_argument('--scales',default='1,2,3');p.add_argument('--profile',action='store_true')
a=p.parse_args();out=Path('work/p415/performance');out.mkdir(parents=True,exist_ok=True)
for scale in map(int,a.scales.split(',')):
 d=out/f'{a.tag}-{scale}x';d.mkdir(exist_ok=True)
 ini=d/'settings.ini';ini.write_text(f'[display]\nframeskip = false\ninternal_resolution = {scale}\nscale = 3\nscaling = sharp\ncrt = false\n')
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 env.update(STV_AUTOPLAY='100',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
 if a.profile:env['SATURN_PROFILE']='1'
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--window','--settings',str(ini),'--stats','5','--unpaced','--vblanks',str(a.fields),'--stvinput','99999:COIN','--out',str(d)]
 load_before=os.getloadavg();start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(['/usr/bin/time','-v']+cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=1800)
 wall=time.monotonic()-start;log=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in log,(d,r.returncode)
 samples=[dict(fields_s=float(x),seconds=float(t),vb=int(v),omitted=int(n)) for x,t,v,n in re.findall(r'host: ([0-9.]+) VBlanks/s over ([0-9.]+) s \(VB (\d+)\), (\d+) pictures left out',log)]
 profiles={}
 for vb,part,c,w,n in re.findall(r'PERF fields=(\d+) part=([^ ]+) cpu_ns=(\d+) elapsed_ns=(\d+) calls=(\d+)',log):profiles.setdefault(vb,{})[part]=dict(cpu_ns=int(c),elapsed_ns=int(w),calls=int(n))
 report=dict(command=cmd,environment={k:env[k] for k in env if k.startswith(('SDL_','STV_','SATURN_'))},profile=a.profile,fields=a.fields,wall_seconds=wall,fields_s=a.fields/wall,load_before=load_before,load_after=os.getloadavg(),samples=samples,profiles=profiles)
 for label,pat in [('cpu_percent',r'Percent of CPU this job got: (\d+)%'),('user_s',r'User time \(seconds\): ([0-9.]+)'),('system_s',r'System time \(seconds\): ([0-9.]+)')]:report[label]=float(re.search(pat,log)[1])
 (d/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(d,round(report['fields_s'],2),'fields/s',report['cpu_percent'],'% CPU','load',report['load_after'],flush=True)
