/* enemy_action_recorded_random_face [0x02240bba]: Recorded angle selects facing before random-angle response. */
#include "dh.h"
/* 0x02240bba - func_02240bba: per-state actor setup w/ facing from a recorded
 * delta angle (blob sibling of func_022402c0). real fn [0x240bba, 0x240c3c) -
 * entry is two scratch loads before the register push; internal pool at
 * 0x240bf2..0x240c30; bounded at the shared bra-tail to func_022413e6 (0x240c32).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   angle = (u16)rec_delta_angle_neg(obj);
 *   if (angle >= 0x4000 && angle < 0xC000) { M8(obj,0x44)=0; M16(obj,0x26)=0xC000; }
 *   else                                   { M8(obj,0x44)=1; M16(obj,0x26)=0x4000; }
 *   func_022413e6(obj, out);                        // random-angle facing tail
 * fidelity: HIGH - matches m2c exactly (same facing split as func_02240588/ae0,
 * with the func_022413e6 tail). angle is zero-extended, so the compares are
 * unsigned over [0,0xffff]. */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern s32  rec_delta_angle_neg(void *obj);          /* 0x02244df2 */
extern void actor_enter_random_launch(void *obj, s32 *out);      /* 0x022413e6 (tail) */

void enemy_action_recorded_random_face(void *obj, s32 *out)
{
    u16 angle;
    actor_apply_damage(obj, M16(obj, 0x64));
    angle = (u16)rec_delta_angle_neg(obj);
    if (angle >= 0x4000 && angle < 0xC000) {
        M8(obj, 0x44) = 0;
        M16(obj, 0x26) = (s16)0xC000;
    } else {
        M8(obj, 0x44) = 1;
        M16(obj, 0x26) = 0x4000;
    }
    actor_enter_random_launch(obj, out);
}
