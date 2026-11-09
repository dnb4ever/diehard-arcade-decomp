/* 0x02213d18  build_attr_block_4: for each of 4 index bytes read from `stream`, use byte*16
 * as an offset into `table`, load 3 longs [+0,+4,+8], and emit to `dst`:
 *   dst.half[k*2+0] = (s16) table_entry[+0]
 *   dst.half[k*2+1] = (s16)(-table_entry[+4])
 *   dst.long[4 + k]  = table_entry[+8]
 * NOTE: the triage unit 0x2213d18 begins with 3 shared literal-pool longwords
 *   (0x06098b1c, 0x06098878, 0x06098b9c - scratch ptrs for the matrix routines);
 *   the executable entry is 0x2213d24. Those words are not referenced by this body.
 * fidelity: faithful - 4 unrolled entries, extu.b + shll2 x4 (=*16) index, mov.w/mov.l stores,
 *   neg on the +4 field. Leaf (saves r9/r10). */
#include "dh_band21.h"

void build_attr_block_4(u8 *stream, s16 *dst, s8 *table) {
    int k;
    for (k = 0; k < 4; k++) {
        s32 *e = (s32 *)(table + ((u32)*stream++ << 4));   /* byte index * 16 */
        dst[k * 2 + 0] = (s16)e[0];
        /* ROM 06017d50..17d66: depth store precedes negated Y. */
        *(s32 *)((s8 *)dst + 16 + k * 4) = e[2];
        dst[k * 2 + 1] = (s16)(-e[1]);
    }
}
