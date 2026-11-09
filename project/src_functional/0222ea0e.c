/* 0x0222ea0e  record_copy_48w: copy a 96-byte (48 x s16) record, indexed by `idx`
 * (record stride 108 bytes), from table g_06096910 to g_06096328.
 *   off = (s16)(108 * idx)   (muls.w then exts.w of the low 16 bits)
 *   src = g_06096910 + off;  dst = g_06096328 + off
 *   copies 16 groups of 3 shorts (loop counter 16, -2 per pass, 2 triples/pass).
 * fidelity: HIGH - muls.w/exts.w index, mov.w triple copies unrolled x2, dt-style
 *   add #-2 / tst / bf.s loop over r6, macl save/restore (callee-saved, no effect). */
#include "dh_band21.h"

void record_copy_48w(s32 idx) {
    s32 off = (s16)(108 * (s16)idx);
    u16 *src = (u16 *)(uintptr_t)(0x06096910 + off);
    u16 *dst = (u16 *)(uintptr_t)(0x06096328 + off);
    int i;

    for (i = 0; i < 16; i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        src += 3;
        dst += 3;
    }
}
