/* enemy_action_select_descriptor_motion [0x02240622]: Actor+180 descriptor, kind-table and actor state select motion/response. */
#include "dh_work.h"
#include "dh.h"
/* 0x02240622 - func_02240622: large per-state actor handler (blob sibling of
 * func_022402c0). real fn [0x240622, 0x24078e) - prologue pushes r8..r14, one
 * stack local; internal literal pool at 0x2406a0..0x2406ce and 0x24078e..;
 * bounded at the shared rts tail (0x240748..0x24078a).
 *   p   = (void *)M32(obj,0x180);
 *   sel = g_0608d824[(u8)M8(obj,0x49)];            // s32 table
 *   kind= M8(obj,0x4A);                            // signed
 *   near= (M16(obj,0x64) >= M16(obj,0x146));       // 0/1
 *   if ((M32(obj,0xE0) & 0x2000000) ||
 *       ((M32(obj,0x38) & 0x7800) && (func_02210020() & 7) == 0)) {
 *       if (!(kind >= 1 && kind <= 2)) { func_022402c0(obj,out); return; }
 *       M16(obj,0x5C) = M16(p,8);  M32(obj,0xE0) |= 0x2000000;
 *   } else {
 *       if (kind < 1 || kind > 2 || sel == 1) { func_022402c0(obj,out); return; }
 *       b   = (void *)M32(g_060995b0[kind],8);
 *       neg = (M32(b,8) - M32(obj,8)) < 0;         // cmp/pz on the difference
 *       if (M8(obj,0x44) != neg ||
 *           (near != 0 && (func_02210020() & 3) != 0)) { func_022402c0(obj,out); return; }
 *       M16(obj,0x5C) = M16(p, near*2);
 *   }
 *   // commit (0x240748):
 *   func_0224174e(obj);
 *   if (g_060e0ddc != 0) sound_queue_push((g_0608da40[near] << 8) | 0xA00000FF);
 *   M16(obj,0xBA) |= 0x100; M32(obj,0x84) = 0x800; *out = 0;
 * fidelity: HIGH - matches m2c exactly. func_02210020() is the no-arg rng; both
 * rng calls are side-effecting and preserved under short-circuit order. 0xE0/0x38
 * read mov.l; 0x49 is an unsigned byte table index; 0x4A/0x44 signed. */
extern s32  game_random_next(void);                     /* 0x02210020 rng, no args */
extern void sound_queue_push(s32 cmd);                  /* 0x02215f6c sound_queue_push */
extern void actor_follow_target_depth(void *obj);                /* 0x0224174e */
extern void actor_check_expiry_or_lock(void *obj, s32 *out);       /* 0x022402c0 (tail) */
extern s32  g_0608d824[];                            /* s32 table, idx (u8)obj->0x49 */
extern s16  g_0608da40[];                            /* s16 table, idx 'near' */
extern s32  g_sound_effects_enabled;                              /* s32 gate flag */
extern void *g_actor_slot_tasks_legacy[];                           /* pointer table, idx obj->0x4A */

void enemy_action_select_descriptor_motion(void *obj, s32 *out)
{
    void *p    = ACTOR_MP(obj, reaction_descriptor);
    s32   sel  = g_0608d824[MU8(obj, 0x49)];
    s32   kind = ACTOR_M8(obj, hit_source_slot);
    s32   near = (M16(obj, 0x64) >= M16(obj, 0x146));

    if ((ACTOR_M32(obj, state_flags) & 0x02000000) ||
        ((ACTOR_M32(obj, render_flags) & 0x7800) && (game_random_next() & 7) == 0)) {
        if (!(kind >= 1 && kind <= 2)) {
            actor_check_expiry_or_lock(obj, out);
            return;
        }
        ACTOR_M16(obj, animation_requested) = M16(p, 8);
        ACTOR_M32(obj, state_flags) |= 0x02000000;
    } else {
        void *b;
        s32   neg;
        if (kind < 1 || kind > 2 || sel == 1) {
            actor_check_expiry_or_lock(obj, out);
            return;
        }
        b   = (void *)(uintptr_t)TASK_S32(g_actor_slot_tasks_legacy[kind], work);
        neg = ((M32(b, 8) - ACTOR_M32(obj, position_x)) < 0);
        if (ACTOR_M8(obj, facing_right) != neg ||
            (near != 0 && (game_random_next() & 3) != 0)) {
            actor_check_expiry_or_lock(obj, out);
            return;
        }
        ACTOR_M16(obj, animation_requested) = M16(p, near * 2);
    }

    /* commit (0x240748) */
    actor_follow_target_depth(obj);
    if (g_sound_effects_enabled != 0)
        sound_queue_push(((s32)g_0608da40[near] << 8) | DH_SOUND_CUE_BASE);
    ACTOR_M16(obj, type_flags) |= 0x100;
    ACTOR_M32(obj, action_category) = 0x800;
    *out = 0;
}
