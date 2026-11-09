/* stage_build_actor_grid [0x02216f7a]: Stage+4 0x1c-byte list populates 32x16 dynamic collision grid after clear. */
#include "dh_band21.h"
/* 0x0216f7a func_02216f7a (role: stage_build_actor_grid) - tail-jump worker split off from the
 * set-piece/stage initialiser dispatcher 0x0216c38 (called as func_02216f7a(stage)). It is the
 * second real entry the census merged into 0x216c38; it builds the dynamic actor/collision grid,
 * complementing func_02216c88 (which builds the static-geometry grid at 0x060B3D5C).
 *
 * stage_rec = 0x0606080C + stage*0x2C. list = *(stage_rec + 4) (0x1C-byte nodes); the stage
 * world-space base offset is stage_rec + 8 (x @+0, y @+4, z @+8), matching the dispatcher's
 * 0x0609742C seed.
 *
 * Steps:
 *  1. func_02213290(0x060B659C, 0x80): zero the 32x16 spatial-grid bucket array (0x800 bytes)
 *     used by grid_bucket_insert_cached.
 *  2. Zero fields +0/+4/+0x18 of the 32 collision-box records g_060B6D9C (stride 0x48).
 *  3. For up to 64 list nodes (stop when node+0xC == 0): build a slot record at 0x060B769C
 *     (stride 0x48) - clear +0/+4, set +8 = -1, cache coords +0xC/+0x10/+0x14 = node coords
 *     (node+0/+4/+8) biased by the stage base offset, copy AABB +0x18/+0x1C/+0x20/+0x24 =
 *     node+0xC/+0x10/+0x14/+0x18 - then grid_bucket_insert_cached(record). Advance node by 0x1C.
 *  4. Install the command handler: *(0x060B889C) = func_02217888 (PIC mova 0x217034 + 0x854),
 *     *(0x060B88A0) = -1 (reset the dispatch ctx).
 *
 * Calls resolved: func_02213290 (bsrf 0x213290, delta 0xffffc2ec); grid_bucket_insert_cached
 * (bsr 0x217ca2, PC-relative direct). The two per-loop "index*stride" address computations the
 * compiler emitted collapse to the base (index 0 at entry); the real advance is the +0x48
 * post-increment, re-rolled here as base + k*0x48.
 *
 * fidelity: HIGH - hand-traced vs fn.py disasm (0x216f7a..0x217052); 0x217054+ is the data pool. */

extern void memory_clear_vec4_blocks(u32 base, s32 n);   /* 0x213290 */
extern void collision_grid_insert_cached(u32 rec);              /* 0x217ca2 grid_bucket_insert_cached */
extern void grid_find_overlap_any(void);                 /* 0x217888 installed command handler */

void stage_build_actor_grid(s32 stage)
{
    u32 stage_rec = 0x0606080c + stage * 0x2C;
    u32 base      = stage_rec + 8;     /* world-space base offset (x/y/z @ +0/+4/+8) */
    u32 node      = MEMU32(stage_rec + 4);
    u32 rec;
    s32 k;

    memory_clear_vec4_blocks(0x060b659c, 0x80);

    for (k = 0; k < 32; k++) {
        u32 box = DH_ACTOR_MOVEMENT_RECORDS_ADDR + (u32)k * 0x48;
        MEM32(box + 0x00) = 0;
        MEM32(box + 0x04) = 0;
        MEM32(box + 0x18) = 0;
    }

    rec = 0x060b769c;
    for (k = 0; k < 64; k++) {
        if (MEM32(node + 0x0C) == 0)
            break;
        MEM32(rec + 0x00) = 0;
        MEM32(rec + 0x04) = 0;
        MEM32(rec + 0x08) = -1;
        MEM32(rec + 0x0C) = MEM32(node + 0x00) + MEM32(base + 0x00);
        MEM32(rec + 0x10) = MEM32(node + 0x04) + MEM32(base + 0x04);
        MEM32(rec + 0x14) = MEM32(node + 0x08) + MEM32(base + 0x08);
        MEM32(rec + 0x18) = MEM32(node + 0x0C);
        MEM32(rec + 0x1C) = MEM32(node + 0x10);
        MEM32(rec + 0x20) = MEM32(node + 0x14);
        MEM32(rec + 0x24) = MEM32(node + 0x18);
        collision_grid_insert_cached(rec);
        rec  += 0x48;
        node += 0x1C;
    }

    MEM32(0x060b889c) = (s32)(uintptr_t)grid_find_overlap_any;
    MEM32(0x060b88a0) = -1;
}
