/* enemy_action_resolve_descriptor_facing [0x022408a0]: Descriptor at actor+180 and state-index-13 path resolve facing/continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x022408a0 - func_022408a0: facing-resolve helper for func_0224081e (blob
 * sibling of func_022402c0). real fn [0x2408a0, 0x240976) - prologue pushes
 * r13,r14 + two stack locals; bounded by the rts at 0x240972. Called via bsr,
 * and also tail-branches to func_02241164.
 *   p = (void *)M32(obj,0x180);
 *   if (M32(obj,0xE4) != 13) { func_02241164(obj,out); return; }
 *   sel  = g_0608d824[(u8)M8(obj,0x49)];
 *   kind = M8(obj,0x4A);
 *   near = (M16(obj,0x64) >= M16(obj,0x146));
 *   if (kind < 1 || kind > 2 || sel == 1) {
 *       actor_apply_damage(obj, M16(obj,0x64)); func_02241164(obj,out); return;
 *   }
 *   b = (void *)M32(g_060995b0[kind],8);
 *   if (M8(obj,0x44) != ((M32(b,8) - M32(obj,8)) < 0)) {
 *       actor_apply_damage(obj, M16(obj,0x64)); func_02241164(obj,out); return;
 *   }
 *   func_0224174e(obj);
 *   M16(obj,0x5C) = M16(p, near*2);
 *   if (g_060e0ddc != 0) sound_queue_push(0xA01249FF);
 *   M16(obj,0xBA) |= 0x100; M32(obj,0x84) = 0x800; M32(obj,0x108) = 6; *out = 0;
 * fidelity: HIGH - matches m2c exactly. actor_apply_damage runs only on the two reject
 * paths; the accept path skips it. 'near' and 'p' are stack-held across the calls. */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void sound_queue_push(s32 cmd);                  /* 0x02215f6c sound_queue_push */
extern void actor_follow_target_depth(void *obj);                /* 0x0224174e */
extern void actor_select_walk_or_random_pose(void *obj, s32 *out);      /* 0x02241164 (tail) */
extern s32  g_0608d824[];                            /* s32 table, idx (u8)obj->0x49 */
extern s32  g_sound_effects_enabled;                              /* s32 gate flag */
extern void *g_actor_slot_tasks_legacy[];                           /* pointer table, idx obj->0x4A */

void enemy_action_resolve_descriptor_facing(void *obj, s32 *out)
{
    void *p = ACTOR_MP(obj, reaction_descriptor);
    s32   sel, kind, near;
    void *b;

    if (ACTOR_M32(obj, state_index) != 13) {
        actor_select_walk_or_random_pose(obj, out);
        return;
    }
    sel  = g_0608d824[MU8(obj, 0x49)];
    kind = ACTOR_M8(obj, hit_source_slot);
    near = (M16(obj, 0x64) >= M16(obj, 0x146));
    if (kind < 1 || kind > 2 || sel == 1) {
        actor_apply_damage(obj, M16(obj, 0x64));
        actor_select_walk_or_random_pose(obj, out);
        return;
    }
    b = (void *)(uintptr_t)TASK_S32(g_actor_slot_tasks_legacy[kind], work);
    if (ACTOR_M8(obj, facing_right) != ((M32(b, 8) - ACTOR_M32(obj, position_x)) < 0)) {
        actor_apply_damage(obj, M16(obj, 0x64));
        actor_select_walk_or_random_pose(obj, out);
        return;
    }
    actor_follow_target_depth(obj);
    ACTOR_M16(obj, animation_requested) = M16(p, near * 2);
    if (g_sound_effects_enabled != 0)
        sound_queue_push(0xA01249FF);
    ACTOR_M16(obj, type_flags) |= 0x100;
    ACTOR_M32(obj, action_category) = 0x800;
    ACTOR_M32(obj, action_phase) = 6;
    *out = 0;
}
