#!/usr/bin/env python3
"""P4.24 windowed on/off pixel, trace, WAV, smoke and exact state evidence (CPU SDL)."""
import argparse, hashlib, json, os, re, subprocess, time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv')
p.add_argument('--sync',choices=['on','off'],required=True)
p.add_argument('--tag',required=True)
p.add_argument('--checks',default='regression,smoke,state')
p.add_argument('--headless',action='store_true',help='release guest-output check without presentation')
p.add_argument('--x11',action='store_true',help='use caller-owned Xvfb DISPLAY, always software renderer')
a=p.parse_args();root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p424')/a.tag;out.mkdir(parents=True,exist_ok=True)
ini=out/'settings.ini';ini.write_text(f'[display]\nvsync = {str(a.sync=="on").lower()}\nscale = 1\n')
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','SDL_'))}
env.update(SDL_VIDEO_DRIVER='x11' if a.x11 else 'dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',APPIMAGE_EXTRACT_AND_RUN='1')
sha=lambda b:hashlib.sha256(b).hexdigest()
report=json.loads((out/'report.json').read_text()) if (out/'report.json').exists() else {};checks=a.checks.split(',')
def run(name,cmd,extra=None):
 d=out/name;d.mkdir(exist_ok=True)
 cmd=cmd+([] if a.headless else ['--window'])+['--no-launcher','--settings',str(ini),'--out',str(d),'--stats','5']
 start=time.monotonic()
 with (d/'run.log').open('w') as log:
  r=subprocess.run(cmd,env=dict(env,**(extra or {})),stdout=log,stderr=log,timeout=1200)
 text=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in text,(name,r.returncode,text[-1000:])
 report[name]=dict(command=cmd,seconds=time.monotonic()-start,vsync=a.sync,
  rates=[float(v) for v in re.findall(r'host: ([\d.]+) VBlanks/s',text)],
  lag_ms=[float(v) for v in re.findall(r'pace lag ([-\d.]+) ms',text)],
  skipped=[int(v) for v in re.findall(r', (\d+) pictures left out',text)],
  transitions=re.findall(r'display: VSync (.*)',text))
 return d,text
base=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin']
if 'regression' in checks:
 reference=json.loads(Path('work/p411/regression/after/manifest.json').read_text())
 for name,ref in reference.items():
  cmd=ref['command'].copy();cmd[0]=a.binary
  del cmd[cmd.index('--out'):cmd.index('--out')+2]
  cmd[cmd.index('--wav')+1]=str(out/name/'audio.wav')
  # Capture requested PNGs only. Hardware dumps are deliberately disabled.
  extra={k:v for k,v in ref['env'].items() if k!='STV_CAPTURE_FIELDS'}
  d,text=run(name,cmd,extra)
  trace='\n'.join(l for l in text.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples',l))+'\n'
  guest='\n'.join(l for l in text.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after',l))+'\n'
  actual=dict(png={n:sha((d/n).read_bytes()) for n in ref['png']},trace_sha256=sha(trace.encode()),guest_trace_sha256=sha(guest.encode()),wav_sha256=sha((d/'audio.wav').read_bytes()))
  for key,value in actual.items():assert value==ref[key],(name,key,value,ref[key])
  report[name].update(actual);(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
  print(name,'PASS',len(actual['png']),'PNGs, traces and WAV',flush=True)
if 'smoke' in checks:
 for factor,expected in [(1,'9bd5386ea7439621c19674381086e8fd'),(2,'8f1d9a1f01a2b77cd9d4f82b22b71b37')]:
  name=f'smoke-{factor}x'
  # CLI overrides saved resolution after the window has loaded its defaults.
  ini.write_text(f'[display]\nvsync = {str(a.sync=="on").lower()}\nscale = 1\ninternal_resolution = {factor}\n')
  d,text=run(name,base+['--internal-resolution',str(factor),'--vblanks','2000','--shots','1900','--stvinput','200:COIN,340:START1'])
  digest=hashlib.md5((d/'shot-1900.png').read_bytes()).hexdigest();assert digest==expected,(factor,digest)
  report[name]['md5']=digest;print(name,'PASS',digest,flush=True)
if 'state' in checks:
 # Fixed RTC, no test-only hardware logging; compare the complete state payload.
 shim=out/'fixed-time.so';subprocess.run(['cc','-shared','-fPIC','port/p415_fixed_time.c','-o',str(shim)],check=True)
 ini.write_text(f'[display]\nvsync = {str(a.sync=="on").lower()}\nscale = 1\n')
 d,text=run('state',base+['--vblanks','2100','--stvinput','100:COIN,130:START1','--save-at','2000','--quick-state',str(out/'state/state.quickstate'),'--shots','1900,2100','--wav',str(out/'state/audio.wav')],{'LD_PRELOAD':str(shim.resolve()),'TZ':'UTC'})
 report['state']['payload_sha256']=sha((d/'state.quickstate').read_bytes())
 report['state']['wav_sha256']=sha((d/'audio.wav').read_bytes())
 report['state']['png']={n.name:sha(n.read_bytes()) for n in d.glob('*.png')}
 print('state',report['state']['payload_sha256'],flush=True)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
