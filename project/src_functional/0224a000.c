/* enemy_update_script_completion [0x0224a000]: Handles hit and clash gates and returns to idle when the enemy script completes. */
#include "dh.h"
/* 0x0224a000  actor handler: pre-animation state service, then tail to
 * animation_update (0x0222dc2c). ABI (r4=task, r5=entity).
 *  if entity[0x4c] != 0        -> func_022489d0(entity)          (hit/stun branch)
 *  else if actor_flag_rng_1in4(ent)  -> func_02249a82(entity)
 *  else if !actor_script_step(ent) -> func_022469f2(entity)
 *  always: animation_update(task, entity)   [braf tail, resolved via ptr_resolve].
 * fidelity: HIGH - hand-traced vs m2c_fn --asm; all 4 bsrf + the braf tail resolved. */
extern void actor_enter_hit_reaction(void *entity);
extern s32  actor_flag_rng_1in4(void *entity);
extern void enemy_enter_clash(void *entity);
extern s32  actor_script_step(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_script_completion(void *task, void *entity) {
    if (M8(entity, 0x4C) != 0) {
        actor_enter_hit_reaction(entity);
    } else if (actor_flag_rng_1in4(entity) != 0) {
        enemy_enter_clash(entity);
    } else if (actor_script_step(entity) == 0) {
        enemy_reset_idle_or_walk(entity);
    }
    actor_anim_frame_update(task, entity);
}
