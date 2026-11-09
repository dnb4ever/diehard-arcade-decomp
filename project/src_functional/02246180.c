/* enemy_choose_random_direction [0x02246180]: RNG optionally selects four-entry direction animation table else fallback heading animation. */
#include "dh_work.h"
#include "dh.h"
/* 0x02246180  pick a new facing/direction for the actor from an rng roll.
 * r = rng(); set retrigger unk41=1 and clear the "has-dir" bit (unkE0 & ~8).
 * If (r & 3) == 0: cand = actor->unk154[(r>>2) & 3] (s16 table); when cand is not
 *   the 0xFFFF sentinel: unk5C = cand, set bit 3 (0x8) of unkE0, done.
 * Otherwise fall back to unk5C = (s16)actor->unk148 (16-bit load at +0x148).
 * fidelity: HIGH - rng = func_02210020; 16-bit table reads via guest address,
 * unsigned 0xFFFF test; fallback is mov.w @(0x148) per 0x2461a0-0x2461a8. */
extern u32 game_random_next(void);  /* rng_next */

void enemy_choose_random_direction(void *actor) {
    s32 r = game_random_next();
    ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M32(actor, state_flags) &= ~8;
    if (!(r & 3)) {
        u32 table = ACTOR_MU32(actor, direction_animation_table);
        s16 cand = M16(table, ((r >> 2) & 3) * 2);
        if ((u16)cand != 0xFFFF) {
            ACTOR_M16(actor, animation_requested) = cand;
            ACTOR_M32(actor, state_flags) |= 8;
            return;
        }
    }
    ACTOR_M16(actor, animation_requested) = ACTOR_M16(actor, idle_animation);
}
