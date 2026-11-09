/* enemy_update_wait [0x02249a04]: Applies hit or animation-ready idle transitions before advancing animation. */
#include "dh.h"
/* 0x02249a04  actor handler (minimal): hit-interrupt or hazard kick, then animate.
 * ABI (r4=task, r5=entity). Same family as 0x24852c / 0x248104.
 *   if (entity[0x4c] != 0)      func_022489d0(entity);     (hit interrupt)
 *   else if (entity[0x54] & 2)  func_022469f2(entity);     (hazard kick)
 *   animation_update(task, entity);                        (braf tail, resolved)
 * func_022489d0 is actor_hit_react (0x2489d0). fidelity: HIGH - hand-traced vs
 * m2c_fn --asm. */
extern void actor_enter_hit_reaction(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_wait(void *task, void *entity) {
    if (M8(entity, 0x4C) != 0) {
        actor_enter_hit_reaction(entity);
    } else if (MU16(entity, 0x54) & 2) {
        enemy_reset_idle_or_walk(entity);
    }
    actor_anim_frame_update(task, entity);
}
