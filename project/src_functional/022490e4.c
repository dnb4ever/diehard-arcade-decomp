/* enemy_try_grab [0x022490e4]: Links a player found in strike range or resumes idle when none is eligible. */
#include "dh.h"
/* 0x022490e4  actor_try_grab: probe for a grab/interaction target and dispatch.
 * Same probe->hand-off idiom as the tail of 0x0224852c, packaged as a predicate.
 * ABI: single arg r4 = entity; returns 1 if a target was found (and handed to
 * func_0224910e), else 0 (after a func_022469f2 fallback).
 *
 *   p = func_022453de(entity);          (probe; overwrites r5 -> 1-arg; ret ptr)
 *   if (p != 0) { func_0224910e(entity, p); return 1; }
 *   func_022469f2(entity); return 0;
 *
 * Both calls are resolved bsrf. func_0224910e is actor_spawn_sub_init (already
 * decompiled). fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void *enemy_find_player_in_strike_range(void *entity);                /* probe (1-arg) */
extern void  enemy_enter_parent_link(void *entity, void *src);     /* actor_spawn_sub_init */
extern void  enemy_reset_idle_or_walk(void *entity);

s32 enemy_try_grab(void *entity) {
    void *p = enemy_find_player_in_strike_range(entity);
    if (p != 0) {
        enemy_enter_parent_link(entity, p);
        return 1;
    }
    enemy_reset_idle_or_walk(entity);
    return 0;
}
