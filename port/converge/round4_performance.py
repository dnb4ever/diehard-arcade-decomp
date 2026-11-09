#!/usr/bin/env python3
"""Frozen SDL software builds and sequential, profiler-off 1x measurements."""
import argparse,hashlib,json,os,re,shutil,subprocess,time
from pathlib import Path
W=Path('work/converge/round4')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(variant):
    deps=W/'perf-deps'
    if not deps.exists():
        shutil.copytree('port/run/P46/sdl3deb',deps,symlinks=True)
        (W/'perf-deps.sha256.json').write_text(json.dumps({str(p.relative_to(deps)):sha(p) for p in sorted(deps.rglob('*')) if p.is_file()},indent=2)+'\n')
    source=W/('snapshot/port/run/P41/recomp' if variant=='stock' else 'generated')
    dest=W/f'build-window-{variant}'
    subprocess.run(['cmake','-S',str(source),'-B',str(dest),f'-DSATURN_RUNTIME={(W/"capture-runtime").resolve()}',f'-DSATURN_EXTRA={(W/"snapshot/port/stv.cmake").resolve()}','-DSATURN_NO_SDL=OFF',f'-DSDL3_DIR={(deps/"x/usr/lib/x86_64-linux-gnu/cmake/SDL3").resolve()}'],check=True)
    subprocess.run(['cmake','--build',str(dest),'--target','saturn_stv','-j4'],check=True)
def run(variant,label,fields):
    import round2_verify
    round2_verify.verify.WORK=W;round2_verify.clock_input()
    d=W/'performance'/label;d.mkdir(parents=True,exist_ok=False)
    settings=d/'settings.ini';settings.write_text('[display]\nframeskip = false\ninternal_resolution = 1\nscale = 3\nscaling = sharp\ncrt = false\n')
    env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','CONVERGE_'))}
    env.update(STV_AUTOPLAY='100',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software')
    binary=W/f'build-window-{variant}/saturn_stv'
    cmd=[str(binary),'--cart',str(W/'snapshot/port/diehard_main.bin'),'--cartrom',str(W/'snapshot/cart.bin'),'--window','--settings',str(settings),'--stats','5','--unpaced','--vblanks',str(fields),'--stvinput','99999:COIN','--out',str(d),'--shots',str(fields)]
    before=os.getloadavg();start=time.monotonic()
    with (d/'run.log').open('w') as f:r=subprocess.run(['/usr/bin/time','-v']+cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=1800)
    seconds=time.monotonic()-start;log=(d/'run.log').read_text()
    assert r.returncode==0 and 'FATAL' not in log
    samples=[dict(fields_s=float(v),seconds=float(t),vb=int(b),omitted=int(n)) for v,t,b,n in re.findall(r'host: ([0-9.]+) VBlanks/s over ([0-9.]+) s \(VB (\d+)\), (\d+) pictures left out',log)]
    assert samples and all(s['omitted']==0 for s in samples)
    result=dict(variant=variant,command=cmd,binary_sha256=sha(binary),fields=fields,wall_seconds=seconds,fields_s=fields/seconds,load_before=before,load_after=os.getloadavg(),samples=samples,environment={k:v for k,v in env.items() if k.startswith(('SDL_','STV_','SATURN_','CONVERGE_'))},png_sha256=sha(d/f'shot-{fields}.png'))
    for key,pat in [('cpu_percent',r'Percent of CPU this job got: (\d+)%'),('user_s',r'User time \(seconds\): ([0-9.]+)'),('system_s',r'System time \(seconds\): ([0-9.]+)')]:result[key]=float(re.search(pat,log)[1])
    result['cpu_ms_per_field']=1000*(result['user_s']+result['system_s'])/fields
    (d/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(label,result['fields_s'],result['cpu_percent'],result['cpu_ms_per_field'],flush=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('variant',choices=['stock','round4']);p.add_argument('--build',action='store_true');p.add_argument('--label');p.add_argument('--fields',type=int,default=3600);a=p.parse_args()
    if a.build:build(a.variant)
    if a.label:run(a.variant,a.label,a.fields)
