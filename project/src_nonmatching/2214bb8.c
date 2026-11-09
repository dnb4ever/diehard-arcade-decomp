/* 0x02214bb8  alloc_array_zeroed: heap_alloc(count*size) then HWRAM zero-fill. Calls intra-unit + 1 PIC thunk. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void hw_ffffe69c(void *dst, u32 words);
void *func_214bb8(Pool *pool, u32 count, u32 size) {
    void *p;
    u32 total;
    total = size * count;
    p = func_214b70(pool, total);
    if (p != 0) {
        hw_ffffe69c(p, ((total + 3) & ~3) >> 2);
    }
    return p;
}
