/* task_alloc_zeroed_array [0x02214bb8]: Allocates count*size bytes owned by pool/task, then clears rounded-up longs. */
#include "dh.h"
/* 0x02214bb8 (alloc_array_zeroed) - allocate count*size bytes via heap_alloc
 * (func_02214b70) and, on success, zero the block with zero_longs (word fill),
 * passing the rounded-up word count ((total + 3) & ~3) >> 2. Returns the block
 * (NULL on allocation failure).
 * fidelity: HIGH - total = size*count (mul.l); word count matches add #3/and ~3/shlr2. */
extern void *heap_allocate_owned(void *pool, s32 size);        /* heap_alloc    */
extern void  zero_longs(void *dst, u32 words);        /* word zero-fill */

void *task_alloc_zeroed_array(void *pool, s32 count, s32 size) {
    u32 total = (u32)size * (u32)count;
    void *p = heap_allocate_owned(pool, (s32)total);

    if (p != (void *)0)
        zero_longs(p, ((total + 3) & ~3u) >> 2);
    return p;
}
