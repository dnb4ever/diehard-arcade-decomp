/* enemy_choose_action_and_side [0x02245632]: Mode-selected peer and combat state classify action 0..4 via output words. */
#include "dh_work.h"
#include "dh.h"
/* 0x02245632  choose the next combat ACTION (0..4) and SIDE flag for `actor`
 * against the mode-selected peer. Out-params: *out_action, *out_side (ORed).
 * Both start as the caller's values; an early return leaves action at 0.
 * Peer by combat mode g_ready_player_mask: 1 -> g_actor_slot_tasks[1]; 2 -> g_actor_slot_tasks[2];
 *   3 -> g_actor_slot_tasks[func_02245a6a(actor)&1]; 0/other -> return. In each case
 *   peer = wrapper->unk8; reject (return) if |actor->unk10 - peer->unk10| > 0x6666.
 * Common: reject if the timing gate func_02245fc4(actor,peer) is busy; tick rng.
 *   dx = peer->unk8 - actor->unk8; flags = actor->unk38; face = actor->unk44.
 * Grounded (flags & 0xFA00 and actor->unkBA in [11,24]): clear unkBA; set the
 *   turn bit (*out_side|=2, unkE0|=0x10000) when face matches dx direction;
 *   action = (rng&1) ? 4 : 3.
 * Near attack (rng > 0 and !(flags & 0x1E)): reject if (rng&7) > unkE8 (unsigned);
 *   require |dx| <= 0x23333 on the facing side; set turn bit on match; action = 1.
 * Far/grab (else): dx>0 needs peer->unk44!=0, dx<=0 needs peer->unk44==0;
 *   reject if |dx| > 0x80000, or |dx| > 0x23333 without peer->unk8C & 0x39E, or
 *   func_022452be finds nothing. On the facing side set *out_side|=2 and
 *   unkE0 |= (flags & 0x8200) ? 0x20000 : 0x10000. Then action = 2 when
 *   (flags & 0x7CC0) && (rng&2), else 3.
 * fidelity: HIGH - the single bsrf (0x210020 rng) resolved; func_02245fc4's dead
 *   3rd reg arg dropped (2-arg callee); unsigned (rng&7) vs unkE8; signed dx
 *   window tests; g_ready_player_mask is 0..3 in play, but the asm default (cluster base as peer)
 *   is kept (peer = cluster base 0x06099584, z window skipped). */
extern s32 g_ready_player_mask;       /* active combat mode */
extern void *g_actor_slot_tasks[];   /* slot -> peer wrapper (cluster 0x06099584+44) */
s32 enemy_select_player_side(void *actor);           /* 0x02245a6a side resolve */
s32 enemy_classify_attack_timing(void *p, void *q);      /* 0x02245fc4 timing gate */
void *collision_probe_between_actors(void *a, void *b);    /* 0x022452be range/aim select */
s32 game_random_next(void);                  /* rng tick 0x02210020 */

void enemy_choose_action_and_side(void *actor, s32 *out_action, s32 *out_side) {
    s32 az = ACTOR_M32(actor, position_z);
    s32 flags = ACTOR_M32(actor, render_flags);
    void *peer;
    s32 dz, dx, rng;
    s8 face;

    switch (g_ready_player_mask) {
    case 1:
        peer = TASK_PTR(g_actor_slot_tasks[1], work);
        break;
    case 2:
        peer = TASK_PTR(g_actor_slot_tasks[2], work);
        break;
    case 3:
        peer = TASK_PTR(g_actor_slot_tasks[enemy_select_player_side(actor) & 1], work);
        break;
    case 0:
        return;
    default:
        /* ROM 0224566a: any other mode skips the z window with the cluster
         * base 0x06099584 as peer (kept for fidelity; modes are 0..3 in play). */
        peer = (u8 *)g_actor_slot_tasks - 0x2C;
        goto gate;
    }
    dz = az - M32(peer, 0x10);
    if (dz < 0)
        dz = -dz;
    if (dz > 0x6666)
        return;

gate:
    if (enemy_classify_attack_timing(actor, peer) != 0)
        return;
    rng = game_random_next();
    dx = M32(peer, 8) - ACTOR_M32(actor, position_x);
    face = ACTOR_M8(actor, facing_right);

    if ((flags & 0xFA00) && ACTOR_M16(actor, type_flags) >= 11 && ACTOR_M16(actor, type_flags) <= 24) {
        ACTOR_M16(actor, type_flags) = 0;
        if ((dx > 0) ? (face != 0) : (face == 0)) {
            *out_side |= 2;
            ACTOR_M32(actor, state_flags) |= 0x10000;
        }
        *out_action = (rng & 1) ? 4 : 3;
        return;
    }

    if (rng > 0 && !(flags & 0x1E)) {
        if ((u32)(rng & 7) > (u32)M32(actor, 0xE8))
            return;
        if (dx > 0) {
            if (dx > 0x23333)
                return;
            if (face != 0) {
                *out_side |= 2;
                ACTOR_M32(actor, state_flags) |= 0x10000;
            }
        } else {
            if (dx < -0x23333)
                return;
            if (face == 0) {
                *out_side |= 2;
                ACTOR_M32(actor, state_flags) |= 0x10000;
            }
        }
        *out_action = 1;
        return;
    }

    /* far / grab approach */
    {
        s32 grab = flags & 0x8200;
        s8 pface = M8(peer, 0x44);
        if (dx > 0) {
            if (pface == 0)
                return;
            if (dx > 0x80000)
                return;
            if (dx > 0x23333 && !(M32(peer, 0x8C) & 0x39E))
                return;
            if (collision_probe_between_actors(actor, peer) == 0)
                return;
            if (face != 0) {
                *out_side |= 2;
                ACTOR_M32(actor, state_flags) |= grab ? 0x20000 : 0x10000;
            }
        } else {
            if (pface != 0)
                return;
            if (dx < -0x80000)
                return;
            if (dx < -0x23333 && !(M32(peer, 0x8C) & 0x39E))
                return;
            if (collision_probe_between_actors(actor, peer) == 0)
                return;
            if (face == 0) {
                *out_side |= 2;
                ACTOR_M32(actor, state_flags) |= grab ? 0x20000 : 0x10000;
            }
        }
    }
    *out_action = ((flags & 0x7CC0) && (rng & 2)) ? 2 : 3;
}
