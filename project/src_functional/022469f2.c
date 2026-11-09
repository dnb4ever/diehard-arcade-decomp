#include "dh_work.h"
/* enemy_reset_idle_or_walk [0x022469f2]: Shared enemy continuation enters walk under gates or resets animation, category/state and random timer. */
#include "dh.h"
/* 0x022469f2 - enemy_reset_idle_or_walk: "kick to walk / reset counters" state handler, the
 * shared tail invoked by most of the 0x248xxx/0x249xxx dispatchers.
 * real fn [0x2469f2, 0x246ab8) (fn.py "code 0xc6"; the 0x152 extent trails into
 * func_02246ab8's own prologue - bounded by the rts epilogue at 0x246ab4).
 *
 * Fast path (all three gates pass): flag the actor and TAIL-call
 *   actor_enter_statewalk (actor_enter_statewalk):
 *     if (M32(obj,0xE4) == 6 && (M32(obj,0x8C) & 0x3FFFF) == 0 && M8(obj,0x6A) < 3) {
 *         M32(obj,0xE0) |= 0x01000000;
 *         actor_enter_statewalk(obj);   // braf tail at 0x246a24 (delta resolves
 *         return;                       //   to 0x241e3c, pr/r14 restored first)
 *     }
 * Otherwise reset the actor to the idle/walk base state:
 *     if (M16(obj,0x5A) != M16(obj,0x148)) { M8(obj,0x41)=1; M16(obj,0x5C)=M16(obj,0x148); }
 *     M32(obj,0xD4)=0; M32(obj,0x84)=1; M32(obj,0x88)=0;
 *     M32(obj,0xE0)=(M32(obj,0xE0)&0x00020320)|1;
 *     M32(obj,0x108)=0; M32(obj,0xC4)=2;
 *     M32(obj,0xC8)=game_random_next()&7;   // randomized frame counter
 *
 * fidelity: HIGH - exact. 0xE4 read as mov.l (==6); 0x8C masked &0x3FFFF (reg-form
 * tst, no imm sign-extend trap); 0x6A is a signed-byte cmp/ge #3; 0x5A/0x148/0x5C
 * are 16-bit (equality compare, sign irrelevant); 0x84 stores the constant 1 (the
 * r5 that the bt/s delay slot sets unconditionally); 0xC8 gets rng()&7. */
extern s32  game_random_next(void);                 /* 0x02210020 rng_next */
extern void actor_enter_statewalk(void *actor);  /* 0x02241e3c (actor_enter_statewalk) */

void enemy_reset_idle_or_walk(void *obj)
{
    if (ACTOR_WORK(obj)->state_index == 6 &&
        (ACTOR_M32(obj, interaction_flags) & 0x3FFFF) == 0 &&
        ACTOR_M8(obj, saved_primary_effect_type) < 3) {
        ACTOR_WORK(obj)->state_flags |= 0x01000000;
        actor_enter_statewalk(obj);
        return;
    }
    if (ACTOR_WORK(obj)->animation_current != ACTOR_M16(obj, idle_animation)) {
        ACTOR_WORK(obj)->animation_control = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_WORK(obj)->animation_requested = ACTOR_M16(obj, idle_animation);
    }
    ACTOR_M32(obj, behavior_state) = 0;
    ACTOR_WORK(obj)->action_category = 1;
    ACTOR_WORK(obj)->action_state = 0;
    /* ROM 02246a90..02246a9c: two separate read-modify-write stores (and, then or). */
    ACTOR_WORK(obj)->state_flags &= 0x00020320;
    ACTOR_WORK(obj)->state_flags |= 1;
    ACTOR_M32(obj, action_phase) = 0;
    M32(obj, 0xC4) = 2;
    M32(obj, 0xC8) = game_random_next() & 7;
}
