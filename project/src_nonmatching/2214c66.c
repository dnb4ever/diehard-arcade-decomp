/* 0x02214c66  heap_free: unlink from used list then coalesce into sorted free list (helper at 0x214c8a, same function). Leaf logic. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void func_214c8a(FB *b);

void func_214c66(Pool *pool, void *ptr) {
    FB **pp;
    FB *b;
    if (ptr == 0) {
        return;
    }
    b = (FB *)((char *)ptr - 8);
    pp = (FB **)&pool->f8;
    while (*pp != b) {
        pp = (FB **)*pp;
    }
    *pp = b->f0;
    func_214c8a(b);
}

void func_214c8a(FB *b) {
    FB *prev;
    FB *cur;
    prev = (FB *)0x06092230;
    cur = prev->f0;
    while (cur != 0) {
        if ((u32)b < (u32)cur) {
            break;
        }
        prev = cur;
        cur = cur->f0;
    }
    if ((char *)b + b->f4 == (char *)cur) {
        b->f0 = cur->f0;
        b->f4 = b->f4 + cur->f4;
    } else {
        b->f0 = cur;
    }
    if ((char *)prev + prev->f4 == (char *)b) {
        prev->f0 = b->f0;
        prev->f4 = prev->f4 + b->f4;
    } else {
        prev->f0 = b;
    }
}
