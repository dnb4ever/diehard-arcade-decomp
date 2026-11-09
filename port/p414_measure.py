#!/usr/bin/env python3
"""CPU-only SDL software throughput, with frameskip disabled and original guest timing."""
import argparse,json,os,re,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;os.chdir(ROOT)
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fields',type=int,default=3600);p.add_argument('--scales',default='1,2,3');p.add_argument('--unpaced',action='store_true');p.add_argument('--tag',default='final');p.add_argument('--frameskip',action='store_true');p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv');a=p.parse_args()
out=Path('work/p414/performance');out.mkdir(exist_ok=True);report={}
for scale in map(int,a.scales.split(',')):
 name=f'{a.tag}-{scale}x-'+('unpaced' if a.unpaced else 'paced');d=out/name;d.mkdir(exist_ok=True)
 ini=d/'settings.ini';ini.write_text(f'[display]\nframeskip = {str(a.frameskip).lower()}\ninternal_resolution = {scale}\nscale = 3\nscaling = sharp\ncrt = false\n')
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 env.update(STV_AUTOPLAY='100',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
 cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--window','--settings',str(ini),'--stats','5','--vblanks',str(a.fields),'--stvinput','99999:COIN','--out',str(d)]
 if a.unpaced:cmd+=['--unpaced']
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(['/usr/bin/time','-v']+cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=1800)
 log=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in log,(name,r.returncode)
 samples=[dict(fields_s=float(x),seconds=float(t),vb=int(v),omitted=int(n)) for x,t,v,n in re.findall(r'host: ([0-9.]+) VBlanks/s over ([0-9.]+) s \(VB (\d+)\), (\d+) pictures left out',log)]
 cpu=re.search(r'Percent of CPU this job got: (\d+)%',log)
 report[name]=dict(command=cmd,seconds=time.monotonic()-start,fields=a.fields,samples=samples,cpu_percent=int(cpu[1]),frameskip=a.frameskip,host_loadavg=os.getloadavg(),renderer='SDL software (CPU)',environment={k:env[k] for k in ['SDL_VIDEO_DRIVER','SDL_AUDIO_DRIVER','SDL_RENDER_DRIVER','STV_AUTOPLAY']})
 (out/f'{name}.json').write_text(json.dumps(report[name],indent=2)+'\n')
 print(name,report[name]['cpu_percent'],'% CPU',samples,flush=True)
