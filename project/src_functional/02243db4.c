/* enemy_commit_dodge_lane [0x02243db4]: Flips blocked lane, records target/offset then spawns move actor. */
#include "dh_work.h"
#include "dh.h"
/* 0x02243db4  commit the actor to a dodge/step in direction arg2 and spawn its
 * move actor. dir = arg2; if lane g_06096EC0[dir] is flagged 0x71 (blocked) flip
 * to the opposite lane (dir ^= 2). Record:
 *   unk124 = dir;  unk128 = (dir & 1) + 1;
 *   unk130 = (dir & 2) ? -0x40000 : +0x40000;  unk134 = 0;
 * Spawn via func_02244820(actor, arg1, dir, 1) and park the returned handle in
 * g_06097128[actor->unk34] (0x24-byte stride). Finally unkE0: clear 0x50, set 0x20.
 * fidelity: HIGH - widths from asm (unk124/128/130/134/E0 32-bit; lane table byte);
 *   g_06097128 holds object handles (pointer-sized slots). */
extern u8 g_06096EC0[];      /* per-lane state bytes */
extern u8 g_06097128[];      /* per-actor move-handle table, 0x24 stride */
void *enemy_test_dodge_side(void *actor, void *arg1, s32 dir, s32 flag);  /* 0x02244820 */

void enemy_commit_dodge_lane(void *actor, void *arg1, s32 arg2) {
    s32 dir = arg2;
    if ((s8)g_06096EC0[dir] == 0x71)
        dir ^= 2;
    M32(actor, 0x124) = dir;
    ACTOR_M32(actor, player_target_slot) = (dir & 1) + 1;
    ACTOR_M32(actor, target_offset_x) = (dir & 2) ? (s32)0xFFFC0000 : 0x40000;
    ACTOR_M32(actor, target_offset_z) = 0;
    MP(&g_06097128[(s16)(ACTOR_M8(actor, slot_id) * 0x24)], 0) =
        enemy_test_dodge_side(actor, arg1, dir, 1);
    ACTOR_M32(actor, state_flags) &= ~0x50;
    ACTOR_M32(actor, state_flags) |= 0x20;
}
