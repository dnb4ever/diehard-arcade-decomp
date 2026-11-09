/* prop_tick_short_rotation [0x02233178]: Initializes model 64, rotates heading for sixteen frames and draws. */
#include "dh.h"
/* 0x02233178 (leafC) - projectile state machine; state=*(s8)(arg0+16), obj=*(arg0+8).
 * state 0: (obj+48)=64 (word); (obj+8)=0,(obj+12)=0x4000,(obj+16)=0x10000 (long);
 *   (obj+38)=0x4000 (word); state++; fall through to state 1.
 * state 1: (obj+38)-=0x400 (word); (obj+50)++ (word); if (obj+50)>15 state++;
 *   tail-call func_022154fc(obj,0x8000,0x8000,4).
 * state 2: (obj+50)++ (word); if (obj+50)>105 clear *arg0; same tail-call.
 * default: same tail-call.  braf cart 0x2154fc -> RAM 0x060194fc.
 * fidelity: HIGH - fields 38/48/50 16-bit (mov.w); 8/12/16 32-bit (mov.l); state 8-bit. */
extern void sprite_draw_billboard(void *, s32, s32, s32);
void prop_tick_short_rotation(void *arg0) {
    void *o = MP(arg0, 0x8);
    s8 state = M8(arg0, 16);
    if (state == 0) {
        M16(o, 48) = 64;
        M32(o, 8)  = 0;
        M32(o, 12) = 0x4000;
        M32(o, 16) = 0x10000;
        M16(o, 38) = 0x4000;
        M8(arg0, 16) = (s8)(M8(arg0, 16) + 1);
    }
    if (state == 0 || state == 1) {
        M16(o, 38) = (s16)(M16(o, 38) - 0x400);
        M16(o, 50) = (s16)(M16(o, 50) + 1);
        if ((s16)M16(o, 50) > 15)
            M8(arg0, 16) = (s8)(M8(arg0, 16) + 1);
        sprite_draw_billboard(o, 0x8000, 0x8000, 4);
        return;
    }
    if (state == 2) {
        M16(o, 50) = (s16)(M16(o, 50) + 1);
        if ((s16)M16(o, 50) > 105)
            M32(arg0, 0) = 0;
    }
    sprite_draw_billboard(o, 0x8000, 0x8000, 4);
}
