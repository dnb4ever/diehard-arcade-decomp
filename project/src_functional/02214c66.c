/* Round 4: 06018c82 tail-branches to shared coalescer 06018c8a after unlinking; preserve the shared boundary instead of duplicating its body. Evidence: work/decomp4/callee_missing.asm. */
/* heap_release_owned [0x02214c66]: Unlinks an owned allocation and returns its header to the coalescing free list. */
#include "dh.h"
/* 0x02214c66 (heap_free) - return a block to the free list (counterpart of heap_alloc
 * 0x02214b70). ptr's header is ptr-8. Walk the owner pool's allocated list (pool+8,
 * linked via @0) to find and unlink the block, then insert it into the address-ordered
 * free list anchored at g_06092230, coalescing with the following block (if block-end
 * meets it) and/or the preceding block (if its end meets the block).
 * Block header: next @0, size @4. No-op when ptr is NULL.
 * fidelity: HIGH - all mov.l; free-list scan uses unsigned address order (cmp/hs). */
typedef struct Block {
    struct Block *next;   /* +0 */
    u32           size;   /* +4 */
} Block;

extern void heap_free_coalesce(void *block);
extern Block *g_06092230;   /* heap free-list head (anchor's next field) */

void heap_release_owned(void *pool, void *ptr) {
    Block *blk, *next, *cur;
    Block **link;

    if (ptr == (void *)0)
        return;

    blk = (Block *)((u8 *)ptr - 8);

    /* unlink from the pool's allocated list */
    link = (Block **)((u8 *)pool + 8);
    while (*link != blk)
        link = &(*link)->next;
    *link = blk->next;

    heap_free_coalesce(blk);
}
