#include "dh_work.h"
/* enemy_enter_random_idle18 [0x02249ea0]: Enters state eighteen and chooses its ordinary or rare descriptor idle pose. */
#include "dh.h"
/* 0x02249ea0 - enter actor state 0x12. t = +0x180; set +0xD4=0x12, +0x84=1, +0x88=0,
 * +0xE0 &= 0x300, +0x108=6, +0x41=1. If +0x38 has any of bits 0x7800 and rng_next()&0xF==0,
 * set +0xE0 |= 0x02000000 and +0x5C = t->+0xA; otherwise +0x5C = t->+0x4.
 * fidelity: HIGH - +0xD4/+0x84/+0x88/+0xE0/+0x108/+0x38 mov.l; +0x41 mov.b; +0x5C mov.w. */
extern s32 game_random_next(void);   /* rng_next */

void enemy_enter_random_idle18(void *a) {
    void *t = ACTOR_MP(a, reaction_descriptor);

    ACTOR_M32(a, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
    ACTOR_M32(a, action_category) = 1;
    ACTOR_M32(a, action_state) = 0;
    ACTOR_M32(a, state_flags) &= 0x300;
    ACTOR_M32(a, action_phase) = 6;
    ACTOR_M8(a, animation_control) = DH_ANIMATION_CONTROL_RESTART;

    if ((ACTOR_M32(a, render_flags) & 0x7800) && (game_random_next() & 0xF) == 0) {
        ACTOR_M32(a, state_flags) |= 0x02000000;
        ACTOR_M16(a, animation_requested) = M16(t, 0xA);
        return;
    }
    ACTOR_M16(a, animation_requested) = M16(t, 4);
}
