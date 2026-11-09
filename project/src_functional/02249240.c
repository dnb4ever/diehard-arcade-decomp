/* enemy_enter_timed_reaction [0x02249240]: Enters reaction state seventeen/substate four, selects its timer and resolves movement. */
#include "dh.h"
#include "dh_work.h"
/* 0x02249240  actor_enter_state4: move the actor into state 4 (a timed pose),
 * compute its countdown from one of two sources, and tail into func_022170ea.
 * ABI: single arg r4 = entity.
 *
 *   entity[0xd4]=0x11; entity[0xd8]=4; entity[0x84]=0x1000; entity[0x108]=3;
 *   func_02245e58(entity);        (recomputes entity[0x4d]; also leaves it in r5)
 *   entity[0xbc]=0x7fffffff;
 *   if (entity[0xe0] & 0x400000)  c = entity[0x4d] - 4;   (r5 leftover == 0x4d)
 *   else {
 *       i = *(s32*)0x0609958c - 1; if (i < 0) i = 0;
 *       c = ((s32*)0x06090138)[ (0xc & (entity[0xe8] << 2)) + (i & 3) ];
 *   }
 *   entity[0xc4] = c;
 *   entity[0xe0] = (entity[0xe0] & 0x01400000) | 0x200400;
 *   func_022170ea(entity, 0);     (braf tail, resolved)
 *
 * func_02245e58 stores its computed nibble to entity[0x4d] and leaves the same
 * value in r5 (its epilogue is `rts; mov.b r5,@(0x4d,entity)`); the 0x400000
 * branch's `add #-4,r5` therefore reads that stored value back as M8(,0x4d).
 * modelling it via the field is faithful (same value) and portable. func_022170ea
 * is passed r5=0 by the compiler, so it is a 2-arg tail call. fidelity: HIGH -
 * hand-traced vs m2c_fn --asm. */
extern void actor_decode_enemy_direction(void *entity);     /* recomputes entity[0x4d], leaves it in r5 */
extern void actor_resolve_movement(void *entity, s32 arg);

void enemy_enter_timed_reaction(void *entity) {
    s32 c;

    ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
    ACTOR_M32(entity, behavior_phase) = 4;
    ACTOR_WORK(entity)->action_category = 0x1000;
    ACTOR_M32(entity, action_phase) = 3;
    actor_decode_enemy_direction(entity);
    M32(entity, 0xBC) = 0x7FFFFFFF;

    if (ACTOR_WORK(entity)->state_flags & 0x400000) {
        c = ACTOR_M8(entity, direction_code) - 4;
    } else {
        s32 i = *(s32 *)(uintptr_t)0x0609958C - 1;
        if (i < 0) {
            i = 0;
        }
        c = ((s32 *)(uintptr_t)0x06090138)[(0xC & (MU32(entity, 0xE8) << 2)) + (i & 3)];
    }
    M32(entity, 0xC4) = c;
    ACTOR_WORK(entity)->state_flags &= 0x01400000;
    ACTOR_WORK(entity)->state_flags |= 0x200400;
    actor_resolve_movement(entity, 0);
}
