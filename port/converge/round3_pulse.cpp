#include "p_dh_funcs.h"
#include "bridge.h"
#include "round3_pulse.h"
extern "C" uint32_t pulse_read(uint32_t a,int n) {
    return n==1 ? uint32_t(int32_t(int8_t(ld8(a)))) : ld32(a);
}
extern "C" void pulse_write(uint32_t a,int n,uint32_t v) {
    if(n==1)st8(a,v);else st32(a,v);
}
extern "C" void pulse_boundary(void *p,const PulseState *s,unsigned phase,uint32_t tick) {
    auto& c=*(SH2Context*)p;
    if(phase==5) {c.r[2]=s->sample;c.t=0;return;}
    if(phase==6) {c.r[0]=s->sample;c.t=0;return;}
    c.r[7]=s->low;c.r[13]=s->total;
    if(!phase) {c.r[3]=tick;return;}
    if(phase==1) {c.r[2]=s->sample;c.t=1;c.cont=5560;}
    else {
        c.r[4]=s->run;
        if(phase==2) {c.r[0]=s->sample;c.t=1;c.cont=5561;}
        else {c.r[0]=s->gate;c.t=s->gate==1;}
        if(phase==3) {c.cont=5562;c.cont_k=1;}
    }
    if(phase<4) {
        c.cont_k=bool(c.cont_k);
        SH2_POLL(c);
    } else {
        c.r[1]=0x060e0bc8;c.r[2]=0;c.r[3]=0x0609869c;
        c.r[12]=ld32(c.r[15]);c.r[13]=ld32(c.r[15]+4);c.r[14]=ld32(c.r[15]+8);
        c.r[15]+=12;c.pc=c.pr;
    }
}
namespace converge {
void poll_pulse(SH2Context& c,unsigned index) {
    unsigned resume=c.cont_entry;c.cont_entry=0;
    bool suspended=resume>=5560 && resume<=5562;
    if(!suspended) {
        record(index,c.cpu);
        st32(c.r[15]-4,c.r[14]);st32(c.r[15]-8,c.r[13]);st32(c.r[15]-12,c.r[12]);
        c.r[15]-=12;
        c.r[5]=c.r[7]=c.r[13]=0;c.r[2]=0x060e0bc8;c.r[3]=1;
        c.r[4]=0x060987bc;c.r[14]=0x060986ae;c.r[6]=0x060986af;c.r[12]=0x060986a4;
    }
    PulseState s{c.r[7],c.r[13],c.r[4],0,0,suspended ? resume-5559 : 0};
    converge_pulse(&c,&s);
}
}
