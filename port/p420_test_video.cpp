#include "saturn.h"
#include "video.h"
#include "state.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char* what) {if(!ok) {std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
static void w(int at,int value) {g_vdp1_vram[at]=uint8_t(value>>8);g_vdp1_vram[at+1]=uint8_t(value);}
static void frame(int x,int count=1) {
    std::memset(g_vdp1_vram,0,sizeof g_vdp1_vram);
    for(int i=0;i<count;++i) {
        int a=i*32;w(a,4);w(a+4,0xc0);w(a+6,0x22);
        for(int v=0;v<4;++v) {w(a+12+v*4,x+((v==1 || v==2)?20:0));w(a+14+v*4,(v>=2)?60:40);}
    }
    w(count*32,0x8000);vdp1_reg_write(4,1,2);vdp1_reg_write(2,3,2);vdp1_vblank_out();vdp1_p420_field();
}
int main() {
    setenv("STV_P420_CAPTURE","1:1",1);g_cfg.headless=true;vdp1_init();vdp1_reg_write(0,1,2);
    frame(40);frame(50);
    State before;vdp1_state(before);
    check(vdp1_p420_render(.5f),"unique moving polygon matches");
    uint16_t dot=0;check(vdp1_p420_dot(45,50,dot) && dot==0x22,"midpoint polygon starts at 45");
    check(vdp1_p420_dot(44,50,dot) && dot==0,"midpoint preserves outside edge");
    check(vdp1_dot(45,50,false)==0 && vdp1_dot(50,50,false)==0x22,"hardware still contains real endpoint");
    State after;vdp1_state(after);check(before.data==after.data,"all serialized VDP1 state identical after replay");
    frame(60,2);check(!vdp1_p420_render(.5f),"command-count ambiguity falls back");
    check(!vdp1_p420_dot(60,50,dot),"fallback uses real hardware, not a reconstructed endpoint");
    frame(65,2);check(!vdp1_p420_render(.5f),"identical moving commands reject ambiguous identity");
    std::puts("PASS midpoint, hardware/state isolation, count and identity ambiguity fallback");
}
