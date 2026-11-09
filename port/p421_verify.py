#!/usr/bin/env python3
"""CPU-only stock/off/identity regressions and inspected example captures."""
import concurrent.futures,hashlib,json,os,re,subprocess,time
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file());os.chdir(ROOT)
W=Path('work/p421');B=W/'build/saturn_stv';STOCK=W/'build-stock/saturn_stv'
BASE=['--cart',str(W/'snapshot/diehard_main.bin'),'--cartrom',str(W/'snapshot/cart.bin')]
def environment(**extra):
 e={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))};e.update(extra);return e
ENV=environment(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',LD_PRELOAD=str((W/'clock.so').resolve()))
def run(label,binary,args,env=None):
 d=W/'verify-final'/label;d.mkdir(parents=True,exist_ok=True)
 cmd=[str(binary)]+BASE+args+['--out',str(d)]
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env or ENV,stdout=f,stderr=f,timeout=1200)
 log=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in log and 'CONVERGE FAIL' not in log,(label,r.returncode,log[-2000:])
 return d,log,{'command':cmd,'seconds':time.monotonic()-start}
def hashes(d,log):
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 trace='\n'.join(line for line in log.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples',line))+'\n'
 guest='\n'.join(line for line in log.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after',line))+'\n'
 return dict(png={p.name:sha(p) for p in sorted(d.glob('shot-*.png'))},trace_sha256=hashlib.sha256(trace.encode()).hexdigest(),guest_trace_sha256=hashlib.sha256(guest.encode()).hexdigest(),wav_sha256=sha(d/'audio.wav'))
def main():
 subprocess.run(['cc','-shared','-fPIC','work/p421/snapshot/port/converge/clock_input.c','-o',str(W/'clock.so')],check=True)
 for name in ('identity','infinite_health','frame_hud'):
  subprocess.run([str(W/'port/mods/build_mod.sh'),str(W/'port/mods/examples'/name),str(W/'data/mods'/name.replace('_','-')),str(W/'build/p421-target.txt')],check=True)
 report={'target':(W/'build/p421-target.txt').read_text().strip(),'smoke':{},'regression':{}}
 for mode,binary in [('stock',STOCK),('off',B),('identity',B)]:
  args=['--vblanks','2000','--shots','1900','--wav',str(W/'verify-final'/('smoke-'+mode)/'audio.wav')]
  if mode!='stock':args+=['--mods-dir',str(W/'data/mods'),'--mods','identity' if mode=='identity' else 'none']
  env=dict(ENV,CONVERGE_AUDIT='1',CONVERGE_COUNTS='1') if mode=='identity' else ENV
  d,log,record=run('smoke-'+mode,binary,args,env)
  md5=hashlib.md5((d/'shot-1900.png').read_bytes()).hexdigest()
  assert md5=='9bd5386ea7439621c19674381086e8fd',(mode,md5)
  report['smoke'][mode]=dict(record,md5=md5,**hashes(d,log));print('smoke',mode,md5,flush=True)
 assert len({r['wav_sha256'] for r in report['smoke'].values()})==1
 assert len({r['trace_sha256'] for r in report['smoke'].values()})==1
 reference=json.loads(Path('work/p411/regression/after/manifest.json').read_text())
 def regression(job):
  mode,name=job;ref=reference[name];label=mode+'/'+name
  args=ref['command'][1:];args=args[:]
  for flag,new in [('--cart',str(W/'snapshot/diehard_main.bin')),('--cartrom',str(W/'snapshot/cart.bin')),('--out',str(W/'verify-final'/label)),('--wav',str(W/'verify-final'/label/'audio.wav'))]:args[args.index(flag)+1]=new
  # run() adds its own identical --out after the copied original.
  args+=['--mods-dir',str(W/'data/mods'),'--mods','identity' if mode=='identity' else 'none']
  env=dict(ENV,**ref['env'])
  if mode=='identity':env.update(CONVERGE_AUDIT='1',CONVERGE_COUNTS='1')
  d,log,record=run(label,B,args,env);got=hashes(d,log)
  for key in got:assert got[key]==ref[key],(mode,name,key,got[key],ref[key])
  print(mode,name,'PASS',len(got['png']),'PNGs, traces, WAV',flush=True)
  return label,dict(record,**got)
 with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
  for name,r in pool.map(regression,[(mode,name) for mode in ('off','identity') for name in reference]):
   report['regression'][name]=r;(W/'verify-final/report.json').write_text(json.dumps(report,indent=2)+'\n')
 # Visual examples use real software SDL presentation and the existing overlay.
 for mode in ('none','infinite-health','frame-hud'):
  label='example-'+mode;d=W/'verify-final'/label;d.mkdir(parents=True,exist_ok=True)
  env=dict(ENV,STV_AUTOPLAY='100',STV_SCENETRACE='1',SATURN_WINDOW_SHOT=str((d/'window.png').resolve()),SATURN_WINDOW_SHOT_VB='1900')
  args=['--window','--no-launcher','--unpaced','--settings','-','--scale','3','--vblanks','2000','--shots','1800,1900','--dump','1900','--stvinput','99999:COIN','--mods-dir',str(W/'data/mods'),'--mods',mode]
  d,log,record=run(label,B,args,env)
  assert (d/'window.png').is_file(),label
  if mode!='none':assert 'MODDED' in log and 'mods: enabled '+mode in log
  report.setdefault('examples',{})[mode]=dict(record,window_sha256=hashlib.sha256((d/'window.png').read_bytes()).hexdigest())
  print(label,'captured',flush=True)
 # The real Mods tab, captured without a display or GPU.
 d=W/'verify-final/launcher';d.mkdir(parents=True,exist_ok=True)
 env=dict(ENV,SATURN_MODS_SHOT='1',SATURN_LAUNCHER_SHOT=str((d/'mods.png').resolve()))
 run('launcher',B,['--window','--launcher','--settings','-','--mods-dir',str(W/'data/mods'),'--mods','frame-hud,infinite-health'],env)
 assert (d/'mods.png').is_file()
 (W/'verify-final/report.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: stock/off/identity smoke; 112 reference PNGs, 8 traces and WAVs; examples and Mods tab captured',flush=True)
if __name__=='__main__':main()
