#!/usr/bin/env python3
"""Read-only native observer measures the actual joined actor health after mods."""
import json,os,re,shutil,subprocess
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file());os.chdir(ROOT);W=Path('work/p421');D=W/'health-delivery';D.mkdir(exist_ok=True)
mods=D/'mods';shutil.copytree(W/'data/mods',mods,dirs_exist_ok=True);probe=mods/'zz-health-observer';probe.mkdir(exist_ok=True)
(probe/'mod.c').write_text(r'''
#include "dh_mod.h"
#include <stdio.h>
static uint32_t observe(const dh_api *a,const dh_call *c){
 uint32_t joined,task,actor,slot,hp;
 if(a->read(DH_PLAYER_JOIN_STATES,1,&joined) && joined==2 &&
    a->read(DH_ACTOR_TASK_SLOTS+4,4,&task) && task>=0x06004000 && task<=0x060fffe0 &&
    a->read(task+8,4,&actor) && actor>=0x06004000 && actor<=0x06100000-DH_ACTOR_SIZE &&
    a->read(actor+DH_ACTOR_SLOT_ID,1,&slot) && slot==1 && a->read(actor+DH_ACTOR_HEALTH,2,&hp))
    fprintf(stderr,"HEALTH_OBSERVER field=%llu hp=%u\n",(unsigned long long)c->frame,hp);
 return 0;
}
static const dh_hook h[]={{DH_FRAME,0,observe,0,0,0}};
static const dh_mod m={DH_MOD_ABI,sizeof(dh_mod),"zz-health-observer",1,h};
DH_EXPORT const dh_mod *dh_mod_query(void){return &m;}
''')
subprocess.run(['cc','-shared','-fPIC','-O2','-I'+str(W/'port/mods/sdk'),str(probe/'mod.c'),'-o',str(probe/'mod.so')],check=True)
(probe/'manifest.ini').write_text('id=zz-health-observer\nname=Read-only health observer\nversion=1\nauthor=Test\ndescription=Read-only acceptance observer\ntarget='+ (W/'build/p421-target.txt').read_text().strip()+'\nlibrary=mod.so\nhooks=frame\n')
report={}
for mode,selection in [('normal','zz-health-observer'),('health','zz-health-observer,infinite-health')]:
 d=D/mode;d.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_'))};env.update(STV_AUTOPLAY='100',LD_PRELOAD=str((W/'clock.so').resolve()))
 cmd=[str(W/'build/saturn_stv'),'--cart',str(W/'snapshot/diehard_main.bin'),'--cartrom',str(W/'snapshot/cart.bin'),'--mods-dir',str(mods),'--mods',selection,'--vblanks','2100','--stvinput','99999:COIN','--shots','1900','--out',str(d)]
 with (d/'run.log').open('w') as f:subprocess.run(cmd,env=env,stdout=f,stderr=f,check=True,timeout=180)
 samples=[(int(f),int(h)) for f,h in re.findall(r'HEALTH_OBSERVER field=(\d+) hp=(\d+)',(d/'run.log').read_text())]
 assert len(samples)>1000
 if mode=='health':assert all(h==114 for f,h in samples)
 else:assert min(h for f,h in samples)<114
 report[mode]={'samples':len(samples),'min_hp':min(h for f,h in samples),'max_hp':max(h for f,h in samples),'hp_at_1900':dict(samples)[1900]}
from PIL import Image,ImageChops
stock=Image.open(W/'verify-final/example-none/shot-1900.png').convert('RGB');health=Image.open(W/'verify-final/example-infinite-health/shot-1900.png').convert('RGB')
diff=ImageChops.difference(stock,health);report['pixels']={'different':sum(p!=(0,0,0) for p in diff.getdata()),'bbox':diff.getbbox(),'size':stock.size}
(D/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
