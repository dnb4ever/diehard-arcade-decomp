#include "dh.h"
/* 0x0220d9ec (view_bounds_from_actors) - leaf. Builds an axis-aligned XZ bounding
 * region over the active actors and a ring of recent-position samples, then writes the
 * min corner to *minOut and the max corner to *maxOut (each a vector with X@0, Z@8).
 *
 * ABI (self, minOut, maxOut) = (r4, r5, r6). Pure leaf (no calls).
 *
 * Data:
 *   g_06099584: object table. slotFlags[8] longs @ +0xC, objPtr[8] pointers @ +0x2C.
 *               objPtr[i]->unk8 points at the actor coord struct {X@8, Z@0x10, flags@0x84}.
 *   g_06098838: recent-position ring, 8 entries of stride 8 {s32 idx@0, s32 ttl@4}.
 *   g_06052194: position-record table, stride 12 {s32 X@0, s32 Z@8}, indexed by
 *               (s8)(idx*0xC) exactly as the asm computes it (shll/add then exts.b).
 *
 * Seed of the box depends on how many of slots 1,2 are occupied (slotFlags[1]+[2]):
 *   0 -> self->unk84/unk8C ; 1 -> objPtr[1]->unk8 ; 2 -> objPtr[2]->unk8 ;
 *   3 -> min/max of objPtr[1]->unk8 and objPtr[2]->unk8.
 * A count outside 0..3 is unreachable (sum of two 0/1 flags handled by case 3); the box
 * is pre-zeroed to keep that dead path defined (asm leaves the pushed regs there).
 *
 * fidelity: HIGH - hand-traced against m2c_fn --asm (0x20d9ec..0x20dfe6). The two scan
 * loops (slots 3..7, then the 8-entry ring 4x-unrolled by the compiler) were re-rolled;
 * each min/max test is the asm's cmp/ge + cmp/gt pair = "if (v<lo) lo=v; else if (v>hi) hi=v". */

extern u8 g_actor_slot_registry[];   /* object table */
extern u8 g_06098838[];   /* recent-position ring (8 x {idx, ttl}) */
extern u8 g_06052194[];   /* position-record table (stride 12) */

void view_bounds_from_actors(void *self, void *minOut, void *maxOut)
{
    s32 minX = 0, maxX = 0, minZ = 0, maxZ = 0;
    void *a, *b;
    void *obj;
    void *coord;
    s32 i, v, x, z;

    switch (M32(g_actor_slot_registry, 0x10) + M32(g_actor_slot_registry, 0x14)) {   /* slotFlags[1] + slotFlags[2] */
    case 3:
        a = MP(MP(g_actor_slot_registry, 0x30), 8);   /* objPtr[1]->unk8 */
        b = MP(MP(g_actor_slot_registry, 0x34), 8);   /* objPtr[2]->unk8 */
        if (M32(a, 8) > M32(b, 8)) { maxX = M32(a, 8); minX = M32(b, 8); }
        else                       { maxX = M32(b, 8); minX = M32(a, 8); }
        if (M32(a, 0x10) > M32(b, 0x10)) { maxZ = M32(a, 0x10); minZ = M32(b, 0x10); }
        else                             { maxZ = M32(b, 0x10); minZ = M32(a, 0x10); }
        break;
    case 1:
        coord = MP(MP(g_actor_slot_registry, 0x30), 8);   /* objPtr[1]->unk8 */
        minX = maxX = M32(coord, 8);
        minZ = maxZ = M32(coord, 0x10);
        break;
    case 2:
        coord = MP(MP(g_actor_slot_registry, 0x34), 8);   /* objPtr[2]->unk8 */
        minX = maxX = M32(coord, 8);
        minZ = maxZ = M32(coord, 0x10);
        break;
    case 0:
        minX = maxX = M32(self, 0x84);
        minZ = maxZ = M32(self, 0x8C);
        break;
    }

    /* Slots 3..7: expand over any active actor whose coord flag top-bit is clear. */
    for (i = 3; i < 8; i++) {
        if (M32(g_actor_slot_registry, 0xC + i * 4) == 0)
            continue;
        coord = MP(MP(g_actor_slot_registry, 0x2C + i * 4), 8);
        if (M32(coord, 0x84) & 0x80000000)
            continue;
        x = M32(coord, 8);
        if (x < minX) minX = x; else if (x > maxX) maxX = x;
        z = M32(coord, 0x10);
        if (z < minZ) minZ = z; else if (z > maxZ) maxZ = z;
    }

    /* Recent-position ring: each live entry (ttl>0) contributes one sampled point, ages
     * one tick, and is retired (ttl=0) unless its slot's actor is still flagged. */
    for (i = 0; i < 8; i++) {
        if (M32(g_06098838, i * 8 + 4) <= 0)   /* ttl */
            continue;
        M32(g_06098838, i * 8 + 4) -= 1;
        v = M32(g_06098838, i * 8);            /* idx */
        x = M32(g_06052194, (s8)(v * 0xC));
        if (x < minX) minX = x; else if (x > maxX) maxX = x;
        z = M32(g_06052194, (s8)(v * 0xC) + 8);
        if (z < minZ) minZ = z; else if (z > maxZ) maxZ = z;
        if (M32(g_actor_slot_registry, 0xC + i * 4) != 0) {
            obj = MP(MP(g_actor_slot_registry, 0x2C + i * 4), 8);
            if (!(M32(obj, 0x84) & 0x80000000))
                M32(g_06098838, i * 8 + 4) = 0;
        } else {
            M32(g_06098838, i * 8 + 4) = 0;
        }
    }

    M32(minOut, 0) = minX;
    M32(minOut, 8) = minZ;
    M32(maxOut, 0) = maxX;
    M32(maxOut, 8) = maxZ;
}
