/* collision_grid_insert_cached [0x02217ca2]: Same bucket/bounds insertion from record's cached X/Y/Z. */
#include "dh.h"
#include <stdint.h>
/* 0x02217ca2 (grid_insert_cached) - same spatial-grid insert as grid_insert_live
 * (0x02217c10) but hashing the object`s own cached coords (obj+0xC, +0x10, +0x14)
 * instead of dereferencing obj+0x8. Runs only when obj+0x18 > 0 and obj+0x20 > 0.
 * col = (s16)((cx - g_0609742C) >> 16) & 0x1F, row = (s16)((cz - g_06097434) >> 16)
 * & 0xF, slot = &g_060B659C[row*32 + col]; pushes obj, sets obj+0x0 = 1 and the AABB.
 * fidelity: HIGH - verified against the asm. */
extern s32 g_0609742C;
extern s32 g_06097434;
extern s32 g_060B659C[];

void collision_grid_insert_cached(void *e) {
    if (M32(e, 0x18) > 0 && M32(e, 0x20) > 0) {
        s32 cx = M32(e, 0xC), cy = M32(e, 0x10), cz = M32(e, 0x14);
        s32 col = (s16)((u32)(cx - g_0609742C) >> 16) & 0x1F;
        s32 row = (s16)((u32)(cz - g_06097434) >> 16) & 0xF;
        s32 *slot = &g_060B659C[row * 32 + col];

        M32(e, 0x40) = (s32)(uintptr_t)slot;
        M32(e, 0x44) = *slot;
        *slot = (s32)(uintptr_t)e;
        M32(e, 0x0) = 1;
        M32(e, 0x28) = cx - M32(e, 0x18);
        M32(e, 0x2C) = cy - M32(e, 0x1C);
        M32(e, 0x30) = cz - M32(e, 0x20);
        M32(e, 0x34) = cx + M32(e, 0x18);
        M32(e, 0x38) = cy + M32(e, 0x24);
        M32(e, 0x3C) = cz + M32(e, 0x20);
    }
}
