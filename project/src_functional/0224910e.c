#include "dh_work.h"
/* enemy_enter_parent_link [0x0224910e]: Selects a random link descriptor and binds the actor to its source slot in state nine. */
#include "dh.h"
/* 0x0224910e  actor sub-entity spawn/init: initialise a freshly handed-off actor
 * (entity, r4) from a parameter record selected out of entity[0x178] and link it
 * back to its parent (src, r5). Called as the tail hand-off from the handler at
 * 0x0224852c when its probe returns non-null. ABI (r4=entity, r5=src).
 *
 *   idx    = func_02210020() & 3;                  (selector; func takes no args)
 *   record = ((void **)entity[0x178])[idx];       (per-variant parameter block)
 *
 *   entity[0xd4]=9; entity[0xd8]=0; entity[0x84]=0x20000; entity[0xe0]=0x6000;
 *   entity[0x108]=2;
 *   mask = func_0224a360(1, src[0x34]);            (value r0, count r1)
 *   entity[0xbc] |= mask;
 *   entity[0x4b] = (s8)src[0x34];
 *   entity[0x42] = (s8)record[4];                  (low byte of a 16-bit field)
 *   entity[0x41] = 1;
 *   entity[0x5c] = record[(entity[0x44]==src[0x44]) ? 2 : 0];   (16-bit)
 *   src[0x4c]    = (s8)entity[0x34];
 *   src[0x84]    = 0x40000;
 *   src[0x88]    = (s32)record[6];
 *   entity[0xc4] = (entity[0x44] != 0) ? -record[8]  : record[8];    (facing sign)
 *   entity[0xc8] = (entity[0x44] != 0) ? -record[12] : record[12];
 *   entity[0x104]= record[16];
 *
 * record layout: s16 @ {0,2,4,6}, s32 @ {8,12,16}. Both computed calls resolved
 * with braf_resolve. func_02210020 overwrites r4 in its own prologue, so it takes
 * no argument here. func_0224a360 (in the already-done dispatch band) reads its
 * count from r1 and value 1 from r0, then ORs the returned mask into entity+0xBC.
 * Round-5 evidence: work/decomp5/runtime_tail.asm, 0604d150..d15c. fidelity: HIGH - hand-traced
 * vs m2c_fn --asm; all PC-relative field offsets resolved from the (clean,
 * 4-aligned) 0x249xxx pool. */
extern s32  game_random_next(void);
extern s32 shc_shift_left(s32 value, s32 count); /* ROM r0/r1 ABI */

void enemy_enter_parent_link(void *entity, void *src) {
    void *record = ((void **)MP(entity, 0x178))[game_random_next() & 3];

    ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_PARENT_LINK;
    ACTOR_M32(entity, behavior_phase) = 0;
    ACTOR_M32(entity, action_category) = 0x20000;
    ACTOR_M32(entity, state_flags) = 0x6000;
    ACTOR_M32(entity, action_phase) = 2;

    /* ROM 0604d154 delay slot seeds r0=1; OR the shifted result. */
    M32(entity, 0xBC) |= shc_shift_left(1, M8(src, 0x34));

    ACTOR_M8(entity, paired_slot) = M8(src, 0x34);
    M8(entity, 0x42) = (s8)M16(record, 4);
    ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(entity, animation_requested) = M16(record, (ACTOR_M8(entity, facing_right) == M8(src, 0x44)) ? 2 : 0);

    M8(src, 0x4C) = ACTOR_M8(entity, slot_id);
    M32(src, 0x84) = 0x40000;
    M32(src, 0x88) = M16(record, 6);

    M32(entity, 0xC4) = (ACTOR_M8(entity, facing_right) != 0) ? -M32(record, 8)  : M32(record, 8);
    M32(entity, 0xC8) = (ACTOR_M8(entity, facing_right) != 0) ? -M32(record, 12) : M32(record, 12);
    ACTOR_M32(entity, queued_attack_slot) = M32(record, 16);
}
