/* enemy_find_player_in_strike_range [0x022453de]: Finds a live player satisfying action, cooldown, facing and descriptor range gates. */
#include "dh_work.h"
#include "dh.h"
/* 0x022453de  scan combat slots 1 and 2 for an in-range strikeable target and
 * return it (the target object), or 0.  Leaf (no calls).
 * Early-out 0 unless actor->unk8C & 0x3FFFF.
 * For slot in {1,2} with g_06099590[slot] != 0, target = g_060995B0[slot]->unk8:
 *   require target->unk84 & 0x40D, NOT target->unk84 & 0x237FBB00,
 *   target->unk48 == 0 and target->unk4E == 0;
 *   idx = max(actor->unkE4 - 7, 0); ent = g_0608EF6C + (s8)(idx*12) (3 s32 fields
 *   x-min/x-max/z-range); require |dz| < ent.z and the signed x-window:
 *     dx = target->unk8 - actor->unk8; dx>0 needs actor->unk44==0, dx<=0 needs
 *     actor->unk44!=0 (|dx| used); ent.xmin <= |dx| < ent.xmax -> return target.
 * fidelity: HIGH - the "bsrf" braf_resolve flagged here was capstone misreading
 *   the 0x0003FFFF literal-pool word; there is no dispatch. The (s8) table-offset
 *   truncation is reproduced verbatim (dead for the small idx the game uses). */
extern s32 g_actor_slot_ids[];     /* slot-occupied flags (cluster 0x06099584+12) */
extern void *g_actor_slot_tasks[];   /* slot -> target wrapper (cluster +44) */
extern u8 g_0608EF6C[];      /* per-state reach window table, 12-byte entries */

void *enemy_find_player_in_strike_range(void *actor) {
    s32 slot;
    if (!(M32(actor, 0x8C) & 0x3FFFF))
        return 0;
    for (slot = 1; slot < 3; slot++) {
        void *target;
        s32 t84, idx, eoff, dz, dx;
        if (g_actor_slot_ids[slot] == 0)
            continue;
        target = TASK_PTR(g_actor_slot_tasks[slot], work);
        t84 = M32(target, 0x84);
        if (!(t84 & 0x40D) || (t84 & 0x237FBB00))
            continue;
        if (M8(target, 0x48) != 0 || M8(target, 0x4E) != 0)
            continue;
        idx = M32(actor, 0xE4) - 7;
        if (idx < 0)
            idx = 0;
        eoff = (s8)(idx * 12);
        dz = M32(actor, 0x10) - M32(target, 0x10);
        if (dz < 0)
            dz = -dz;
        if (dz >= M32(g_0608EF6C, eoff + 8))
            continue;
        dx = M32(target, 8) - M32(actor, 8);
        if (dx > 0) {
            if (M8(actor, 0x44) != 0)
                continue;
        } else {
            if (M8(actor, 0x44) == 0)
                continue;
            dx = -dx;
        }
        if (M32(g_0608EF6C, eoff) > dx || dx >= M32(g_0608EF6C, eoff + 4))
            continue;
        return target;
    }
    return 0;
}
