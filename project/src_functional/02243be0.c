/* enemy_commit_target_approach [0x02243be0]: Target-slot eligibility and lane distances reserve lateral approach. */
#include "dh_work.h"
#include "dh.h"
/* 0x02243be0  commit a side-step / approach toward the target in slot `slot`,
 * filling probe block `blk` with per-lane distances. i = slot - 1.
 * target = g_060995B0[slot]->unk8.
 * If target->unk84 & 0x20000000 (ungrabbable/occupied): clear the two adjacent
 *   lanes (g_06096EC0[slot+1]=g_06096EC0[slot-1]=-1), clear actor unkE0 bits
 *   0xC and unkC8, return 0.
 * Otherwise:
 *   - mark a near-wall lane 0x71 when target x +/- 0x16666 crosses the arena
 *     bounds g_060972D0 (min) / g_060972D4 (max);
 *   - dx = actor->unk8 - target->unk8; blk[8 + i*8]  = |dx - 0x16666|,
 *     blk[12 + i*8] = |dx + 0x16666|;
 *   - dz = actor->unk10 - target->unk10; blk[24 + i*4] = |dz|;
 *     fail (return 0) if |dz| >= actor->unk13C;
 *   - latch: unkE0 |= 0x200, unk120 = slot;
 *   - fail if both x-distances >= 0x10000 (target out of reach on both sides);
 *   - side = (target->unk8 < actor->unk8) ? 0 : 2; lane = side + i;
 *     fail if that lane is already reserved (g_06096EC0[lane] != 0);
 *   - commit: unk130 = side ? 0x16666 : -0x16666 (step velocity), unk134 = 0,
 *     reserve g_06096EC0[lane] = 1, unk124 = lane, unkE0 = (unkE0 & ~0x30)|0x100,
 *     return 1.
 * fidelity: HIGH - signed abs via cmp/pz; -0x16666 == 0xFFFE999A; the unkE0|=0x200
 *   and unk120 writes precede the reach gate exactly as in the asm. */
extern void *g_actor_slot_tasks[];   /* slot -> target wrapper (cluster 0x06099584+44) */
extern s8 g_06096EC0[];      /* per-lane reservation/block table */
extern s32 g_060972D0;       /* arena x min bound */
extern s32 g_060972D4;       /* arena x max bound */

s32 enemy_commit_target_approach(void *actor, s32 slot, void *blk) {
    s32 i = slot - 1;
    void *target = TASK_PTR(g_actor_slot_tasks[slot], work);

    if (M32(target, 0x84) & 0x20000000) {
        g_06096EC0[slot + 1] = -1;
        g_06096EC0[slot - 1] = -1;
        ACTOR_M32(actor, state_flags) &= ~0xC;
        M32(actor, 0xC8) = 0;
        return 0;
    }

    if ((M32(target, 8) - 0x16666) < g_060972D0)
        g_06096EC0[slot + 1] = 0x71;
    if ((M32(target, 8) + 0x16666) > g_060972D4)
        g_06096EC0[slot - 1] = 0x71;

    {
        s32 dx = ACTOR_M32(actor, position_x) - M32(target, 8);
        s32 lo = dx - 0x16666;
        s32 hi = dx + 0x16666;
        M32(blk, 8 + i * 8) = (lo < 0) ? -lo : lo;
        M32(blk, 12 + i * 8) = (hi < 0) ? -hi : hi;
    }
    {
        s32 dz = ACTOR_M32(actor, position_z) - M32(target, 0x10);
        s32 adz = (dz < 0) ? -dz : dz;
        M32(blk, 24 + i * 4) = adz;
        if (adz >= M32(actor, 0x13C))
            return 0;
    }

    ACTOR_M32(actor, state_flags) |= 0x200;
    ACTOR_M32(actor, target_slot) = slot;

    if (M32(blk, 8 + i * 8) >= 0x10000 && M32(blk, 12 + i * 8) >= 0x10000)
        return 0;

    {
        s32 side = (M32(target, 8) < ACTOR_M32(actor, position_x)) ? 0 : 2;
        s32 lane = side + i;
        if (g_06096EC0[lane] != 0)
            return 0;
        ACTOR_M32(actor, target_offset_x) = side ? 0x16666 : (s32)0xFFFE999A;
        ACTOR_M32(actor, target_offset_z) = 0;
        g_06096EC0[lane] = 1;
        M32(actor, 0x124) = lane;
        ACTOR_M32(actor, state_flags) = (ACTOR_M32(actor, state_flags) & ~0x30) | 0x100;
        return 1;
    }
}
