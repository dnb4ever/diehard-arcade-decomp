/* grid_find_overlap_any [0x02217888]: Searches neighbouring grid buckets for an overlapping node while excluding the active parent. */
#include "dh.h"
/* 0x02217888  grid_overlap_find_any: spatial-hash neighbour search.
 *
 * The world is bucketed into a 0x20 x 0x10 grid of linked lists at g_060B659C
 * (row stride 0x80 = 0x20 cells * 4-byte head pointers; list link = node->unk44).
 * Cell coords come from the subject position `pos`:
 *   cellX = ((pos->x - g_0609742C) >> 16) & 0x1F
 *   cellZ = ((pos->z - g_06097434) >> 16) & 0x0F
 * A 5x5 cell window (+-2, clamped to the grid) is scanned. For each node whose
 * type (node->unk4) is not the active-parent id g_060B88A0, the subject's swept
 * AABB [pos-ext .. pos+ext] (asymmetric Y: lower uses ext->unk4, upper ext->unkC)
 * is tested against the node box [node+0x28 .. node+0x34] via aabb_overlap_3; the
 * first overlapping node is returned, else NULL.
 *
 * fidelity: HIGH. bsrf resolved to aabb_overlap_3 (0x213354). This variant applies
 * no type filter beyond excluding the parent. */
extern u8  g_060B659C[];                               /* grid head-pointer array */
extern s32 aabb_overlap_3(s32 *a, s32 *b, s32 *c, s32 *d);

void *grid_find_overlap_any(void *pos, void *ext) {
    s32 cellX = (s32)(((u32)(M32(pos, 0) - M32(0x0609742C, 0)) >> 16) & 0x1F);
    s32 cellZ = (s32)(((u32)(M32(pos, 8) - M32(0x06097434, 0)) >> 16) & 0x0F);
    s32 box_min[3], box_max[3];
    s32 xStart, xEnd, zStart, zEnd, x, z;

    box_min[0] = M32(pos, 0) - M32(ext, 0);
    box_min[1] = M32(pos, 4) - M32(ext, 4);
    box_min[2] = M32(pos, 8) - M32(ext, 8);
    box_max[0] = M32(pos, 0) + M32(ext, 0);
    box_max[1] = M32(pos, 4) + M32(ext, 0xC);
    box_max[2] = M32(pos, 8) + M32(ext, 8);

    xStart = cellX - 2;
    if (xStart < 0) {
        xStart = 0;
        xEnd = cellX + 2;
    } else {
        xEnd = cellX + 2;
        if (xEnd > 0x1F) {
            xEnd = 0x1F;
        }
    }
    zStart = cellZ - 2;
    if (zStart < 0) {
        zStart = 0;
        zEnd = cellZ + 2;
    } else {
        zEnd = cellZ + 2;
        if (zEnd > 0xF) {
            zEnd = 0xF;
        }
    }

    for (z = zStart; z <= zEnd; z++) {
        void **row = (void **)(g_060B659C + z * 0x80);
        for (x = xStart; x <= xEnd; x++) {
            void *cand = row[x];
            while (cand != (void *)0) {
                if (M32(cand, 4) != M32(0x060B88A0, 0)) {
                    if (aabb_overlap_3(box_min, box_max,
                                       (s32 *)((u8 *)cand + 0x28),
                                       (s32 *)((u8 *)cand + 0x34))) {
                        return cand;
                    }
                }
                cand = MP(cand, 0x44);
            }
        }
    }
    return (void *)0;
}
