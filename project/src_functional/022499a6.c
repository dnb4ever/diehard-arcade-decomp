#include "dh_work.h"
/* enemy_enter_optional_pose [0x022499a6]: Picks a descriptor pose for state fourteen or resumes idle when the entry is absent. */
#include "dh.h"
/* 0x022499a6  actor_enter_state14: enter actor state 0xE. Picks one of two values
 * from the actor's parameter table (entity[0x188]) with a 1-bit selector; if that
 * value reads back as 0xffff ("no entry") it bails to func_022469f2, otherwise it
 * installs state 0xE and seeds the usual state-enter fields. ABI (r4=entity).
 * Called by the handlers at 0x02248104 / 0x0224852c / 0x02248436.
 *   idx   = func_02210020() & 1;                 (selector; func takes no args)
 *   val   = ((s16 *)entity[0x188])[idx];
 *   if ((u16)val == 0xffff) { func_022469f2(entity); return; }   (braf tail)
 *   entity[0xd4]=14; entity[0xd8]=0; entity[0x84]=1;
 *   entity[0xe0] &= 0x20300; entity[0x108]=0; entity[0x41]=1;
 *   entity[0x5c]=val;
 * Both computed branches resolved with braf_resolve. func_02210020 overwrites r4 in
 * its own prologue, so it takes no argument. fidelity: HIGH - hand-traced vs
 * m2c_fn --asm; offsets from the clean 0x249xxx pool. */
extern s32  game_random_next(void);
extern void enemy_reset_idle_or_walk(void *entity);

void enemy_enter_optional_pose(void *entity) {
    s16 val = ((s16 *)ACTOR_MP(entity, variant_animation_table))[game_random_next() & 1];

    if ((u16)val == DH_ANIMATION_ABSENT) {
        enemy_reset_idle_or_walk(entity);
        return;
    }
    ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_OPTIONAL_POSE;
    ACTOR_M32(entity, behavior_phase) = 0;
    ACTOR_M32(entity, action_category) = 1;
    ACTOR_M32(entity, state_flags) &= 0x20300;
    ACTOR_M32(entity, action_phase) = 0;
    ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(entity, animation_requested) = val;
}
