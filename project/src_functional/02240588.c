/* enemy_action_face_recorded_motion [0x02240588]: Damage/update then recorded angle determines facing before motion continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x02240588 - func_02240588: per-state actor setup w/ facing from a recorded
 * delta angle (blob sibling of func_022402c0). real fn [0x240588, 0x240612) -
 * bounded by the rts epilogue at 0x24060e.
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   angle = (u16)rec_delta_angle_neg(obj);          // rec_delta_angle_neg, low 16 bits
 *   if (angle >= 0x4000 && angle < 0xC000) {         // facing one half-plane
 *       M8(obj,0x44)  = 0;  M16(obj,0x26) = 0xC000;
 *   } else {
 *       M8(obj,0x44)  = 1;  M16(obj,0x26) = 0x4000;
 *   }
 *   if (M16(obj,0xBA) & 0x40) {                       // scripted-facing override
 *       M16(obj,0x5C) = g_06086304[0];
 *       M32(obj,0x84) = 0x1000;
 *       *out = 3;
 *       return;
 *   }
 *   M32(obj,0xE0) |= 0x40000;
 *   M32(obj,0x108) = 1;  M8(obj,0x49) = 1;  M32(obj,0x84) = 0x1000;
 *   *out = 1;
 *   func_0224124e(obj, out, 0);                       // facing-resolve tail
 * fidelity: HIGH - exact. angle uses extu.w (zero-extend) so the cmp/ge compares
 * are effectively unsigned over [0,0xffff]; 0xBA read mov.w, bit 0x40 tested
 * (imm<0x80, no sign-extend trap); 0x108/0x49/0x44 get the constant 1. */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern s32  rec_delta_angle_neg(void *obj);               /* 0x02244df2 */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */
extern s16  g_06086304[];                                 /* facing s16 table */

void enemy_action_face_recorded_motion(void *obj, s32 *out)
{
    u16 angle;
    actor_apply_damage(obj, M16(obj, 0x64));
    angle = (u16)rec_delta_angle_neg(obj);
    if (angle >= 0x4000 && angle < 0xC000) {
        ACTOR_M8(obj, facing_right) = 0;
        ACTOR_M16(obj, angle_y) = (s16)0xC000;
    } else {
        ACTOR_M8(obj, facing_right) = 1;
        ACTOR_M16(obj, angle_y) = 0x4000;
    }
    if (ACTOR_M16(obj, type_flags) & 0x40) {
        ACTOR_M16(obj, animation_requested) = g_06086304[0];
        ACTOR_M32(obj, action_category) = 0x1000;
        *out = 3;
        return;
    }
    ACTOR_M32(obj, state_flags) |= 0x40000;
    ACTOR_M32(obj, action_phase) = 1;
    M8(obj, 0x49) = 1;
    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 1;
    actor_select_directional_animation(obj, out, 0);
}
