#include "p_dh_funcs.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static uint32_t rng=0x9469efb1u;
static uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(){
 if(!std::getenv("CONVERGE_AUDIT")) return 2;
 const SH2Func fns[]={p_dh::f_060333EA,p_dh::f_0603340E,p_dh::f_06033432,p_dh::f_06033456,p_dh::f_06045556,p_dh::f_0604558C,p_dh::f_06047FBA,p_dh::f_060171D4,p_dh::f_060171FC};
 const uint32_t edges[]={0,1,2,0xffff,0x7fff,0x8000,0xffffffff,0x80000000,0x7fffffff,59,60,61,89,90,91};
 unsigned checked=0;
 for(auto fn:fns) for(unsigned i=0;i<10000;++i){
  SH2Context c{};auto *b=(unsigned char*)&c;
  for(size_t off=0;off<sizeof c;off+=4){uint32_t v=next();std::memcpy(b+off,&v,4);}
  c.cpu=i&1;c.t&=1;c.s&=1;c.q&=1;c.m&=1;c.imask&=15;c.cont_entry=0;
  c.r[15]=0x060ff000;c.r[4]=0x06080000;c.r[5]=0x06081000;
  for(unsigned off=0;off<0x200;off+=4){st32(c.r[4]+off,next());st32(c.r[5]+off,next());}
  uint32_t v=i<225?edges[i%15]:next(),w=i<225?edges[i/15]:next();
  st16(c.r[4]+0x58,v);st32(c.r[5]+0x84,w);
  st32(c.r[4]+0x10c,v);st32(c.r[4]+0x110,w);st32(0x060987bc,w+edges[i%15]);
  // Test effect alias: counter store overlaps the flags read.
  if(i%7==0 && fn!=p_dh::f_06045556 && fn!=p_dh::f_0604558C) c.r[5]=c.r[4]+0x58-0x84;
  if(fn==p_dh::f_06045556 || fn==p_dh::f_0604558C)c.r[5]=v;
  fn(c);++checked;
 }
 std::printf("PASS: %u full-context and ordered-memory checks, nine new direct recipes; signed, wrap, threshold and alias boundaries\n",checked);
}
