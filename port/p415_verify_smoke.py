#!/usr/bin/env python3
"""The PM's default paid-input VB1900 smoke, without autoplay or window input."""
from pathlib import Path
import hashlib,json,os,subprocess
root=Path(__file__).resolve().parent.parent;os.chdir(root)
expected=['9bd5386ea7439621c19674381086e8fd','8f1d9a1f01a2b77cd9d4f82b22b71b37','a48d6414e53a778c69cfe4fbe71ba423'];report={}
for scale,digest in enumerate(expected,1):
 d=Path(f'work/p415/smoke/{scale}x');d.mkdir(parents=True,exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))}
 cmd=['port/run/P46/build-sdl/saturn_stv','--cart','port/diehard_main.bin','--cartrom','cart.bin','--internal-resolution',str(scale),'--vblanks','2000','--shots','1900','--out',str(d)]
 with (d/'run.log').open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=600)
 assert r.returncode==0 and 'FATAL' not in (d/'run.log').read_text(),(scale,r.returncode)
 actual=hashlib.md5((d/'shot-1900.png').read_bytes()).hexdigest();assert actual==digest,(scale,actual,digest)
 report[str(scale)]=dict(command=cmd,md5=actual);print(scale,'PASS',actual,flush=True)
Path('work/p415/smoke-md5.json').write_text(json.dumps(report,indent=2)+'\n')
