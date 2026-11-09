/* shc_copy_word_block [0x0224a280]: Copies a compiler-runtime word block with the original short/long path store order. */
#include "dh.h"
/* 0x0224a280  block_copy - compiler runtime word-block copy (struct assignment helper).
 *
 * ABI (NON-STANDARD, register-passed):
 *   in : r0 = byte count (multiple of 4), r1 = dst, r2 = src
 *   out: nothing. r3 and r2 are saved/restored; r0 is clobbered, and on the
 *        long path r1 is left advanced. Callers model it as (count, dst, src).
 *
 * MECHANISM (decoded from ROM):
 *   count <= 64 (unsigned): jmp through the word table @0x0604e2dc (== cart
 *     0x24a2dc), indexed by the byte count, into an unrolled ladder at
 *     0x24a296..0x24a2d4 that copies words count/4-1 .. 0, i.e. DESCENDING
 *     (highest word first).  count == 0 lands on the rts and copies nothing.
 *     (A count that is not a multiple of 4 would be a misaligned table read in
 *     the original; no caller does that - all pass 108.)
 *   count  > 64: an ascending two-words-per-iteration loop that copies word i
 *     while src + 4*(i+1) <= src + count, i.e. floor(count/4) words (it reads
 *     one word past the end before exiting; that load has no effect).
 * The copy order differs between the two paths, which only matters for
 * overlapping buffers; both orders are reproduced here.
 *
 * fidelity: HIGH - exact store sequence (order, width, values) for both paths. */
void shc_copy_word_block(s32 count, void *dst, void *src)
{
    u32 n = (u32)count;
    u32 i;

    if (n <= 64) {
        for (i = n / 4; i != 0; i--)
            MU32(dst, (i - 1) * 4) = MU32(src, (i - 1) * 4);
        return;
    }
    for (i = 0; i < n / 4; i++)
        MU32(dst, i * 4) = MU32(src, i * 4);
}
