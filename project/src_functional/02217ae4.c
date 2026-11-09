/* grid_find_overlap_type_zero [0x02217ae4]: Searches neighbouring grid buckets for the first overlapping type-zero node. */
#include "dh.h"
/* 0x02217ae4  grid_overlap_find_type0: spatial-hash neighbour search.
 *
 * Identical scan to func_02217888 over the g_060B659C grid, but only nodes of
 * type 0 (node->unk4 == 0) are considered, and that type-0 node is still skipped
 * when it matches the active-parent id g_060B88A0 (i.e. when g_060B88A0 == 0).
 * First overlapping type-0 node is returned, else NULL.
 *
 * fidelity: HIGH. bsrf resolved to aabb_overlap_3 (0x213354). The two guards are
 * emitted in ROM order: skip when node->unk4 != 0, then skip when node->unk4 ==
 * g_060B88A0. */
extern u8  g_060B659C[];
extern s32 aabb_overlap_3(s32 *a, s32 *b, s32 *c, s32 *d);

void *grid_find_overlap_type_zero(void *pos, void *ext) {
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
                if (type == 0 && type != M32(0x060B88A0, 0)) {
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
