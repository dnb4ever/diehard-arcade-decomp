/* enemy_wait_or_turn [0x0223e96c]: Compares target facing and selects the waiting pose or enters turn state seven. */
#include "dh.h"
/* func_0223e96c(entity): pick a facing flag, then either latch a turn state or advance.
 * Compares ((g_060995B0[unk120])->unk8)->unk8 with entity->unk8: target further (>0)
 * -> flag 0, else flag 1. When flag == entity->unk44, latch: unkD4=0x12, unkD8=0,
 * unk84=1, unk88=0, unkE0=0x100000, unk41=1, unk5C=0x247. Otherwise run
 * func_0224840a(entity) and set unk5C = entity->unk14A.
 * fidelity: HIGH - all offsets verified from asm; func_0224840a resolved via braf;
 *   g_060995B0 is a 4-byte pointer table. */
extern void actor_set_state_7_entry(void *ent);
extern u8   g_actor_slot_tasks[];

void enemy_wait_or_turn(void *ent) {
    void *p = MP(MP(g_actor_slot_tasks, M32(ent, 0x120) * 4), 8);
    s32 flag;

    if ((M32(p, 8) - M32(ent, 8)) > 0) {
        flag = 0;
    } else {
        flag = 1;
    }
    if (flag == M8(ent, 0x44)) {
        M32(ent, 0xD4) = DH_ENEMY_BEHAVIOR_POSE;
        M32(ent, 0xD8) = 0;
        M32(ent, 0x84) = 1;
        M32(ent, 0x88) = 0;
        M32(ent, 0xE0) = 0x100000;
        M8(ent, 0x41) = 1;
        M16(ent, 0x5C) = DH_ANIM_TARGET_WAIT;
        return;
    }
    actor_set_state_7_entry(ent);
    M16(ent, 0x5C) = M16(ent, 0x14A);
}
