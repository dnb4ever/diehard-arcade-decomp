#include "round4_references.h"
#include "bridge.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/resource.h>
static uint32_t rng=0x485a4931;
static uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void check(SH2Func fn,SH2Func ref,SH2Context c) {
 std::vector<uint8_t> low(g_wram_l,g_wram_l+0x100000);
 std::vector<uint8_t> before(g_wram_h,g_wram_h+0x100000);
 SH2Context expected=c;ref(expected);std::vector<uint8_t> after(g_wram_h,g_wram_h+0x100000);
 std::memcpy(g_wram_h,before.data(),before.size());fn(c);
 if(std::memcmp(g_wram_l,low.data(),low.size())||std::memcmp(&c,&expected,sizeof c)||std::memcmp(g_wram_h,after.data(),after.size()))std::abort();
}
extern "C" uint32_t dt_write(uint32_t,int,uint32_t);
static uint32_t output_address;
static void ordered_machine(SH2Context& c,converge::Transaction& tx) {
 tx.write(output_address,4,0x12345678);tx.write(output_address+2,2,0xabcd);c.pc=c.pr;c.cont_entry=0;
}
static uint32_t reordered_c(uint32_t*) {
 dt_write(output_address+2,2,0xabcd);dt_write(output_address,4,0x1234abcd);return 0;
}
static uint32_t escaped_c(uint32_t*) {dt_write(output_address+4,1,0);return 0;}
int main(int argc,char**argv) {
 if(!std::getenv("CONVERGE_AUDIT"))return 2;
 const SH2Func fns[]={p_dh::f_06018194,p_dh::f_06018284,p_dh::f_0601800C,p_dh::f_060454EE};
 const SH2Func refs[]={reference::ref_06018194,reference::ref_06018284,reference::ref_0601800C,reference::ref_060454EE};
 if(argc>1) {
  FILE* f=std::fopen(argv[1],"rb");if(!f)return 3;
  uint32_t kind;SH2Context c;
  if(std::fread(&kind,4,1,f)!=1 || kind>=4 || std::fread(&c,sizeof c,1,f)!=1 || std::fread(g_wram_h,1,0x100000,f)!=0x100000 || std::fread(g_wram_l,1,0x100000,f)!=0x100000)return 4;
  check(fns[kind],refs[kind],c);std::puts("PASS real saved-frame stack-output fixture: complete context and both RAM banks");return 0;
 }
 FILE* rom=std::fopen("work/converge/round4/snapshot/cart.bin","rb");if(!rom)return 6;
 std::fseek(rom,0x200000,SEEK_SET);if(std::fread(g_wram_h+0x4000,1,0x8de20,rom)!=0x8de20)return 7;std::fclose(rom);
 unsigned count=0;
 for(unsigned kind=0;kind<4;++kind)for(unsigned i=0;i<600;++i) {
  SH2Context c{};auto*b=(uint8_t*)&c;
  for(unsigned o=0;o<sizeof c;o+=4){uint32_t v=next();std::memcpy(b+o,&v,4);}
  c.cpu=i&1;c.cont_entry=0;c.t&=1;c.r[15]=0x060ff000;
  c.r[4]=0x06080000;c.r[5]=(i%5)<<8;c.r[6]=c.r[15]+(i%8)*4;c.r[7]=c.r[6]+(i%3==0?4:16);
  for(unsigned off=0;off<256;off+=4)st32(0x06080000+off,next());
  for(unsigned off=0;off<2048;off+=2)st16(0x06053570+off,next());
  st16(0x0608672c,next());st16(0x0608672e,next());
  // Alias outputs with one another and with inputs; observed order is compared
  // against original ROM-derived stores, including spill bytes below entry SP.
  if(i%4==0)c.r[6]=c.r[4]+16;
  if(i%4==1)c.r[6]|=0x20000000; // cache-through output alias
  if(i%4==2)c.r[6]+=0x100000; // high-RAM mirror output alias
  if(kind==3)c.r[5]=i%4==0?c.r[4]+0x60:c.r[15]+(i%8)*4;
  check(fns[kind],refs[kind],c);++count;
 }
 const SH2Func direct[]={p_dh::f_06017D24,p_dh::f_06018818,p_dh::f_06017FB4,p_dh::f_06019000,p_dh::f_06049E58,p_dh::f_06017F90,p_dh::f_06017D90};
 for(unsigned k=0;k<7;++k)for(unsigned i=0;i<2000;++i) {
  SH2Context c{};auto*b=(uint8_t*)&c;for(unsigned o=0;o<sizeof c;o+=4){uint32_t v=next();std::memcpy(b+o,&v,4);}
  c.cpu=i&1;c.cont_entry=0;c.t&=1;c.r[15]=0x060ff000;c.r[4]=0x06080000;c.r[5]=0x06081000;c.r[6]=0x06082000;c.r[8]=0x06083000;c.r[9]=i%6;
  for(unsigned off=0;off<0x4000;off+=4)st32(0x06080000+off,next());
  for(unsigned off=0;off<4;++off)st8(c.r[4]+off,i%4);
  st32(0x06098558,0x06084000);st32(0x06098878,0x06085000);st32(0x060cf0c4,0x06086000);
  if(k==0 && i%3==0)c.r[5]=c.r[6]+16; // overlapping quad source/destination
  direct[k](c);++count;
 }
 // The strict bus and private-stack rejections are required behavior.
 auto reject=[&](unsigned kind) {
  pid_t pid=fork();if(!pid){rlimit limit{0,0};setrlimit(RLIMIT_CORE,&limit);if(!std::freopen("/dev/null","w",stderr))_exit(7);SH2Context c{};c.r[15]=0x060ff000;c.r[4]=0x06080000;c.r[5]=c.r[15]-4;
   if(kind==0)p_dh::f_060454EE(c);
   else if(kind<5) {converge::Transaction tx;if(kind==1)tx.write(0x20000210,4,0);else if(kind==2)tx.read(0x00100000,1);else if(kind==3)tx.read(0x207,2);else tx.read(0x212,4);}
   else {output_address=c.r[15];converge::invoke(c,ordered_machine,kind==5?reordered_c:escaped_c,0,250,false,false,{{output_address,4}});}
   _exit(0);}
  int status;waitpid(pid,&status,0);if(!WIFSIGNALED(status))std::abort();
 };
 for(unsigned k=0;k<7;++k)reject(k);
 std::printf("PASS: %u stack/alias/full-context direct ABI vectors; cache/HWRAM aliases, ordered overlap mutation, exact span bounds, private spill, BIOS width/bounds/write and SMPC read checks\n",count);
}
