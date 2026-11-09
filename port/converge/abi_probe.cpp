// Exercise direct C adapters with complete-context auditing, including live
// scratch registers and signed/overflow boundaries omitted by a C ABI test.
#include "p_dh_funcs.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

static uint32_t state=0x81a9c37du;
static uint32_t next() { state^=state<<13;state^=state>>17;state^=state<<5;return state; }
int main() {
    if (!std::getenv("CONVERGE_AUDIT")) {
        std::fprintf(stderr,"Set CONVERGE_AUDIT=1 to compare complete SH2Context state\n");return 2;
    }
    for (unsigned i=0;i<0x100000;++i) g_wram_h[i]=next();
    const std::array<SH2Func,5> functions={p_dh::f_06017190,p_dh::f_060171A8,
        p_dh::f_060171C8,p_dh::f_060425BC,p_dh::f_0604A17A};
    const uint32_t edges[]={0,1,0xffffffffu,0x80000000u,0x7fffffffu,
        0xfffffff8u,0x7ffffff8u,0x8000,0x10000,0xffff};
    unsigned checked=0;
    for (auto fn:functions) for (unsigned i=0;i<10000;++i) {
        SH2Context c{};
        auto* bytes=reinterpret_cast<unsigned char*>(&c);
        for (size_t offset=0;offset<sizeof c;offset+=4) {
            uint32_t value=next();std::memcpy(bytes+offset,&value,4);
        }
        c.cpu=i&1;c.budget=int(next()%2000)-1000;c.cont_entry=0;
        c.t&=1;c.s&=1;c.q&=1;c.m&=1;c.imask&=15;
        c.r[15]=0x060ff000u;
        if (i<100) { c.r[4]=edges[i%10];c.r[5]=edges[i/10]; }
        fn(c); // CONVERGE_AUDIT compares every context byte, not only r0.
        ++checked;
    }
    std::printf("PASS: %u complete-context direct ABI checks (5 functions x 10000)\n",checked);
    sh2_activate(&p_dh::module);
    for (unsigned i=0;i<256;++i) {
        SH2Context c{};c.r[4]=next();c.r[5]=next();c.r[13]=next();c.r[14]=next();
        c.r[15]=0x060ff000u;c.macl=next();c.pr=0x06004000u;c.budget=100000;
        SH2Context initial=c, expected=c;
        uint32_t a=(int32_t(c.r[4])<0 ? 0u-c.r[4] : c.r[4])>>8;
        uint32_t b=(int32_t(c.r[5])<0 ? 0u-c.r[5] : c.r[5])>>8;
        expected.r[4]=a*a+b*b;
        p_dh::f_06017164(expected); // The genuine remaining recompiled callee.
        p_dh::f_0601447C(c);        // Readable C magnitude -> dt_callv -> that callee.
        if (c.r[0]!=expected.r[0] || c.pc!=initial.pr || c.pr!=initial.pr ||
            c.budget!=expected.budget || c.r[15]!=initial.r[15] ||
            c.r[13]!=initial.r[13] || c.r[14]!=initial.r[14] || c.macl!=initial.macl) {
            std::fprintf(stderr,"FAIL: real tail-callee bridge vector %u\n",i);return 1;
        }
    }
    std::printf("PASS: 256 real C-to-recompiled tail calls, returns, stack and poll budgets\n");
}
