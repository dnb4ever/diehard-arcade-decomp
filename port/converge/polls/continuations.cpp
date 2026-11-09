#include "p_dh_funcs.h"
#include "bridge.h"
#include "continuations.h"
#include <cstdlib>
static void root_registers(SH2Context& c,const ConvergeRoot& s){
 c.r[4]=s.input;c.r[0]=s.root;c.r[3]=s.remainder;c.r[1]=s.iterations;c.r[5]=s.top;c.r[6]=3;c.t=s.iterations==3;
}
extern "C" void converge_root_checkpoint(void *p,const ConvergeRoot *s){
 auto& c=*(SH2Context*)p;root_registers(c,*s);
 c.cont=3228; // 06017188, original taken backedge; resume after the spent poll
 // cont_x is unchanged. The generated bool local normalizes cont_k at polls.
 c.cont_k=bool(c.cont_k);SH2_POLL(c);
}
extern "C" int converge_slot_read(void *p,uint32_t i){
 auto& c=*(SH2Context*)p;c.r[4]=i;c.r[0]=i;
 // Use live RAM after each safe point; the other CPU/ISR may change occupancy.
 c.r[2]=(uint32_t)(int32_t)(int8_t)ld8(c.r[5]+i);c.t=c.r[2]==0;return !c.t;
}
extern "C" void converge_slot_checkpoint(void *p,uint32_t){
 auto& c=*(SH2Context*)p;c.cont=5859;c.cont_k=bool(c.cont_k);SH2_POLL(c);
}
extern "C" int32_t converge_slot_finish(void *p,uint32_t i){
 auto& c=*(SH2Context*)p;c.r[3]=24;c.t=(int32_t)i>=24;
 if(c.t)return -1;
 c.r[5]+=i;c.r[1]=1;st8(c.r[5],1);return i+8;
}
namespace converge {
void poll_root(SH2Context& c,unsigned index){
 unsigned e=c.cont_entry;c.cont_entry=0;
 if(e!=3228){record(index,c.cpu);c.r[0]=c.r[3]=0;c.r[1]=19;c.r[6]=3;}
 ConvergeRoot s{c.r[4],c.r[0],c.r[3],c.r[1],c.r[5]};
 uint32_t result=converge_root(&c,&s);root_registers(c,s);
 c.t=s.root&1;c.r[0]=result;c.pc=c.pr;
}
void poll_slot(SH2Context& c,unsigned index){
 unsigned e=c.cont_entry;c.cont_entry=0;
 if(e!=5859){record(index,c.cpu);c.r[4]=0xffffffffu;c.r[5]=0x060996d4u;}
 c.r[0]=converge_slot(&c,c.r[4]);c.pc=c.pr;
}
}
