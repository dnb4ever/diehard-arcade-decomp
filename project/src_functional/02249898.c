#include "dh_work.h"
/* enemy_update_grab_lock [0x02249898]: Advances the linked effect grab/throw lock and handles hit interruption. */
#include "dh.h"
/* 0x02249898  actor grab/throw-lock handler. ABI (r4=task, r5=entity).
 * Globals: base 0x060e0e88; tableA = 0x060e0f0c (base+0x84), tableB = 0x060e0f2c
 * (base+0xa4), both s32[] indexed by an actor id.
 *
 *   if (entity[0x4c] != 0) {            (hit interrupt)
 *       func_022489d0(entity);
 *       tableB[entity[0x94]] = 1;
 *   } else switch (entity[0xd8]) {
 *     case 0:
 *       if (entity[0x54] & 2) {         (partner engaged)
 *         i = entity[0x12c];
 *         if (tableA[i] != 0 && tableB[i] == 1) {
 *           entity[0xd8]=1; entity[0x108]=5;
 *           func_02239ce8(entity, &task, i);   (r5 = &saved-task on stack)
 *           entity[0x84]=1;
 *         } else func_022469f2(entity);
 *       }
 *       break;
 *     case 1:
 *       if ((u16)entity[0x58] == entity[0x90] && entity[0x40] == 0) {
 *         func_02239fc6(entity); entity[0x108]=0; func_02245c88(entity);
 *       } else if (entity[0x54] & 2) func_022469f2(entity);
 *       break;
 *   }
 *   animation_update(task, entity);     (braf tail, resolved)
 *
 * The task argument is spilled to the stack (mov.l r4,@r15) and its ADDRESS (r15)
 * is passed to func_02239ce8, so the C passes &task. All computed branches
 * resolved with braf_resolve; func_022489d0 is actor_hit_react (0x2489d0).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void actor_enter_hit_reaction(void *entity);
extern void actor_select_effect_action(void *entity, void **ptask, s32 i);
extern void actor_apply_effect_action(void *entity);
extern void enemy_load_action_descriptor(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_grab_lock(void *task, void *entity) {
    if (ACTOR_M8(entity, hit_pending) != 0) {
        actor_enter_hit_reaction(entity);
        ((s32 *)(uintptr_t)0x060E0F2C)[ACTOR_M32(entity, selected_effect_slot)] = 1;
    } else {
        switch (ACTOR_M32(entity, behavior_phase)) {
        case 0:
            if (ACTOR_MU16(entity, animation_events) & 2) {
                s32 i = ACTOR_M32(entity, effect_target_slot);
                if (((s32 *)(uintptr_t)0x060E0F0C)[i] != 0 &&
                    ((s32 *)(uintptr_t)0x060E0F2C)[i] == 1) {
                    ACTOR_M32(entity, behavior_phase) = 1;
                    ACTOR_M32(entity, action_phase) = 5;
                    actor_select_effect_action(entity, &task, i);
                    ACTOR_M32(entity, action_category) = 1;
                } else {
                    enemy_reset_idle_or_walk(entity);
                }
            }
            break;
        case 1:
            if ((u16)ACTOR_M16(entity, animation_frame) == ACTOR_M32(entity, action_ticks) && ACTOR_M8(entity, interpolation_ticks) == 0) {
                actor_apply_effect_action(entity);
                ACTOR_M32(entity, action_phase) = 0;
                enemy_load_action_descriptor(entity);
            } else if (ACTOR_MU16(entity, animation_events) & 2) {
                enemy_reset_idle_or_walk(entity);
            }
            break;
        }
    }
    actor_anim_frame_update(task, entity);
}
