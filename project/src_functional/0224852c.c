/* enemy_update_grab_probe [0x0224852c]: Tests a strike-range player for a parent link or resumes idle before animation. */
#include "dh.h"
/* 0x0224852c  actor handler: per-tick actor FSM. Hit-interrupt -> func_022489d0;
 * otherwise probe func_022453de and, on a nonzero result, hand off to
 * func_0224910e; else optionally kick func_022469f2 (when entity[0x54] & 2) and
 * tail-call animation_update (0x0222dc2c). ABI (r4=task, r5=entity).
 * Sibling of the handlers at 0x02248104 / 0x02248436 (same prologue + hit check).
 *   if entity[0x4c] != 0:  func_022489d0(entity); animation_update(task,entity); return;
 *   p = func_022453de(entity);              (probe; overwrites r5 -> 1-arg; returns ptr)
 *   if (p != 0):  func_0224910e(entity, p); return;   (tail hand-off, no anim)
 *   if (entity[0x54] & 2):  func_022469f2(entity);
 *   animation_update(task, entity);         (braf tail, resolved)
 * The 3 reachable computed branches resolved with braf_resolve (this address is a
 * merged fn.units extent; only the first function, 0x24852c..0x248582, is handled
 * here). The dead rts epilogue after the braf tail (0x24857c..0x248582) is
 * unreachable. fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void  actor_enter_hit_reaction(void *entity);
extern void *enemy_find_player_in_strike_range(void *entity);
extern void  enemy_enter_parent_link(void *entity, void *src);
extern void  enemy_reset_idle_or_walk(void *entity);
extern void  actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_grab_probe(void *task, void *entity) {
    void *p;

    if (M8(entity, 0x4C) != 0) {
        actor_enter_hit_reaction(entity);
        actor_anim_frame_update(task, entity);
        return;
    }
    p = enemy_find_player_in_strike_range(entity);
    if (p != 0) {
        enemy_enter_parent_link(entity, p);
        return;
    }
    if (MU16(entity, 0x54) & 2) {
        enemy_reset_idle_or_walk(entity);
    }
    actor_anim_frame_update(task, entity);
}
