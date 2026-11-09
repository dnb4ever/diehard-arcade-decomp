/* effect_tick_expanding_frames [0x02231336]: Expands sprite scale by 0x8000, advances paired frame ID and ends after eleven. */
#include "dh.h"
/* 0x02231336 (callC) - advance obj=*(arg0+8): (obj+100)+=0x8000;
 * (obj+48)=((obj+96)>>1)+0x17C (word); call func_022153f6(obj, (obj+100), 1);
 * (obj+96)++; if (obj+96)>11 clear *arg0.  bsrf cart 0x2153f6 -> RAM 0x060193f6.
 * fidelity: HIGH - (obj+96/100) 32-bit; field 48 16-bit (mov.w). */
extern void sprite_draw_child(void *, s32, s32);
void effect_tick_expanding_frames(void *arg0) {
    void *o = MP(arg0, 0x8);
    M32(o, 100) += 0x8000;
    M16(o, 48) = (s16)((M32(o, 96) >> 1) + 0x17C);
    sprite_draw_child(o, M32(o, 100), 1);
    M32(o, 96) = M32(o, 96) + 1;
    if (M32(o, 96) > 11)
        M32(arg0, 0) = 0;
}
