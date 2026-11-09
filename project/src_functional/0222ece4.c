/* collision_sweep_grid [0x0222ece4]: Scans clamped 5x5 grid neighbourhoods, applies layer masks and dispatches narrow-phase tests. */
#include "dh.h"
/* 0x0222ece4 - broad-phase collision sweep over the spatial-hash grid.
 *
 * Walks the active-object list at g_06097554 (node->unk0 = next, node->unk4 =
 * entity). For each entity "self":
 *   col = 0x1F & ((self->unk8  - g_0609742c) >> 16)
 *   row = 0x0F & ((self->unk10 - g_06097434) >> 16)
 * (a 32x16 cell grid; g_0609742c/g_06097434 are the world/view origin). It
 * builds a 5x5 neighbourhood window [col-2..col+2] x [row-2..row+2] clamped to
 * [0,0x1F] x [0,0x0F].
 *
 * It then calls the transform/AABB helper vec3_minmax(self+8, self+0x4c,
 * &g_06097438, &g_06097444) and contracts the working box by the entity radius
 * self->unk64: mins g_06097438/3c/40 += radius, maxes g_06097444/48/4c -= radius.
 *
 * Finally it scans every bucket in the window (grid base g_06097558, row stride
 * 0x80, cell stride 4; bucket is a linked list via ->unk0, element entity via
 * ->unk4). For each "other" with unk48==0 it applies a two-sided layer/mask gate
 * (table g_0604ebe4 indexed by unk34; entities with unk62&0x40 use mask
 * 0x80000000) against the collision masks unkBC / unk68, and on pass dispatches
 * the narrow-phase test: unk34<8 -> func_0222ef38, else func_0222f1d0. A hit with
 * (self->unk62 & 0x3F) >= 0x0B clears self->unk58.
 *
 * fidelity: HIGH - hand-verified vs m2c_fn --asm (fn.py 0x22ece4). The single
 * bsrf @0x22ed6c -> vec3_minmax (braf_resolve, delta 0xfffe4564);
 * func_0222ef38 / func_0222f1d0 are direct bsr. r12 is const 0 (unk58 cleared).
 * Field widths: unk8/10/64/BC/68/438.. are longs, unk62 halfword, unk48/unk34
 * bytes, unk58 halfword. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern void vec3_minmax(void *a, void *b, void *minp, void *maxp);
extern s32  collision_test_pose_hitboxes(void *other, void *self);
extern s32  collision_test_swept_hitbox(void *other, void *self);

void collision_sweep_grid(void) {
    void *node = (void *)(uintptr_t)(u32)G32(DH_COLLISION_ACTIVE_HEAD_ADDR);

    while (node != 0) {
        void *self = MP(node, 4);
        s32 col = 0x1F & (s16)((u32)(M32(self, 8)   - G32(0x0609742c)) >> 16);
        s32 row = 0x0F & (s16)((u32)(M32(self, 0x10) - G32(0x06097434)) >> 16);
        s32 col_min = col - 2, col_max = col + 2;
        s32 row_min = row - 2, row_max = row + 2;
        s32 radius, r, c;

        if (col_min < 0)      col_min = 0;
        if (col_max >= 0x20)  col_max = 0x1F;
        if (row_min < 0)      row_min = 0;
        if (row_max >= 0x10)  row_max = 0x0F;

        radius = M32(self, 0x64);
        vec3_minmax((void *)((u8 *)self + 8), (void *)((u8 *)self + 0x4c),
                      (void *)(uintptr_t)0x06097438, (void *)(uintptr_t)0x06097444);
        G32(0x06097438) += radius;
        G32(0x0609743c) += radius;
        G32(0x06097440) += radius;
        G32(0x06097444) -= radius;
        G32(0x06097448) -= radius;
        G32(0x0609744c) -= radius;

        for (r = row_min; r <= row_max; r++) {
            void *rowbase = (void *)(uintptr_t)(DH_COLLISION_GRID_HEADS_ADDR + r * 0x80);
            for (c = col_min; c <= col_max; c++) {
                void *cell = MP(rowbase, c * 4);
                while (cell != 0) {
                    void *other = MP(cell, 4);
                    if (MU8(other, 0x48) == 0) {
                        u32 mask = (MU16(self, 0x62) & 0x40)
                                     ? 0x80000000u
                                     : (u32)G32(DH_ACTOR_SLOT_MASKS_ADDR + MU8(self, 0x34) * 4);
                        if (!(mask & MU32(other, 0xBC)) &&
                            !((u32)G32(DH_ACTOR_SLOT_MASKS_ADDR + MU8(other, 0x34) * 4) & MU32(self, 0x68))) {
                            s32 hit = (MU8(other, 0x34) < 8)
                                        ? collision_test_pose_hitboxes(other, self)
                                        : collision_test_swept_hitbox(other, self);
                            if (hit != 0 && (MU16(self, 0x62) & 0x3F) >= 0x0B)
                                M16(self, 0x58) = 0;
                        }
                    }
                    cell = MP(cell, 0);
                }
            }
        }
        node = MP(node, 0);
    }
}
