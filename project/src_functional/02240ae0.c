/* enemy_action_face_recorded_angle [0x02240ae0]: Recorded angle selects facing then shared facing response. */
#include "dh.h"
/* 0x02240ae0 - func_02240ae0: per-state actor setup w/ facing from a recorded
 * delta angle (blob sibling of func_022402c0). real fn [0x240ae0, 0x240b2e).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   angle = (u16)rec_delta_angle_neg(obj);            // rec_delta_angle_neg
 *   if (angle >= 0x4000 && angle < 0xC000) { M8(obj,0x44)=0; M16(obj,0x26)=0xC000; }
 *   else                                   { M8(obj,0x44)=1; M16(obj,0x26)=0x4000; }
 *   func_02241324(obj, out);                          // facing-resolve tail (bra)
 * fidelity: HIGH - exact. angle via extu.w (unsigned over [0,0xffff]). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern s32  rec_delta_angle_neg(void *obj);          /* 0x02244df2 */
extern void actor_enter_directional_launch(void *obj, s32 *out);      /* 0x02241324 (tail) */

void enemy_action_face_recorded_angle(void *obj, s32 *out)
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
    actor_enter_directional_launch(obj, out);
}
