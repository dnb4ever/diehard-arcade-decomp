/* enemy_select_player_side [0x02245a6a]: Selects the ready player's distance/side record, choosing the nearer side when both are ready. */
#include "dh.h"
/* 0x02245a6a  resolve the actor's active facing/side flag according to the global
 * combat mode g_06098564, reading the actor's slot in g_060995D0 (0x20 stride):
 *   slot = &g_060995D0[actor->unk34 * 0x20]
 *   mode 0 -> 0
 *   mode 1 -> slot->unk4 & ~1
 *   mode 2 -> (slot->unk8 & ~1) | 1
 *   mode 3 -> (u)slot->unk4 >= (u)slot->unk8 ? (slot->unk8 & ~1)|1 : slot->unk4 & ~1
 *   else   -> actor->unk34
 * fidelity: HIGH - mode cascade matches the cmp/eq chain; unsigned cmp/hs in mode 3. */
extern u32 g_ready_player_mask;       /* combat mode */
extern u8 g_060995D0[];      /* per-actor combat slots, 0x20 stride */

s32 enemy_select_player_side(void *actor) {
    u8 *slot = &g_060995D0[(s32)M8(actor, 0x34) << 5];
    switch (g_ready_player_mask) {
    case 0:
        return 0;
    case 1:
        return M32(slot, 4) & ~1;
    case 2:
        return (M32(slot, 8) & ~1) | 1;
    case 3:
        if ((u32)M32(slot, 4) >= (u32)M32(slot, 8))
            return (M32(slot, 8) & ~1) | 1;
        return M32(slot, 4) & ~1;
    default:
        return (s8)M8(actor, 0x34);
    }
}
