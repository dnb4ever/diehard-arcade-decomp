#!/usr/bin/env python3
"""Adversarial manifests/ZIPs, disabled constructor, memory/phase API and contracts."""
import hashlib,json,os,shutil,subprocess,zipfile
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file());os.chdir(ROOT);W=Path('work/p421');D=W/'loader-tests-final';D.mkdir(exist_ok=True)
B=W/'build/saturn_stv';SDK=W/'port/mods/sdk';TARGET=(W/'build/p421-target.txt').read_text().strip()
BASE=[str(B),'--cart',str(W/'snapshot/diehard_main.bin'),'--cartrom',str(W/'snapshot/cart.bin')]
report={};env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
def invoke(label,folder,selection='none',extra=None,ok=True,contains=None):
 out=D/label;out.mkdir(exist_ok=True);cmd=BASE+['--mods-dir',str(folder),'--mods',selection,'--out',str(out)]+(extra or ['--mods-check'])
 result=subprocess.run(cmd,env=dict(env,P421_MARKER=str((out/'marker.txt').resolve())),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=90)
 (out/'run.log').write_text(result.stdout)
 assert (result.returncode==0)==ok,(label,result.returncode,result.stdout[-2000:])
 if contains:assert contains in result.stdout,(label,result.stdout[-2000:])
 report[label]={'rc':result.returncode,'expected':contains};print(label,'PASS',flush=True);return out,result.stdout
def module(folder,id='probe',hooks='frame',body='',declarations='',abi='DH_MOD_ABI',constructor=False):
 folder.mkdir(parents=True,exist_ok=True)
 text=f'''#include "dh_mod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static void marker(const char *s) {{const char *p=getenv("P421_MARKER");if(p){{FILE *f=fopen(p,"a");if(f){{fputs(s,f);fclose(f);}}}}}}
'''
 if constructor:text+='__attribute__((constructor)) static void init(void){marker("loaded\\n");}\n'
 text+=body+'\n'+declarations+f'\nstatic const dh_mod mod={{{abi},sizeof(dh_mod),"{id}",sizeof(h)/sizeof(h[0]),h}};\nDH_EXPORT const dh_mod *dh_mod_query(void){{return &mod;}}\n'
 (folder/'mod.c').write_text(text)
 subprocess.run(['cc','-std=c11','-shared','-fPIC','-O2','-I'+str(SDK),str(folder/'mod.c'),'-o',str(folder/'mod.so')],check=True)
 manifest=f'id={id}\nname=Test {id}\nversion=1\nauthor=Test\ndescription=Loader boundary test\ntarget={TARGET}\nlibrary=mod.so\nhooks={hooks}\n'
 (folder/'manifest.ini').write_text(manifest);return manifest
basic='static uint32_t cb(const dh_api *a,const dh_call *c){(void)a;(void)c;return 0;}'
frame='static const dh_hook h[]={{DH_FRAME,0,cb,0,0,0}};'
root=D/'inventory';module(root/'probe',body=basic,declarations=frame,constructor=True)
out,_=invoke('disabled-no-dlopen',root);assert not (out/'marker.txt').exists()
out,log=invoke('enabled-loads',root,'probe');assert (out/'marker.txt').read_text()=='loaded\n'
expected=hashlib.sha256((root/'probe/manifest.ini').read_bytes()+b'\0'+(root/'probe/mod.so').read_bytes()).hexdigest();assert expected in log
invoke('unknown-enabled-id',root,'missing',ok=False,contains='missing/duplicate enabled mod')
original=(root/'probe/manifest.ini').read_text()
for name,transform,message in [
 ('wrong-target',lambda s:s.replace(TARGET,'0'*64),'Target build differs'),
 ('duplicate-key',lambda s:s+'id=probe\n','duplicate'),
 ('unsafe-library',lambda s:s.replace('library=mod.so','library=../mod.so'),'invalid manifest'),
 ('unknown-field',lambda s:s+'unexpected=value\n','unknown'),
]:
 (root/'probe/manifest.ini').write_text(transform(original));out,_=invoke(name,root,'probe',ok=False,contains=message);assert not (out/'marker.txt').exists()
(root/'probe/manifest.ini').write_text(original)
module(root/'probe',body=basic,declarations=frame,abi='999')
invoke('bad-native-abi',root,'probe',ok=False,contains='ABI/id mismatch')
module(root/'probe',body=basic,declarations=frame,hooks='replace:fixed_mul16')
invoke('manifest-library-hooks-differ',root,'probe',ok=False,contains='hook list differs')
for label,target in [('device-floor','sh2_hw_divide'),('unknown-function','game_boot_and_run'),('resumable-entry','02213164')]:
 module(root/'probe',body=basic,declarations=f'static const dh_hook h[]={{{{DH_REPLACE,"{target}",cb,0,0,0}}}};',hooks='replace:'+target)
 invoke(label,root,'probe',ok=False,contains='unqualified/resumable')
for label,address,width in [('rom-write',0x02200000,4),('device-write',0x25d00000,4),('span-overflow',0x060fffff,4),('invalid-width',0x060f0000,3)]:
 module(root/'probe',body=basic,declarations=f'static const dh_hook h[]={{{{DH_DATA_START,0,0,{address}u,{width},1}}}};',hooks='data:start')
 invoke(label,root,'probe',ok=False,contains='outside guest RAM')
# Two aliases of the same callable entry conflict before gameplay.
for id,target in [('probe','fixed_mul16'),('other','022131c8')]:
 module(root/id,id=id,body=basic,declarations=f'static const dh_hook h[]={{{{DH_REPLACE,"{target}",cb,0,0,0}}}};',hooks='replace:'+target)
invoke('alias-replacement-conflict',root,'probe,other',ok=False,contains='conflicting replacements')
# Duplicate IDs are never picked by arbitrary folder enumeration order.
shutil.copytree(root/'probe',root/'duplicate',dirs_exist_ok=True)
invoke('duplicate-id',root,'probe',ok=False,contains='duplicate')
shutil.rmtree(root/'duplicate');shutil.rmtree(root/'other')
module(root/'probe',body=basic,declarations='static const dh_hook h[]={{DH_DATA_FRAME,0,0,0x060f0000,4,1},{DH_DATA_START,0,0,0x060f0002,2,2}};',hooks='data:frame,data:start')
invoke('overlapping-patches',root,'probe',ok=False,contains='overlapping data patches')
# Runtime API uses big-endian guest bytes, canonical cache aliases and full spans.
api=r'''
static uint32_t cb(const dh_api *a,const dh_call *c){
 uint32_t v=0;unsigned char before=a->state[0];
 assert(a->abi==DH_MOD_ABI && a->size==sizeof(dh_api));
 assert(a->read(0x060f0000,4,&v) && v==0x12345678);
 assert(a->read(0x060f0000,1,&v) && v==0x12);
 assert(a->read(0x260f0002,2,&v) && v==0x5678);
 assert(a->read(0x02200000,4,&v));
 assert(!a->read(0x060fffff,2,&v));assert(!a->read(0xffffffff,4,&v));
 assert(!a->read(0x25d00000,4,&v));assert(!a->read(0x060f0000,3,&v));
 assert(!a->write(0x02200000,4,0));assert(!a->write(0x25d00000,4,0));
 assert(!a->write(0x060fffff,4,0));assert(!a->write(0x060f0000,0,0));
 assert(a->write(0x260f0004,4,0x89abcdef));assert(a->read(0x060f0004,4,&v) && v==0x89abcdef);
 a->state[0]=before+1;assert(c->frame>0);marker("api-ok\n");return 0;
}
'''
module(root/'probe',body=api,declarations='static const dh_hook h[]={{DH_DATA_FRAME,0,0,0x060f0000,4,0x12345678},{DH_FRAME,0,cb,0,0,0}};',hooks='data:frame,frame')
out,_=invoke('memory-api-and-data-frame',root,'probe',extra=['--vblanks','2']);assert (out/'marker.txt').read_text()=='api-ok\napi-ok\n'
# pre/post run in sorted registration order, observer writes are rejected, and
# the pure-result replacement refuses memory writes while its full recipe runs.
phases=r'''
static uint32_t pre(const dh_api *a,const dh_call *c){
 assert(c->address==DH_FN_fixed_mul16 && c->argc==2);assert(!a->write(0x060f0000,4,1));
 assert(a->state[0]==0);a->state[0]=1;return 0;
}
static uint32_t replace(const dh_api *a,const dh_call *c){
 assert(a->state[0]==1);a->state[0]=2;assert(!a->write(0x060f0000,4,1));
 return (uint32_t)((uint64_t)((int64_t)(int32_t)c->args[0]*(int32_t)c->args[1])>>16);
}
static uint32_t post(const dh_api *a,const dh_call *c){
 assert(a->state[0]==2);a->state[0]=0;assert(a->write(0x060f0000,4,1));
 assert(c->result==(uint32_t)((uint64_t)((int64_t)(int32_t)c->args[0]*(int32_t)c->args[1])>>16));marker("hooks-ok\n");return 0;
}
'''
module(root/'probe',body=phases,declarations='static const dh_hook h[]={{DH_PRE,"fixed_mul16",pre,0,0,0},{DH_REPLACE,"022131c8",replace,0,0,0},{DH_POST,"060171c8",post,0,0,0}};',hooks='pre:fixed_mul16,replace:022131c8,post:060171c8')
out,_=invoke('pre-replace-post-phase-contract',root,'probe',extra=['--vblanks','1000']);assert len((out/'marker.txt').read_text().splitlines())>100
# ZIPs are handled by the production in-process installer; no extraction tool.
install=D/'installed';install.mkdir(exist_ok=True)
valid=D/'valid.zip'
with zipfile.ZipFile(valid,'w',compression=zipfile.ZIP_DEFLATED) as z:
 for name in ('manifest.ini','mod.so'):z.write(root/'probe'/name,name)
invoke('install-deflate',install,extra=['--mods-install',str(valid)],contains='Installed')
assert not (install/'enabled.txt').exists()
invoke('install-no-overwrite',install,extra=['--mods-install',str(valid)],ok=False,contains='already installed')
for label,entries in [('zip-traversal',[('../outside',b'x')]),('zip-absolute',[('/tmp/outside',b'x')]),('zip-case-duplicate',[('mod.so',b'a'),('MOD.SO',b'b')]),('zip-duplicate',[('mod.so',b'a'),('mod.so',b'b')])]:
 zpath=D/(label+'.zip')
 with zipfile.ZipFile(zpath,'w') as z:
  for name,data in entries:z.writestr(name,data)
 invoke(label,install,extra=['--mods-install',str(zpath)],ok=False,contains='unsafe ZIP')
# A Unix symlink archive entry and damaged CRC both fail without any output.
zpath=D/'symlink.zip'
with zipfile.ZipFile(zpath,'w') as z:
 info=zipfile.ZipInfo('link');info.create_system=3;info.external_attr=0o120777<<16;z.writestr(info,'/tmp/outside')
invoke('zip-symlink',install,extra=['--mods-install',str(zpath)],ok=False,contains='unsafe ZIP')
bad=bytearray(valid.read_bytes());off=bad.index(b'PK\x01\x02');bad[off+16]^=1;(D/'crc.zip').write_bytes(bad)
invoke('zip-crc',install,extra=['--mods-install',str(D/'crc.zip')],ok=False,contains='CRC differs')
for n in (0,10,len(bad)//2):
 (D/'truncated.zip').write_bytes(bad[:n]);invoke('zip-truncated-'+str(n),install,extra=['--mods-install',str(D/'truncated.zip')],ok=False)
assert not list(install.glob('.install-*'))
(D/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(report),'loader/ABI/memory/phase/archive cases',flush=True)
