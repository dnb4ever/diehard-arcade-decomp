/* projectile_tick_owner_phase_finish [0x022303a6]: Computes lifetime from owner's phase count, draws and updates phase continuation. */
/* Round 4: 06034402 targets 060194fc, the four-argument billboard renderer; model_draw_by_matrix_mode names the invalid 060194fa entry. Evidence: work/decomp4/callee_missing.asm. */
#include "dh.h"
/* 0x022303a6 (callC) - obj=*(arg0+8), src=*(arg0+20).
 * If *(s8)(src+15) == *(s32)(obj+116)-1: (obj+88) = *(s8)(src+18) - *(s16)(obj+90) (word).
 * Decrement (obj+108). If it was !=0 AND (obj+12) > 0x500: advance
 *   (obj+8)+=(obj+20), (obj+12)+=(obj+24), call func_022154fa(obj, 0x20000, 0x10000, 1),
 *   (obj+90)++ (word).  Otherwise clear *arg0.
 * bsrf cart 0x2154fa -> RAM 0x060194fa.
 * fidelity: HIGH - (obj+116/108/12/20/24/8) 32-bit; (obj+88/90) 16-bit; src fields 8-bit. */
extern void sprite_draw_billboard(void *, s32, s32, s32);
void projectile_tick_owner_phase_finish(void *arg0) {
    void *o = MP(arg0, 0x8);
    void *src = MP(arg0, 20);
    if (M8(src, 15) == M32(o, 116) - 1)
        M16(o, 88) = (s16)(M8(src, 18) - M16(o, 90));
    M32(o, 108) = M32(o, 108) - 1;
    if (M32(o, 108) + 1 != 0 && M32(o, 12) > 0x500) {
        M32(o, 8)  += M32(o, 20);
        M32(o, 12) += M32(o, 24);
        sprite_draw_billboard(o, 0x20000, 0x10000, 1);
        M16(o, 90) = (s16)(M16(o, 90) + 1);
    } else {
        M32(arg0, 0) = 0;
    }
}
