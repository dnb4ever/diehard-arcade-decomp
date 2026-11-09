/* collision_grid_insert_live [0x02217c10]: Reads owner work position, caches bounds and links into 32x16 bucket grid. */
#include "dh.h"
#include <stdint.h>
/* 0x02217c10 (grid_insert_live) - insert an object into the spatial collision grid
 * using its live coords (from *(obj+0x8) + 8: cx@+8, cy@+12, cz@+16). Runs only when
 * both half-extents obj+0x18 > 0 and obj+0x20 > 0. Hashes cx and cz into a
 * 32(col)x16(row) bucket grid: col = (s16)((cx - g_0609742C) >> 16) & 0x1F,
 * row = (s16)((cz - g_06097434) >> 16) & 0xF, slot = &g_060B659C[row*32 + col].
 * Pushes obj onto that bucket list (obj+0x40 = slot, obj+0x44 = old head, *slot =
 * obj), sets obj+0x0 = 1, caches coords (0xC/0x10/0x14) and the AABB
 * (0x28..0x3C = coord -/+ half-extents). fidelity: HIGH - widths and hash verified. */
extern s32 g_0609742C;
extern s32 g_06097434;
extern s32 g_060B659C[];

void collision_grid_insert_live(void *e) {
    if (M32(e, 0x18) > 0 && M32(e, 0x20) > 0) {
        s32 *base = (s32 *)(uintptr_t)M32(e, 0x8);
        s32 cx = base[2], cy = base[3], cz = base[4];
        s32 col = (s16)((u32)(cx - g_0609742C) >> 16) & 0x1F;
        s32 row = (s16)((u32)(cz - g_06097434) >> 16) & 0xF;
        s32 *slot = &g_060B659C[row * 32 + col];

        M32(e, 0x40) = (s32)(uintptr_t)slot;
        M32(e, 0x44) = *slot;
        *slot = (s32)(uintptr_t)e;
        M32(e, 0x0) = 1;
        M32(e, 0xC) = cx;
        M32(e, 0x10) = cy;
        M32(e, 0x14) = cz;
        M32(e, 0x28) = cx - M32(e, 0x18);
        M32(e, 0x2C) = cy - M32(e, 0x1C);
        M32(e, 0x30) = cz - M32(e, 0x20);
        M32(e, 0x34) = cx + M32(e, 0x18);
        M32(e, 0x38) = cy + M32(e, 0x24);
        M32(e, 0x3C) = cz + M32(e, 0x20);
    }
}
