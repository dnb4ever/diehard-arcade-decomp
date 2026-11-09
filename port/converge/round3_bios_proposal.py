#!/usr/bin/env python3
"""Create (but never install) an opt-in low-BIOS oracle proposal and exercise it.

The cart ROM proves the three load addresses/widths; frozen stock bios_rom_read
supplies their immutable HLE bytes. This is not a dump of the physical ST-V BIOS.
"""
from pathlib import Path
import difflib,hashlib,json,subprocess,sys,types,struct
import round3_generate as generate
HERE=Path(__file__).resolve().parent
WORK=Path('work/converge/round3')

def proposal(source):
 old='''class GuestMemory(sh2emu.Memory):'''
 new='''# Convergence proposal: explicitly opt in the immutable HLE data span.
# Stock bios_rom_read supplies zero for physical 00000208..00000213.
# Cartridge instructions 06018A46..4A and 06018A56..5A prove these loads.
BIOS_READONLY = False
BIOS_DATA = bytes(12)
class GuestMemory(sh2emu.Memory):'''
 assert old in source;source=source.replace(old,new)
 anchor='''    def write(self, addr, size, value):
        a = ram_address(addr)
'''
 replacement='''    def read(self, addr, size):
        a = ram_address(addr)
        if BIOS_READONLY and 0x208 <= a and a+size <= 0x214 and size in (1,2,4):
            return int.from_bytes(BIOS_DATA[a-0x208:a-0x208+size], 'big')
        return super().read(addr,size)

    def write(self, addr, size, value):
        a = ram_address(addr)
        if BIOS_READONLY and a < 0x214 and a+size > 0x208:
            raise sh2emu.MemoryError_('write to read-only BIOS data')
'''
 assert source.count(anchor)==1;source=source.replace(anchor,replacement)
 # Read-only route, deliberately absent from dt_ptr / dt_write.
 anchor='''static u32 dt_read(u32 a,int n) {
'''
 replacement='''#ifndef DT_BIOS_READONLY
#define DT_BIOS_READONLY 0
#endif
#ifndef DT_BIOS_DATA
#define DT_BIOS_DATA 0
#endif
static const unsigned char dt_bios_data[12]={DT_BIOS_DATA};
static u32 dt_read(u32 a,int n) {
 u32 bios_a=(a>>28)==2 ? a&0x1fffffff : a;
 if(DT_BIOS_READONLY && bios_a>=0x208u && (uint64_t)bios_a+n<=0x214u && (n==1 || n==2 || n==4)) {
  u32 v=0;for(int i=0;i<n;++i)v=(v<<8)|dt_bios_data[bios_a-0x208+i];return v;
 }
'''
 assert source.count(anchor)==1;return source.replace(anchor,replacement)

def main():
 snap=(WORK/'snapshot').resolve();path=snap/'tools/decomp/difftest.py';original=path.read_text();patched=proposal(original)
 (HERE/'round3_oracle_bios.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),patched.splitlines(True),fromfile='a/tools/decomp/difftest.py',tofile='b/tools/decomp/difftest.py')))
 dest=WORK/'bios-proposal';dest.mkdir(exist_ok=True);(dest/'difftest_proposed.py').write_text(patched)
 dt=generate.load_oracle(snap)
 # Execute the proposed text with its original __file__: config and all ROM/C
 # inputs still come from the immutable snapshot, with no frozen file edits.
 m=types.ModuleType('converge_bios_proposed');m.__file__=str(path);exec(compile(patched,str(path),'exec'),m.__dict__)
 mem=m.GuestMemory();cases=[]
 for enabled in (False,True):
  m.BIOS_READONLY=enabled
  for a,n in ((0x208,4),(0x20000210,4),(0x20c,2),(0x213,1),(0x212,4),(0x207,2),(0x100000,1)):
   try:v=mem.read(a,n);cases.append([enabled,f'{a:08x}',n,'read',v]);assert enabled and 0x208<=(a&0x1fffffff) and (a&0x1fffffff)+n<=0x214
   except m.sh2emu.MemoryError_:cases.append([enabled,f'{a:08x}',n,'rejected'])
 for a in (0x208,0x20000210):
  try:mem.write(a,4,1);raise AssertionError('BIOS write accepted')
  except m.sh2emu.MemoryError_:cases.append([True,f'{a:08x}',4,'write rejected'])
 # Mutation-sensitive address/value proof independent of the stock zero bytes.
 m.BIOS_DATA=bytes(range(12));assert mem.read(0x2000020c,4)==0x04050607;m.BIOS_DATA=bytes(12)
 c=m.PRELUDE+'\n#define DT_BIOS_READONLY 1\n'+m.HARNESS+'''
int main(int argc,char**argv){
 if(argc>1){if(argv[1][0]=='w')dt_write(0x20000210,4,1);else dt_read(0x100000,1);return 99;}
 for(unsigned a=0x208;a<0x214;++a)for(int n=1;n<=4;n*=2)if(a+n<=0x214)
  if(dt_read(a,n)||dt_read(a|0x20000000u,n))return 1;
 return 0;
}
'''
 (dest/'model.c').write_text(c);subprocess.run(['cc','-O2',str(dest/'model.c'),'-o',str(dest/'model')],check=True)
 results={}
 for arg in ('','write','mmio'):
  r=subprocess.run([str(dest/'model')]+([arg] if arg else []),capture_output=True,text=True);results[arg or 'reads']=dict(exit=r.returncode,stderr=r.stderr);assert r.returncode==(0 if not arg else 73)
 # Original machine vector wrappers against proposed model and independently
 # lowered C: the target callee is stubbed by the oracle's normal call contract.
 symbols=json.loads((WORK/'symbols.json').read_text());vector=[]
 for data_kind,data in [('stock-hle',bytes(12)),('nonzero-fixture',bytes(range(12)))]:
  m.BIOS_DATA=data
  for addr,offset in ((0x02214a40,8),(0x02214a50,0)):
   pc=m.canonical(addr);meta={};text=(snap/f'project/src_functional/{addr:08x}.c').read_text();body,_=m.prepare(text,symbols,pc,meta)
   high=bytearray(0x100000);high[0x4000:0x91e20]=m.cart_rom()[0x200000:0x28de20];low=bytearray(0x100000)
   args=[0x20000208-offset,0,0,0]
   ref=m.reference(pc,args,high,low,0,entries=(0x06017ca4,),call_specs=meta['call_specs'])
   source=m.PRELUDE+'\n#define DT_BIOS_READONLY 1\n#define DT_BIOS_DATA '+','.join(map(str,data))+'\n'+m.HARNESS+body+m.MAIN
   cp=dest/f'{addr:08x}-{data_kind}.c';exe=dest/f'{addr:08x}-{data_kind}';cp.write_text(source)
   subprocess.run(['cc','-O2','-fwrapv','-fno-strict-aliasing',str(cp),'-o',str(exe)],check=True)
   state=dest/'state.bin';state.write_bytes(struct.pack('<4I',*args)+high+low)
   run=subprocess.run([str(exe),str(state)],capture_output=True,text=True,check=True)
   calls=[l for l in run.stdout.splitlines() if l.startswith('C ')]
   assert len(calls)==len(ref[2])==1 and [int(x,16) for x in calls[0].split()[1:]]==list(ref[2][0]),(calls,ref)
   vector.append(dict(address=f'{addr:08x}',data=data_kind,metadata=meta,reference=ref,candidate=run.stdout))

 out=dict(cart_sha256=hashlib.sha256((snap/'cart.bin').read_bytes()).hexdigest(),runtime_bios_sha256=hashlib.sha256((snap/'tools/saturn-recomp/runtime/src/machine/bios.cpp').read_bytes()).hexdigest(),rom_load_bytes={f'{pc:08x}':m.cart_rom()[0x200000+pc-0x06004000:0x200000+pc-0x06004000+10].hex() for pc in (0x06018a46,0x06018a56)},source_sha256=hashlib.sha256(original.encode()).hexdigest(),proposal_sha256=hashlib.sha256(patched.encode()).hexdigest(),python_cases=cases,c_model=results,vector_wrappers=vector,physical_bios_dump=False)
 (dest/'result.json').write_text(json.dumps(out,indent=2)+'\n');(HERE/'evidence/round3-bios-model.json').write_text(json.dumps(out,indent=2)+'\n')
 print('PASS proposed BIOS model: alias/width/bounds/write/MMIO/mutation cases and two original-ROM vector wrappers')
if __name__=='__main__':main()
