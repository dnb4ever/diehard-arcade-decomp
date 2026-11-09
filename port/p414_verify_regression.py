#!/usr/bin/env python3
"""Require all P4.11 reference PNG, guest/full-trace and WAV hashes to remain identical."""
import hashlib,json,os,re,subprocess,time
from pathlib import Path
os.chdir(Path(__file__).resolve().parent.parent)
reference=json.loads(Path('work/p411/regression/after/manifest.json').read_text())
out=Path('work/p414/regression');out.mkdir(parents=True,exist_ok=True);report={}
sha=lambda b:hashlib.sha256(b).hexdigest()
for name,ref in reference.items():
 d=out/name;d.mkdir(exist_ok=True);cmd=ref['command'].copy();cmd[0]='port/run/P46/build-sdl/saturn_stv'
 cmd[cmd.index('--out')+1]=str(d);cmd[cmd.index('--wav')+1]=str(d/'audio.wav')
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))};env.update(ref['env'])
 start=time.monotonic()
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=1200)
 text=(d/'run.log').read_text();assert r.returncode==0 and 'FATAL' not in text,name
 trace='\n'.join(line for line in text.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples',line))+'\n'
 guest='\n'.join(line for line in text.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after',line))+'\n'
 png={n:sha((d/n).read_bytes()) for n in ref['png']};assert png==ref['png'],(name,'PNG')
 assert sha(trace.encode())==ref['trace_sha256'],(name,'full trace')
 assert sha(guest.encode())==ref['guest_trace_sha256'],(name,'guest trace')
 assert sha((d/'audio.wav').read_bytes())==ref['wav_sha256'],(name,'WAV')
 report[name]=dict(command=cmd,seconds=time.monotonic()-start,png=png,trace_sha256=sha(trace.encode()),guest_trace_sha256=sha(guest.encode()),wav_sha256=ref['wav_sha256'])
 (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
 print(name,'PASS',len(png),'PNGs, traces and WAV;',round(report[name]['seconds'],3),'s',flush=True)
print('PASS: 56/56 P4.11 PNGs, all four guest/full traces and WAVs identical')
