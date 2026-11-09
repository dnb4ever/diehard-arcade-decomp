#include "dh.h"
/* 0x022227c4  displist_priority_fill - OR a computed 7-bit sort/priority value into the top byte
 * (bits 24..30) of each of 0xF0 32-bit command words in a display list. The priority is
 * ((g_060BACA8 - ((word & 0x7FFFF) >> 15)) + 0xF) & 0x7F, i.e. a reference depth minus the word's
 * top-4 depth bits. Does nothing unless the enable flag g_060BACB4 bit0 is set.
 *   mode 0: list A (g_060BAC98), priority from each word itself.
 *   mode 1: list B (g_060BAC9C), priority from each word itself.
 *   mode 2: list A, but priority depth is taken from A[k] when A[k] > 0 else from B[k] (a merge).
 * fidelity: HIGH - the ROM unrolls modes 0/1 by 3 and mode 2 by 4, and mode 2 keeps two cursors
 * that advance in lockstep (always equal); folded back to one word loop (0xF0 words = 0x3C0 bytes)
 * which is observationally identical. Command words 32-bit, priority read 32-bit. No braf/bsrf. */
extern s32 *g_060BAC98;   /* display list A command buffer */
extern s32 *g_060BAC9C;   /* display list B command buffer */
extern s32  g_060BACA8;   /* reference depth/priority */
extern s32  g_060BACB4;   /* enable flag (bit0) */

static s32 prio_bits(s32 word) {
    return (((g_060BACA8 - ((s32)(word & 0x7FFFF) >> 15)) + 0xF) & 0x7F) << 24;
}

void displist_priority_fill(s32 mode) {
    s32 *a = g_060BAC98;
    s32 *b = g_060BAC9C;
    s32 k;
    if ((g_060BACB4 & 1) == 0)
        return;
    switch (mode) {
    case 0:
        for (k = 0; k < 0xF0; k++)
            a[k] |= prio_bits(a[k]);
        break;
    case 1:
        for (k = 0; k < 0xF0; k++)
            b[k] |= prio_bits(b[k]);
        break;
    case 2:
        for (k = 0; k < 0xF0; k++)
            a[k] |= prio_bits(a[k] > 0 ? a[k] : b[k]);
        break;
    default:
        break;
    }
}
