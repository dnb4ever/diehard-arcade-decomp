#include "dh_work.h"
/* enemy_pick_optional_animation [0x02249b80]: Chooses an optional random descriptor pose, falling back when the entry is absent. */
#include "dh.h"
/* 0x02249b80  actor_pick_anim_variant: choose entity[0x5c] (current animation id)
 * from a random 2-bit selector. ABI: single arg r4 = entity.
 *   sel = func_02210020() & 3;              (func ignores args; returns a counter)
 *   fallback = entity[0x148];
 *   if (sel < 2)          anim = fallback;
 *   else { anim = ((s16*)entity[0x188])[sel];   (variant table pointer)
 *          if ((u16)anim == 0xffff) anim = fallback; }
 *   entity[0x5c] = anim;
 * func_02210020 overwrites r4 in its prologue, so it takes no argument here.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern s32 game_random_next(void);   /* ignores args; returns a rolling counter/rng */

void enemy_pick_optional_animation(void *entity) {
    s32 sel = game_random_next() & 3;
    s16 fallback = ACTOR_M16(entity, idle_animation);
    s16 anim;

    if (sel < 2) {
        anim = fallback;
    } else {
        anim = ((s16 *)MP(entity, 0x188))[sel];
        if ((u16)anim == DH_ANIMATION_ABSENT) {
            anim = fallback;
        }
    }
    MU16(entity, 0x5C) = anim;
}
