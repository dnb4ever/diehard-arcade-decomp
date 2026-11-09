/* collision_probe_between_actors [0x022452be]: Builds an intermediate collision probe from the actors' separation and extents. */
#include "dh.h"
/* 0x022452be  "can arg0 reach arg1" range/aim test used by the attack AI.
 * Each actor has a 0x48-byte combat-box in g_060B6D9C[unk34]. Compute the signed
 * half x-distance h = (arg1->unk8 - arg0->unk8) >> 1 (arithmetic) and the
 * clearance d = |h| - boxA->unk18 - boxB->unk18; reject (return 0) if d <= 0.
 * Then build a probe block on the stack and call dispatch_cmd (segment query):
 *   aim block (&blk+4)  = { d, boxA->unk1C, boxA->unk20, boxA->unk24, ... }
 *   pos block (&blk+0x14) = { h + arg0->unk8, 0, arg0->unk10 }
 * On a hit it returns the struck object (hit->unk8); otherwise 0.
 * fidelity: HIGH - shar for the half-distance; box stride 0x48; the two pointers
 *   passed to dispatch_cmd are &blk[0x14] (pos) and &blk[4] (aim). */
extern u8 g_actor_movement_records[];   /* combat-box array, 0x48-byte elements */
void *dispatch_cmd(void *pos, void *aim, s32 a3, s8 kind); /* 0x02217838 */

void *collision_probe_between_actors(void *arg0, void *arg1) {
    u8 *boxA = &g_actor_movement_records[(s16)(M8(arg0, 0x34) * 0x48)];
    u8 *boxB = &g_actor_movement_records[(s16)(M8(arg1, 0x34) * 0x48)];
    s32 h = (M32(arg1, 8) - M32(arg0, 8)) >> 1;
    s32 ah = (h < 0) ? -h : h;
    s32 d = (ah - M32(boxA, 0x18)) - M32(boxB, 0x18);
    if (d <= 0)
        return 0;
    {
        u8 blk[0x20];
        M32(blk, 0x04) = d;
        M32(blk, 0x08) = M32(boxA, 0x1C);
        M32(blk, 0x0C) = M32(boxA, 0x20);
        M32(blk, 0x10) = M32(boxA, 0x24);
        M32(blk, 0x14) = h + M32(arg0, 8);
        M32(blk, 0x18) = 0;
        M32(blk, 0x1C) = M32(arg0, 0x10);
        {
            void *hit = dispatch_cmd(blk + 0x14, blk + 4, 0, M8(arg0, 0x34));
            if (hit != 0)
                return MP(hit, 8);
            return 0;
        }
    }
}
