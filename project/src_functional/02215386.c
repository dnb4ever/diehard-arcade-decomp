/* sprite_clear_scratch_heads [0x02215386]: Clears the sprite scratch line heads by sixteen-byte strides. */
/* 0x02215386 (clear_sprite_scratch) - clear the 0x200-byte scratch region at
 * 0x060d5ccc by tail-calling the strided zero-fill helper
 * func_022148ac(base=0x060d5ccc, len=0x200).
 * NOTE: fn.extent over-runs this entry into the following (already-decompiled)
 * function at 0x2153f6 because the braf tail-branch is not treated as a terminator;
 * this TU models only 0x215386's own body, which ends at that braf.
 * fidelity: faithful - two literal args (r4=0x060d5ccc, r5=0x200) then braf tail-call
 *   resolved to func_022148ac. */
#include "dh_band21.h"

extern void cache_clear_line_heads(u32 base, u32 len);

void sprite_clear_scratch_heads(void) {
    cache_clear_line_heads(0x060d5ccc, 0x200);
}
