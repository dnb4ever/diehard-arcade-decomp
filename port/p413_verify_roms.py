#!/usr/bin/env python3
"""Check the shared ROM inflater against zlib and actual zip/loose/prepared sets."""
import ctypes,hashlib,json,os,random,shutil,subprocess,time,zipfile,zlib
from pathlib import Path
os.chdir(Path(__file__).resolve().parent.parent)
out=Path('work/p413/roms');out.mkdir(parents=True,exist_ok=True)
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O2','-fPIC','-shared','-DP413_PROBE_INTERNALS','port/p413_roms_probe.c','-o',str(out/'probe.so')],check=True)
lib=ctypes.CDLL(str((out/'probe.so').resolve()))
lib.p413_inflate_probe.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t]
lib.p413_sha_probe.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p]
lib.dha_prepare_roms.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.c_void_p,ctypes.c_size_t]
lib.dha_roms_ready.argtypes=[ctypes.c_char_p]
rng=random.Random(413);cases=0;bad=0
for raw in [b'',b'abc',b'a'*65536,bytes(range(256))*1000,rng.randbytes(65536),(b'abcd'*256+b'xyz')*100]:
 for level in [0,1,6,9]:
  for strategy in [zlib.Z_DEFAULT_STRATEGY,zlib.Z_FIXED,zlib.Z_HUFFMAN_ONLY]:
   enc=zlib.compressobj(level,zlib.DEFLATED,-15,8,strategy);data=enc.compress(raw)+enc.flush()
   target=ctypes.create_string_buffer(len(raw)+1)
   assert lib.p413_inflate_probe(data,len(data),target,len(raw))==1,(len(raw),level,strategy)
   assert target.raw[:len(raw)]==raw
   cases+=1
   if len(data)>1:
    assert lib.p413_inflate_probe(data,len(data)//2,target,len(raw))==0
    bad+=1
   if raw:
    assert lib.p413_inflate_probe(data,len(data),target,len(raw)-1)==0
    bad+=1
for raw in [b'',b'abc',b'a'*1000000]:
 hex=ctypes.create_string_buffer(65);lib.p413_sha_probe(raw,len(raw),hex)
 assert hex.value.decode()==hashlib.sha256(raw).hexdigest()
start=time.monotonic();checks={}
def prep(name,source,success=True):
 d=out/name;d.mkdir(exist_ok=True);error=ctypes.create_string_buffer(512)
 result=lib.dha_prepare_roms(os.fsencode(source),os.fsencode(d),error,len(error))
 assert (result==1)==success,(name,result,error.value)
 if success:
  assert lib.dha_roms_ready(os.fsencode(d))==1
  assert hashlib.sha256((d/'cart.bin').read_bytes()).digest()==hashlib.sha256(Path('cart.bin').read_bytes()).digest()
  assert (d/'diehard_main.bin').read_bytes()==Path('port/diehard_main.bin').read_bytes()
 checks[name]=dict(result=result,message=error.value.decode())
prep('zip','diehard.zip')
folder=out/'zip-folder';folder.mkdir(exist_ok=True)
if not (folder/'diehard.zip').exists():os.link('diehard.zip',folder/'diehard.zip')
prep('folder',folder)
loose=out/'loose';loose.mkdir(exist_ok=True)
with zipfile.ZipFile('diehard.zip') as z:
 for n in ['fpr19119.13','mpr19115.2','mpr19116.3','mpr19117.4','mpr19118.5']:(loose/n).write_bytes(z.read(n))
prep('loose-output',loose)
prep('prepared',out/'zip')
prep('replace',out/'zip');prep('replace',out/'zip')
# A changed member must fail the supported-set SHA before creating either output.
p=loose/'mpr19117.4';b=bytearray(p.read_bytes());b[123]^=1;p.write_bytes(b)
prep('bad-member',loose,False)
assert not (out/'bad-member/cart.bin').exists()
p=out/'truncated.zip';p.write_bytes(Path('diehard.zip').read_bytes()[:100]);prep('truncated',p,False)
p=out/'zip/diehard_main.bin';b=p.read_bytes();p.write_bytes(b[:-1]);assert lib.dha_roms_ready(os.fsencode(out/'zip'))==0;p.write_bytes(b)
(out/'report.json').write_text(json.dumps(dict(deflate_cases=cases,malformed_or_capacity_checks=bad,sha_vectors=3,sets=checks,seconds=time.monotonic()-start),indent=2)+'\n')
print('PASS:',cases,'zlib-oracle cases;',bad,'malformed/capacity checks; 3 SHA vectors; zip/folder/loose/prepared/replacement/corruption cases')
