/* heap_allocate_owned [0x02214b70]: Takes a first-fit aligned heap block, links it to the owning task and returns the payload. */
#include "dh.h"
/* 0x02214b70 (heap_alloc) - first-fit allocator over the free list anchored at
 * g_06092230. Rounds the request up: need = (size + 0x17) & ~0xF (8-byte header +
 * 16-byte granularity). Walks the free list; the first block whose size >= need is
 * taken (unlinked if exact, otherwise split, leaving the tail on the free list),
 * pushed onto the owner pool's allocated list (pool+8), and its payload (block+8)
 * returned. Returns NULL for size 0 or when no block fits.
 * Block header: next @0, size @4. Called by alloc_array_zeroed (0x02214bb8).
 * fidelity: HIGH - all mov.l (32-bit); block size compared unsigned (cmp/hs). */
typedef struct Block {
    struct Block *next;   /* +0 */
    u32           size;   /* +4 */
} Block;

extern Block *g_06092230;   /* heap free-list head */

void *heap_allocate_owned(void *pool, s32 size) {
    Block **link;
    Block *blk;
    u32 need;

    if (size == 0)
        return (void *)0;

    link = &g_06092230;
    need = ((u32)size + 0x17) & ~0xFu;
    while ((blk = *link) != (Block *)0) {
        if (blk->size >= need) {
            if (blk->size == need) {
                *link = blk->next;
            } else {
                Block *split = (Block *)((u8 *)blk + need);
                split->next = blk->next;
                split->size = blk->size - need;
                blk->size = need;
                *link = split;
            }
            blk->next = *(Block **)((u8 *)pool + 8);   /* push onto pool alloc list */
            *(Block **)((u8 *)pool + 8) = blk;
            return (u8 *)blk + 8;
        }
        link = &blk->next;   /* next link slot (= &block, since next is at +0) */
    }
    return (void *)0;
}
