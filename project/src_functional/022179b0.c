/* grid_find_overlap_static [0x022179b0]: Searches overlap buckets while excluding dynamic types one through seven and the active parent. */
#include "dh.h"
/* 0x022179b0  grid_overlap_find_skiptypes: spatial-hash neighbour search.
 *
 * Identical scan to func_02217888 over the g_060B659C grid, but the candidate
 * filter excludes node types 1..7 as well as the active-parent id g_060B88A0:
 * the overlap test runs only when ((u32)(node->unk4 - 1) >= 7) i.e. node->unk4 is
 * 0 or >= 8, and node->unk4 != g_060B88A0. First overlapping node is returned.
 *
 * fidelity: HIGH. bsrf resolved to aabb_overlap_3 (0x213354). */
extern u8  g_060B659C[];
extern s32 aabb_overlap_3(s32 *a, s32 *b, s32 *c, s32 *d);

void *grid_find_overlap_static(void *pos, void *ext) {
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
                s32 type = M32(cand, 4);
                if (((u32)(type - 1) >= 7U) && (type != M32(0x060B88A0, 0))) {
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
