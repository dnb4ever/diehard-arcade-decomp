#include "bridge.h"
#include <cstdio>
#include <cstdlib>
extern "C" uint32_t dt_write(uint32_t,int,uint32_t);
static unsigned mode;
#ifdef CONVERGE_PLAYFIELD_MOD
static constexpr uint32_t field=0x060c0054;
#else
static constexpr uint32_t field=0x06099788;
#endif
static void original(SH2Context& c,converge::Transaction& tx){tx.write(field,2,90);c.r[0]=0x81f035adu;}
static uint32_t readable(uint32_t*){
 dt_write(mode==3?field+8:field,2,mode?45:90);
 if(mode==4)dt_write(field+2,2,1);
 return 0;
}
int main(int argc,char**argv){
 mode=argc>1?std::atoi(argv[1]):0;
 SH2Context c{};c.r[15]=0x060ff000;c.r[0]=0x12345678;
 st32(0x060cf0c4,0x060c0000);
 converge::invoke(c,original,readable,0,0,mode!=2,false);
 if(c.r[0]!=0x81f035adu || ld16(field)!=(mode?45:90))return 1;
 std::puts("PASS: scoped memory mod and residual void r0");
}
