// Dynamic identity module through the real convergence adapter, full context.
#include "p_dh_funcs.h"
#include "p421_mods.h"
#include "p410_identity.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>
static uint32_t seed=0x421c31;
static uint32_t next(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(int argc,char** argv) {
    if(argc!=2 || !std::getenv("CONVERGE_AUDIT"))return 2;
    mods_configure(STV_MOD_TARGET,argv[1],"identity");if(!mods_open())return 3;
    const uint32_t edge[]={0,1,0xffffffffu,0x80000000u,0x7fffffffu,0xfffffff8u,0x7ffffff8u,0x8000,0x10000,0xffff};
    for(unsigned i=0;i<0x100000;++i){g_wram_h[i]=uint8_t(next());g_wram_l[i]=uint8_t(next());}
    std::vector<uint8_t> high(g_wram_h,g_wram_h+0x100000),low(g_wram_l,g_wram_l+0x100000);
    for(unsigned i=0;i<20000;++i) {
        SH2Context c{};auto* bytes=(unsigned char*)&c;
        for(size_t n=0;n<sizeof c;++n)bytes[n]=uint8_t(next());
        c.cpu=i&1;c.budget=int(next()%2000)-1000;c.cont_entry=0;
        c.t&=1;c.s&=1;c.q&=1;c.m&=1;c.imask&=15;c.r[15]=0x060ff000;
        if(i<100){c.r[4]=edge[i%10];c.r[5]=edge[i/10];}
        SH2Context expected=c;
        uint64_t product=uint64_t(int64_t(int32_t(c.r[4]))*int32_t(c.r[5]));
        expected.mach=uint32_t(product>>32);expected.macl=uint32_t(product);expected.r[1]=expected.mach;
        expected.r[0]=uint32_t(product>>16);expected.pc=c.pr;expected.cont_entry=0;
        p_dh::f_060171C8(c);
        if(std::memcmp(&c,&expected,sizeof c)){std::fprintf(stderr,"context differs vector %u\n",i);return 1;}
    }
    if(std::memcmp(high.data(),g_wram_h,high.size()) || std::memcmp(low.data(),g_wram_l,low.size()))return 1;
    std::puts("PASS: 20000 dynamically loaded identity replacements; complete SH2Context, both CPUs, signed edges and both RAM banks unchanged");
}
