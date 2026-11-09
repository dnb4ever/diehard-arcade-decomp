/* heap_free_coalesce [0x02214c8a]: Inserts a freed block into the ordered free list and merges adjacent neighbours. */
#include "dh_band21.h"
/* 0x02214c8a (block_free_coalesce) - K&R-style free() into an address-ordered free
 * list whose base header lives at 0x06092230. Each node = {next@0, size@4}. Walk
 * from the base header to the predecessor node p such that the freed block bp sorts
 * right after it, then splice bp in, coalescing with the successor and/or the
 * predecessor whenever they are physically adjacent (node + node->size == neighbour).
 * fidelity: HIGH - list-walk sense (stop at next==0 or bp<next), both adjacency
 *   tests and the size merges match the asm. */
void heap_free_coalesce(u32 bp)
{
    u32 p = 0x06092230;   /* free-list base header */
    u32 next;

    next = MEMU32(p);
    while (next != 0) {
        if (bp < next) break;
        p = next;
        next = MEMU32(p);
    }

    if (bp + MEMU32(bp + 4) == next) {        /* adjacent to successor */
        MEMU32(bp + 0) = MEMU32(next + 0);
        MEMU32(bp + 4) = MEMU32(bp + 4) + MEMU32(next + 4);
    } else {
        MEMU32(bp + 0) = next;
    }

    if (p + MEMU32(p + 4) == bp) {            /* adjacent to predecessor */
        MEMU32(p + 0) = MEMU32(bp + 0);
        MEMU32(p + 4) = MEMU32(p + 4) + MEMU32(bp + 4);
    } else {
        MEMU32(p + 0) = bp;
    }
}
