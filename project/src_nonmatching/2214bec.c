/* 0x02214bec  heap_realloc: alloc-copy-free or in-place shrink; calls heap_alloc/heap_free/coalesce + 1 PIC copy thunk. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void hw_fffff8e8(void *src, void *dst, u32 words);
extern void func_214c8a(FB *b);
extern void func_214c66(Pool *pool, void *ptr);
extern void *func_214b70(Pool *pool, u32 size);
void *func_214bec(void *arg0, void *oldp, u32 newsize) {
    FB *oh;
    void *np;
    FB *nb;
    s32 rem;
    oh = (FB *)((char *)oldp - 8);
    np = func_214b70((Pool *)arg0, newsize);
    if (np != 0) {
        u32 cap = oh->f4 - 8;
        if (newsize > cap) {
            newsize = cap;
        }
        hw_fffff8e8(oldp, np, ((newsize + 1) & ~1) >> 1);
        func_214c66((Pool *)arg0, oldp);
        return np;
    }
    newsize = (newsize + 23) & ~15;
    if ((u32)oh->f4 >= newsize) {
        if ((u32)oh->f4 != newsize) {
            rem = oh->f4 - newsize;
            nb = (FB *)((char *)oh + newsize);
            nb->f0 = oh->f0;
            nb->f4 = rem;
            oh->f4 = newsize;
            func_214c8a(nb);
        }
        return oldp;
    }
    return (void *)0;
}
