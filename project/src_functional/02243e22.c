/* enemy_setup_lateral_approach [0x02243e22]: Mode-selected peer, arena bounds and lane occupancy determine target X offset. */
/* Round 4: 06047f92..47fb0 preserves separate AND and OR stores to actor+E0; final value equality had hidden the missing intermediate store. Evidence: work/decomp4/fix3.asm. */
#include "dh_work.h"
#include "dh.h"
/* 0x02243e22  set up a lateral approach toward the mode-selected peer, choosing a
 * target x-offset (unk130) that respects arena bounds and lane blocking. void.
 * range = (actor->unkE4 == 9) ? 0x50000 : 0x38000.
 * peer by combat mode g_06098564: 0 -> actor itself; 1 -> g_060995B0[1]->unk8;
 *   2 -> g_060995B0[2]->unk8; 3 -> the z-nearer of g_060995B0[1]/[2]->unk8.
 * box18 = g_060B6D9C[actor->unk34 (0x48 stride)].unk18 (combat-box inset).
 * If the peer is to the left (actor->unk8 - peer->unk8 > 0):
 *   blocked lane g_06096EC0[peer->unk34-1]==0x71 -> ofs = -range; else
 *   d = g_060972D4 - peerx; ofs = (d > range) ? range : d - box18.
 * If the peer is to the right:
 *   !(peer->unk84 & 0x20000000) && g_06096EC0[(peer->unk34-1)|2]==0x71 -> ofs=range;
 *   else d = g_060972D0 - peerx; ofs = (d >= -range) ? d + box18 : -range.
 * Timing gate: func_02245fc4 busy -> unkE0 = (unkE0 & ~0x50)|0x20; clear ->
 *   unkE0 = (unkE0 & ~0x60)|0x90. Then unk128 = peer->unk34, unk130 = ofs,
 *   unk134 = 0.
 * fidelity: HIGH - the "bsrf r0" braf_resolve saw at 0x243e64 was the 0x00038000
 *   literal-pool word; the only real bsrf resolves to the 0x245fc4 timing gate.
 *   g_06098564 is 0..3 so the asm's unset-reg default is dead (rendered return). */
extern s32 g_ready_player_mask;       /* active combat mode */
extern void *g_actor_slot_tasks[];   /* slot -> peer wrapper (cluster 0x06099584+44) */
extern s8 g_06096EC0[];      /* per-lane reservation/block table */
extern u8 g_actor_movement_records[];      /* combat-box table, 0x48 stride */
extern s32 g_060972D0;       /* arena x min bound */
extern s32 g_060972D4;       /* arena x max bound */
s32 enemy_classify_attack_timing(void *p, void *q);      /* 0x02245fc4 timing gate */

void enemy_setup_lateral_approach(void *actor) {
    s32 range = (ACTOR_M32(actor, state_index) == 9) ? 0x50000 : 0x38000;
    void *peer;
    s32 peerx, box18, ofs;
    s8 p34m1;

    switch (g_ready_player_mask) {
    case 0:
        peer = actor;
        break;
    case 1:
        peer = TASK_PTR(g_actor_slot_tasks[1], work);
        break;
    case 2:
        peer = TASK_PTR(g_actor_slot_tasks[2], work);
        break;
    case 3: {
        void *a = TASK_PTR(g_actor_slot_tasks[1], work);
        void *b = TASK_PTR(g_actor_slot_tasks[2], work);
        s32 az = ACTOR_M32(actor, position_z);
        s32 da = az - M32(a, 0x10);
        s32 db = az - M32(b, 0x10);
        if (da < 0)
            da = -da;
        if (db < 0)
            db = -db;
        peer = (da >= db) ? b : a;
        break;
    }
    default:
        return; /* g_06098564 is 0..3; asm default path is dead */
    }

    peerx = M32(peer, 8);
    p34m1 = M8(peer, 0x34) - 1;
    box18 = M32(g_actor_movement_records, ACTOR_M8(actor, slot_id) * 0x48 + 0x18);

    if (ACTOR_M32(actor, position_x) - peerx > 0) {
        if (g_06096EC0[p34m1] == 0x71) {
            ofs = -range;
        } else {
            s32 d = g_060972D4 - peerx;
            ofs = (d > range) ? range : (d - box18);
        }
    } else {
        if (!(M32(peer, 0x84) & 0x20000000) && g_06096EC0[p34m1 | 2] == 0x71) {
            ofs = range;
        } else {
            s32 d = g_060972D0 - peerx;
            ofs = (d >= -range) ? (d + box18) : -range;
        }
    }

    if (enemy_classify_attack_timing(actor, peer) != 0) {
        ACTOR_M32(actor, state_flags) &= ~0x50;
        ACTOR_M32(actor, state_flags) |= 0x20;
    } else {
        ACTOR_M32(actor, state_flags) &= ~0x60;
        ACTOR_M32(actor, state_flags) |= 0x90;
    }
    ACTOR_M32(actor, player_target_slot) = M8(peer, 0x34);
    ACTOR_M32(actor, target_offset_x) = ofs;
    ACTOR_M32(actor, target_offset_z) = 0;
}
