/* stage_build_render_grid [0x02216c88]: Copies the stage object list into render nodes linked by XZ grid bucket. */
#include "dh_band21.h"
/* 0x0216c88 func_02216c88 (role: stage_build_object_grid) - bsr worker split off from the
 * set-piece/stage initialiser dispatcher 0x0216c38 (which calls it as func_02216c88(stage,
 * variant, rec); only `stage` is used here - variant/rec are passed but ignored, kept in the
 * signature to match the caller).
 *
 * It clears a 32x32 spatial-hash bucket grid (0x060B3D5C, 0x100 x 16-byte cells = 0x1000 bytes)
 * via func_02213290, then walks the stage's source object list (head =
 * *(0x0606080C + stage*0x2C), 0x14-byte records terminated by a record whose s16 @+0x12 < 0),
 * copying each into the 0x18-byte output array at 0x060B4D5C and chaining it at the head of the
 * grid bucket keyed by its X/Z position. Finally it resets the control block at 0x060B655C.
 *
 * Output record layout (from src): +4 = src+0 (x32), +8 = src+4 (y32), +C = src+8 (z32),
 * +10 = src+C, +12 = src+E, +14 = src+10, +16 = src+12 (all s16); +0 = bucket link (next).
 *
 * Bucket index: base 0x060B3D5C + (cz << 7) + (cx << 2), where
 *   cz = (s16)((u32)((s32)out.zC >> 2) >> 16)   (row stride 0x80 = 32 ptr)
 *   cx = (s16)((u32)((s32)out.x4 >> 2) >> 16)    (4 bytes per bucket)
 * i.e. each coord is arithmetic->2 then its high signed 16 bits select the cell.
 *
 * Call resolved via braf_resolve: func_02213290 (0x213290, delta 0xffffc5ec; base r4, n r5).
 * fidelity: HIGH - hand-traced vs fn.py disasm (0x216c88..0x216d28); the pool at 0x216d2a+ is
 * data. This is the real entry 0x216c88 that owns the grid-build loop the census merged with
 * 0x216c38. */

extern void memory_clear_vec4_blocks(u32 base, s32 n);   /* 0x213290 */

void stage_build_render_grid(s32 stage, s32 variant, u32 rec)
{
    u32 src;
    u32 out;
    u32 bucket;
    s16 cz, cx;

    (void)variant;
    (void)rec;

    memory_clear_vec4_blocks(0x060b3d5c, 0x100);

    src = MEMU32(0x0606080c + stage * 0x2C);
    out = 0x060b4d5c;

    while (MEM16(src + 0x12) >= 0) {
        MEM32(out + 0x04) = MEM32(src + 0x00);
        MEM32(out + 0x08) = MEM32(src + 0x04);
        MEM32(out + 0x0C) = MEM32(src + 0x08);
        MEM16(out + 0x10) = MEM16(src + 0x0C);
        MEM16(out + 0x12) = MEM16(src + 0x0E);
        MEM16(out + 0x14) = MEM16(src + 0x10);
        MEM16(out + 0x16) = MEM16(src + 0x12);

        cz = (s16)((u32)((s32)MEM32(out + 0x0C) >> 2) >> 16);
        cx = (s16)((u32)((s32)MEM32(out + 0x04) >> 2) >> 16);
        bucket = 0x060b3d5c + (cz << 7) + (cx << 2);

        MEM32(out + 0x00) = MEM32(bucket);
        MEM32(bucket) = (s32)out;

        src += 0x14;
        out += 0x18;
    }

    MEM16(0x060b655c + 0x20) = 0x1000;
    MEM16(0x060b655c + 0x32) = 0;
    MEM16(0x060b655c + 0x30) = 0;
    MEM16(0x060b655c + 0x22) = 0;
    MEM32(0x060b655c + 0x10) = 0;
    MEM32(0x060b655c + 0x0C) = 0;
    MEM32(0x060b655c + 0x08) = 0;
    MEM16(0x060b655c + 0x28) = 0;
    MEM16(0x060b655c + 0x26) = 0;
    MEM16(0x060b655c + 0x24) = 0;
}
