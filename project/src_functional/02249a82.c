#include "dh_work.h"
/* enemy_enter_clash [0x02249a82]: Chooses the clash anchor/quadrant and enters state fifteen or resumes idle. */
#include "dh.h"
/* 0x02249a82  actor_enter_clash: set the actor into the clash/tie-up pose (state
 * 0xf), choosing a clash descriptor (one of the 12-byte records at 0x06099764)
 * from the global clash selector, and recording which quadrant the actor sits in
 * relative to that descriptor. ABI: single arg r4 = entity.
 *
 *   rec = (*(void**)0x0609882c)->unk8;              (active clash record)
 *   switch (*(s32*)0x0609977c & 3) {                (clash selector)
 *     0: func_022469f2(entity); return;             (abort / plain kick)
 *     1: desc = &tbl[0];                             (tbl = 0x06099764)
 *     2: desc = &tbl[0]+0xc;
 *     3: desc = &tbl[0] + 0xc*(1 & (rec[0xaf]-1));
 *   }
 *   entity[0x124] = desc;
 *   quad  = (entity[0x8]  < desc[0x0]) ? 1 : 0;      (x side)
 *   quad |= (entity[0x10] < desc[0x8]) ? 2 : 0;      (y side)
 *   flag = (quad==0 || quad==3) ? 1 : 0;             (diagonal agreement)
 *   *(s32*)(0x06097128 + (s16)(entity[0x34]*0x24)) = flag;
 *   func_022444b6(entity, quad, 1);
 *   entity[0xd4]=0xf; entity[0xd8]=0; entity[0x84]=0x201;
 *   entity[0xe0] &= 0x300; entity[0x108]=0;
 *
 * The & 3 selector covers all 4 switch arms, so the asm's bare `bra default`
 * (register uninitialised) is unreachable. func_022469f2 and func_022444b6 are
 * both 1-arg; func_022444b6 reads only r4 (its prologue reloads r4 from a field
 * and never touches r5/r6), so the quad/1 still live in r5/r6 at the call are
 * leftovers it ignores. fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern s32  enemy_sweep_to_anchor(void *entity);   /* 1-arg; return discarded here */
extern void enemy_reset_idle_or_walk(void *entity);

void enemy_enter_clash(void *entity) {
    void *rec = MP(*(void **)(uintptr_t)DH_VIEW_TASK_ADDR, 8);
    void *desc = (void *)(uintptr_t)0x06099764;
    s32 quad;
    s32 flag;

    switch (*(s32 *)(uintptr_t)0x0609977C & 3) {
    case 0:
        enemy_reset_idle_or_walk(entity);
        return;
    case 1:
        desc = (void *)(uintptr_t)0x06099764;
        break;
    case 2:
        desc = (void *)(uintptr_t)(0x06099764 + 0xC);
        break;
    case 3:
        desc = (void *)(uintptr_t)(0x06099764 + (s8)((1 & (M8(rec, 0xAF) - 1)) * 0xC));
        break;
    }

    MP(entity, 0x124) = desc;
    quad = (ACTOR_M32(entity, position_x) < M32(desc, 0x0)) ? 1 : 0;
    if (ACTOR_M32(entity, position_z) < M32(desc, 0x8)) {
        quad |= 2;
    }
    flag = (quad == 0 || quad == 3) ? 1 : 0;
    *(s32 *)((uintptr_t)0x06097128 + (s16)(ACTOR_M8(entity, slot_id) * 0x24)) = flag;
    enemy_sweep_to_anchor(entity);
    ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_CLASH;
    ACTOR_M32(entity, behavior_phase) = 0;
    ACTOR_M32(entity, action_category) = 0x201;
    ACTOR_M32(entity, state_flags) &= 0x300;
    ACTOR_M32(entity, action_phase) = 0;
}
