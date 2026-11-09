#!/usr/bin/env python3
"""Reproduce P4.23 snapshot evidence. Run from the repository root; no network."""
import argparse,hashlib,json,os,re,struct,subprocess,threading,time,shutil
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('part',choices=['regression','authority','visual','performance','smoke']);p.add_argument('--variant',default='all');a=p.parse_args()
ROOT=Path.cwd();OUT=Path('work/p423');BIN=str(OUT/'build/saturn_stv')
sha=lambda b:hashlib.sha256(b).hexdigest()
def disk_check():
 free=shutil.disk_usage('/home').free
 if free<15*1024**3:raise RuntimeError(f'STOP: /home has {free/1024**3:.2f} GiB free (<15 GiB)')
def execute(cmd,d,env,log):
 disk_check()
 # The frozen harness always writes this file. Discard that optional log.
 hw=d/'hw-log.txt'
 if hw.exists() or hw.is_symlink():hw.unlink()
 hw.symlink_to('/dev/null')
 # Raw device fields are unnecessary for PNG/trace/WAV equivalence.
 env.pop('STV_CAPTURE_FIELDS',None)
 start=time.monotonic()
 with subprocess.Popen(cmd,env=env,stdout=log,stderr=subprocess.STDOUT) as child:
  try:
   while child.poll() is None:
    disk_check()
    if time.monotonic()-start>1800:raise TimeoutError(str(d))
    time.sleep(1)
  except BaseException:
   child.terminate()
   try:child.wait(timeout=5)
   except subprocess.TimeoutExpired:child.kill();child.wait()
   raise
  return child.returncode
def environment(extra={}):
 e={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 e.update(LD_PRELOAD=str(ROOT/'work/p415/fixed-time.so'),TZ='UTC',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software');e.update(extra);return e

def run(d,fields=2100,extra=(),options={},authority=False,shots='1200,1800,1900,2100'):
 d.mkdir(parents=True,exist_ok=True);rows=[];errors=[];env=environment(options)
 if authority:
  fifo=d/'stream';fifo.unlink(missing_ok=True);os.mkfifo(fifo)
  env.update(STV_SCENARIO_PIPE=str(ROOT/fifo),STV_SCENARIO_MEMORY='1',STV_SCENARIO_VIDEO='0')
  def reader():
   try:
    with fifo.open('rb') as f:
     def exact(n):
      b=f.read(n)
      if len(b)!=n:raise EOFError('short capture')
      return b
     while b:=f.read(4):
      r=json.loads(exact(struct.unpack('<I',b)[0]));ram=exact(r.pop('ram_bytes'));exact(r.pop('pixel_bytes'));r.pop('width');r.pop('height')
      rows.append(dict(frame=r['frame'],ram=sha(ram),semantics=r))
   except Exception as e: errors.append(repr(e))
  t=threading.Thread(target=reader,daemon=True);t.start()
 cmd=[BIN,'--cart','work/p423/port/diehard_main.bin','--cartrom','cart.bin','--internal-resolution','1','--vblanks',str(fields),'--stvinput','99999:COIN','--out',str(d),'--shots',shots,'--wav',str(d/'audio.wav')]+list(extra)
 start=time.monotonic()
 with (d/'run.log').open('w') as f:rc=execute(cmd,d,env,f)
 if authority: t.join(5);fifo.unlink();assert not t.is_alive() and not errors,(errors,len(rows))
 log=(d/'run.log').read_text();assert not rc and 'FATAL' not in log,(d,rc,log[-1000:])
 result=dict(command=cmd,options=options,seconds=time.monotonic()-start,rows=rows,wav=sha((d/'audio.wav').read_bytes()),png={f.name:sha(f.read_bytes()) for f in d.glob('shot-*.png')},samples=[line for line in log.splitlines() if 'P423' in line])
 (d/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(str(d),round(result['seconds'],3),'s',flush=True);return result

def parts(path):
 b=path.read_bytes();assert b[:8]==b'STVQUICK';at=48;out={}
 for name in ['machine','video','core','mmio','bios','stv','player','scu','scudsp','onchip','clock','smpc','stv-smpc','cdblock','cddrive','cdfs','cdpos','vdp1','vdp2','sound','scsp','scspdsp']:
  assert b[at:at+len(name)]==name.encode();at+=len(name);n=struct.unpack_from('<Q',b,at)[0];at+=8;out[name]=b[at:at+n];at+=n
 assert at==len(b);return out
if a.part=='regression':
 reference=json.loads(Path('work/p411/regression/after/manifest.json').read_text());report={}
 for name,ref in reference.items():
  d=OUT/'regression'/name;d.mkdir(parents=True,exist_ok=True);cmd=ref['command'].copy();cmd[0]=BIN;cmd[cmd.index('--cart')+1]='work/p423/port/diehard_main.bin';cmd[cmd.index('--out')+1]=str(d);cmd[cmd.index('--wav')+1]=str(d/'audio.wav')
  with (d/'run.log').open('w') as f:rc=execute(cmd,d,environment(ref['env']),f)
  log=(d/'run.log').read_text();assert rc==0 and 'FATAL' not in log
  trace='\n'.join(line for line in log.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples',line))+'\n'
  guest='\n'.join(line for line in log.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after',line))+'\n'
  png={n:sha((d/n).read_bytes()) for n in ref['png']}
  assert png==ref['png'] and sha(trace.encode())==ref['trace_sha256'] and sha(guest.encode())==ref['guest_trace_sha256'] and sha((d/'audio.wav').read_bytes())==ref['wav_sha256'],(name,'reference mismatch')
  report[name]=dict(command=cmd,png=png,trace=sha(trace.encode()),guest=sha(guest.encode()),wav=sha((d/'audio.wav').read_bytes()))
  print(name,'PASS',len(png),'PNGs, full/guest traces, WAV',flush=True)
 (OUT/'regression.json').write_text(json.dumps(report,indent=2)+'\n')
elif a.part=='authority':
 out=OUT/'authority';movie=out/'record/panel.inputs';runs={}
 cases=[('record',{'STV_AUTOPLAY':'100'},['--record-inputs',str(movie)]),('default',{},['--replay-inputs',str(movie)]),('hfr60',{'STV_P423_HZ':'60'},['--replay-inputs',str(movie)]),]
 for name,opt,extra in cases:
  d=out/name
  runs[name]=run(d,options=opt,extra=extra+['--save-at','2000','--quick-state',str(d/'state.quickstate'),'--dump','1800,2100'],authority=True)
 ref=runs['default'];report={}
 for name,r in runs.items():
  if name=='record':continue
  x,y=parts(out/'default/state.quickstate'),parts(out/name/'state.quickstate')
  report[name]=dict(fields=len(r['rows']),equal_fields=sum(u==v for u,v in zip(ref['rows'],r['rows'])),first_difference=next((u['frame'] for u,v in zip(ref['rows'],r['rows']) if u!=v),None),wav_equal=r['wav']==ref['wav'],parts_equal=[n for n in x if x[n]==y[n]],parts_different=[n for n in x if x[n]!=y[n]],hardware_equal={str(vb):(out/'default'/f'dump-{vb}.bin').read_bytes()==(out/name/f'dump-{vb}.bin').read_bytes() for vb in [1800,2100]})
  print(name,report[name],flush=True)
  if name.startswith('hfr') or name=='wide':assert report[name]['equal_fields']==2100 and report[name]['wav_equal'] and not report[name]['parts_different']
 (OUT/'authority.json').write_text(json.dumps(report,indent=2)+'\n')
elif a.part=='visual':
 for scene,index,field,limit in [('fight',None,1800,2100),('floor',9,1200,1500),('attract',None,1200,1500)]:
  for variant,options in [('hfr',{'STV_P423_HZ':'60','STV_P423_CAPTURE':f'{field}:{field+12}'})]:
   if a.variant!='all' and variant!=a.variant:continue
   d=OUT/'visual'/scene/variant;d.mkdir(parents=True,exist_ok=True);extra=[]
   if scene!='attract':options['STV_AUTOPLAY']='100'
   if index:
    ini=d/'scene.ini';ini.write_text(f'[training]\nstart_scene = {index}\n');extra=['--settings',str(ini)]
   run(d,fields=limit,options=options,extra=extra,shots=','.join(map(str,range(field,field+9))))
elif a.part=='smoke':
 r=run(OUT/'smoke',fields=2000,shots='1900',extra=['--stvinput','200:COIN,340:START1'])
 md5=hashlib.md5((OUT/'smoke/shot-1900.png').read_bytes()).hexdigest()
 assert md5=='9bd5386ea7439621c19674381086e8fd',md5
 (OUT/'smoke.json').write_text(json.dumps(dict(md5=md5,command=r['command']),indent=2)+'\n')
 print('PASS shot-1900 MD5',md5)
else:
 report={}
 for name,options in [('default',{}),('hfr60',{'STV_P423_HZ':'60'})]:
  d=OUT/'performance'/name;d.mkdir(parents=True,exist_ok=True)
  ini=d/'settings.ini';ini.write_text('[display]\nframeskip = false\nscaling = sharp\ninteger = false\n')
  cmd=['/usr/bin/time','-v','-o',str(d/'time.txt'),BIN,'--cart','work/p423/port/diehard_main.bin','--cartrom','cart.bin','--internal-resolution','1','--vblanks','2100','--stvinput','99999:COIN','--out',str(d),'--window','--no-launcher','--settings',str(ini),'--unpaced','--stats','5','--shots','1900']
  t=time.monotonic()
  with (d/'run.log').open('w') as f:rc=execute(cmd,d,environment(dict(STV_AUTOPLAY='100',**options)),f)
  assert rc==0
  report[name]=dict(command=cmd,options=options,seconds=time.monotonic()-t,time=(d/'time.txt').read_text(),log=(d/'run.log').read_text())
  print(name,round(report[name]['seconds'],3),'s',flush=True)
 (OUT/'performance.json').write_text(json.dumps(report,indent=2)+'\n')
