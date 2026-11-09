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
    w(count*32,0x8000);vdp1_reg_write(4,1,2);vdp1_reg_write(2,3,2);vdp1_vblank_out();vdp1_p423_field();
}
static void r2(int a,unsigned v){g_vdp2_regs[a]=v>>8;g_vdp2_regs[a+1]=v;}
static void v2(int a,unsigned v){g_vdp2_vram[a]=v>>8;g_vdp2_vram[a+1]=v;}
static void v4(int a,unsigned v){v2(a,v>>16);v2(a+2,v);}
static void reset_bg(){vdp1_p423_hide();vdp2_p423_reset();std::memset(g_vdp2_regs,0,sizeof g_vdp2_regs);std::memset(g_vdp2_vram,0,sizeof g_vdp2_vram);std::memset(g_vdp2_cram,0,sizeof g_vdp2_cram);r2(0,0x8000);}
static void background_tests(){
    reset_bg();r2(0x20,1);r2(0x28,0x12);r2(0x78,1);r2(0x7c,1);r2(0xf8,1);
    for(int y=0;y<256;++y)g_vdp2_vram[y*512+10]=1;
    g_vdp2_cram[2]=3;g_vdp2_cram[3]=0xe0;
    vdp2_p423_field();r2(0x70,10);vdp2_p423_field();
    State a;vdp2_state(a);check(vdp2_p423_prepare(.5f),"NBG scroll pair accepted");
    vdp2_p423_enable(true);Frame f;vdp2_compose(f);vdp2_p423_enable(false);
    check(f.px[5]==0x00ff00&&f.px[0]==0,"NBG midpoint samples scroll 5, not 0 or 10");
    State b;vdp2_state(b);check(a.data==b.data,"VDP2 bytes unchanged by composition");
    // Same motion delivered through the compact per-line horizontal table.
    reset_bg();r2(0x20,1);r2(0x28,0x12);r2(0x78,1);r2(0x7c,1);r2(0xf8,1);r2(0x9a,2);r2(0xa2,0x10000/2);
    for(int y=0;y<128;++y)g_vdp2_vram[y*512+10]=1;
    g_vdp2_cram[2]=3;g_vdp2_cram[3]=0xe0;
    vdp2_p423_field();for(int y=0;y<256;++y)v4(0x10000+y*4,10u<<16);vdp2_p423_field();
    check(vdp2_p423_prepare(.5f),"line scroll pair accepted");vdp2_p423_enable(true);vdp2_compose(f);vdp2_p423_enable(false);
    check(f.px[5]==0x00ff00,"line scroll midpoint samples 5");
    // Rotation matrix and Mx move the plane boundary from 512 to 502.
    reset_bg();r2(0x20,16);r2(0x38,0x8000);r2(0xfc,1);r2(0xbe,0x4000);
    for(int pair=0;pair<8;++pair)r2(0x50+pair*2,16|(17<<8));
    for(int off:{0x10,0x14,0x1c,0x2c})v4(0x8000+off,0x10000);
    for(int slot=0;slot<2;++slot){unsigned chr=0x200+slot;std::memset(g_vdp2_vram+chr*32,0x11,32);for(int cell=0;cell<64;++cell)v2(0x20000+slot*0x2000+cell*2,(slot<<12)|chr);g_vdp2_cram[(slot*16+1)*2+1]=slot+1;}
    v4(0x8044,400u<<16);vdp2_p423_field();v4(0x8044,410u<<16);vdp2_p423_field();
    check(vdp2_p423_prepare(.5f),"rotation Mx pair accepted");vdp2_p423_enable(true);vdp2_compose(f);vdp2_p423_enable(false);
    check(f.px[106]==0x080000&&f.px[107]==0x100000,"rotation boundary at midpoint Mx=405");
    // Exercise smoothly changing perspective coefficients at all sampled rows.
    r2(0xb4,1);r2(0xb6,1);v4(0x8058,1u<<16);for(int y=0;y<=255;++y)v4(0x40000+y*4,1u<<16);vdp2_p423_field();
    for(int y=0;y<=255;++y)v4(0x40000+y*4,0x12000);vdp2_p423_field();
    check(vdp2_p423_prepare(.5f),"coefficient pair accepted");vdp2_p423_enable(true);vdp2_compose(f);vdp2_p423_enable(false);
    // With Mx=410, the midpoint coefficient 1.0625 crosses source 512 at x=96.
    check(f.px[95]==0x080000&&f.px[96]==0x100000,"perspective coefficient midpoint boundary");
    g_vdp2_cram[3]^=1;vdp2_p423_field();check(!vdp2_p423_prepare(.5f)&&vdp2_p423_reason()==2,"palette change snaps");
    vdp2_p423_field();g_vdp2_vram[0x40000]^=0x80;vdp2_p423_field();check(!vdp2_p423_prepare(.5f),"coefficient control flag change snaps");
    vdp2_p423_reset();check(!vdp2_p423_prepare(.5f),"reset invalidates endpoints");
    std::puts("PASS NBG/line/RBG/coefficient midpoints, VDP2 state isolation, palette/flag/reset guards");
}
int main() {
    setenv("STV_P423_CAPTURE","1:1",1);g_cfg.headless=true;vdp1_init();vdp1_reg_write(0,1,2);
    frame(40);frame(50);
    State before;vdp1_state(before);
    check(vdp1_p423_render(.5f),"unique moving polygon matches");
    uint16_t dot=0;check(vdp1_p423_dot(45,50,dot) && dot==0x22,"midpoint polygon starts at 45");
    check(vdp1_p423_dot(44,50,dot) && dot==0,"midpoint preserves outside edge");
    check(vdp1_dot(45,50,false)==0 && vdp1_dot(50,50,false)==0x22,"hardware still contains real endpoint");
    State after;vdp1_state(after);check(before.data==after.data,"all serialized VDP1 state identical after replay");
    frame(60,2);check(!vdp1_p423_render(.5f),"command-count ambiguity falls back");
    check(!vdp1_p423_dot(60,50,dot),"fallback uses real hardware, not a reconstructed endpoint");
    frame(65,2);check(!vdp1_p423_render(.5f),"identical moving commands reject ambiguous identity");
    background_tests();
    std::puts("PASS midpoint, hardware/state isolation, count and identity ambiguity fallback");
}
