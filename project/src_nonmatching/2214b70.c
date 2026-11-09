/* 0x02214b70  heap_alloc: first-fit over free list @0x06092230, split/exact-fit. Leaf; equivalence-verified logic (SHC reg-alloc/spill differs). */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void *func_214b70(Pool *pool, u32 size) {
    FB **pp;
    FB *b;
    FB *nb;
    if (size == 0) {
        return (void *)0;
    }
    pp = (FB **)0x06092230;
    size = (size + 23) & ~15;
    while ((b = *pp) != 0) {
        if (b->f4 >= size) {
            if (b->f4 == size) {
                *pp = b->f0;
            } else {
                nb = (FB *)((char *)b + size);
                nb->f0 = b->f0;
                nb->f4 = b->f4 - size;
                b->f4 = size;
                *pp = nb;
            }
            b->f0 = (FB *)pool->f8;
            pool->f8 = b;
            return (void *)((char *)b + 8);
        }
        pp = (FB **)b;
    }
    return (void *)0;
}
