/* projectile_tick_owner_phase [0x0223033e]: Owner task phase/disable gate, position integration and billboard draw before continuation. */
#include "dh.h"
/* 0x0223033e (callC) - obj=*(arg0+8), src=*(arg0+20).
 * If *(s8)(src+15) == *(s32)(obj+116)-1 AND *(s8)(src+18) == 0: clear *arg0 (abort).
 * If (obj+12) > 0x500: advance (obj+8)+=(obj+20), (obj+12)+=(obj+24),
 *   call func_022154fc(obj, 0x20000, 0x10000, 1), (obj+90)++ (word),
 *   set *arg0 = resume ptr 0x060343A6 (next step = func_022303a6 at runtime).
 * Else: clear *arg0.
 * bsrf cart 0x2154fc -> RAM 0x060194fc.
 * fidelity: MED - control flow faithful; (obj+116/12/8/20/24) 32-bit, (obj+90) 16-bit, src 8-bit. */
extern void sprite_draw_billboard(void *, s32, s32, s32);
void projectile_tick_owner_phase(void *arg0) {
    void *o = MP(arg0, 0x8);
    void *src = MP(arg0, 20);
    if (M8(src, 15) == M32(o, 116) - 1 && M8(src, 18) == 0)
        M32(arg0, 0) = 0;
    if (M32(o, 12) > 0x500) {
        M32(o, 8)  += M32(o, 20);
        M32(o, 12) += M32(o, 24);
        sprite_draw_billboard(o, 0x20000, 0x10000, 1);
        M16(o, 90) = (s16)(M16(o, 90) + 1);
        M32(arg0, 0) = (s32)0x060343A6;
    } else {
        M32(arg0, 0) = 0;
    }
}
