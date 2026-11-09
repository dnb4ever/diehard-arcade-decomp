/* tilemap_blit_biased [0x02216714]: Signed byte dimensions, source pointer+4, 0x80 row stride and halfword tile bias. */
#include "dh_band21.h"
/* 0x02216714 (tilemap_blit_add) - copy a cols x rows block of 16-bit cells from a
 * descriptor's source buffer (desc->unk4) into a 128-byte-stride destination, adding
 * a constant bias to each cell. desc->unk0 = columns (byte), desc->unk1 = rows (byte).
 * dst = base + (row << 7) + col*2; each output row advances by 0x80 bytes.
 * fidelity: HIGH - mov.w cells (post-inc by one element), mov.b signed dims, 0x80 stride. */
void tilemap_blit_biased(s32 add, s32 base, void *desc, s32 col, s32 row) {
    const s16 *src = *(const s16 * const *)((const u8 *)desc + 4);
    s32 cols = *(const s8 *)desc;             /* desc->unk0 (signed byte) */
    s32 rows = *((const s8 *)desc + 1);       /* desc->unk1 (signed byte) */
    s16 *dst = PTR(s16, base + (row << 7) + col * 2);
    s32 y, x;

    for (y = 0; y < rows; y++) {
        s16 *rowp = dst;
        for (x = 0; x < cols; x++) {
            *dst = (s16)(*src + add);
            src++;
            dst++;
        }
        dst = (s16 *)((u8 *)rowp + 0x80);
    }
}
