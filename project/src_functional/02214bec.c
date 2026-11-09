/* heap_resize_allocation [0x02214bec]: Allocate/copy/free first; otherwise aligned in-place shrink with free-tail split. */
#include "dh.h"
/* 0x02214bec (heap_realloc) - resize an allocation in the pool at arg0.
 * Header is oldp-8 {next@0, size@4}. First try heap_alloc(pool,newsize): on success,
 * copy min(newsize, oldcap=hdr->size-8) bytes (as ((n+1)&~1)>>1 words) old->new via the
 * SH-2 DMAC copy func_02214510, free the old block (heap_free), return the new block.
 * If heap_alloc fails, try an in-place shrink: round newsize to (newsize+23)&~15; if the
 * block is already >= that, split off the tail into a new free block (func_02214c8a) when
 * strictly larger and return oldp; otherwise return NULL.
 * fidelity: HIGH - unsigned size compares (cmp/hs,cmp/hi), word-count (n+1)/2 copy. */
typedef struct Block { struct Block *next; s32 size; } Block;

extern void  *heap_allocate_owned(void *pool, u32 size);   /* heap_alloc */
extern void   heap_release_owned(void *pool, void *ptr);  /* heap_free */
extern void   heap_free_coalesce(Block *blk);             /* free-list insert/coalesce */
extern void   sh2_dmac_copy_halfwords(void *src, void *dst, u32 words);  /* SH-2 DMAC copy */

void *heap_resize_allocation(void *pool, void *oldp, u32 newsize) {
    Block *hdr = (Block *)((u8 *)oldp - 8);
    void *np = heap_allocate_owned(pool, newsize);

    if (np != (void *)0) {
        u32 cap = (u32)hdr->size - 8;
        if (newsize > cap)
            newsize = cap;
        sh2_dmac_copy_halfwords(oldp, np, ((newsize + 1) & ~1u) >> 1);
        heap_release_owned(pool, oldp);
        return np;
    }

    newsize = (newsize + 23) & ~15u;
    if ((u32)hdr->size >= newsize) {
        if ((u32)hdr->size != newsize) {
            s32 rem = hdr->size - (s32)newsize;
            Block *nb = (Block *)((u8 *)hdr + newsize);
            nb->next = hdr->next;
            nb->size = rem;
            hdr->size = (s32)newsize;
            heap_free_coalesce(nb);
        }
        return oldp;
    }
    return (void *)0;
}
