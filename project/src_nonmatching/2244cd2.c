/* 0x02244cd2  actor init (f60/f62/f74/f64) + PIC thunk + braf tail-dispatch + conditional partner reset. Functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void hw_ffff5cd6(Actor *a);
void f_244cd2(Actor *a) {
    char *o;
    a->f60 = 0x1000;
    a->f62 = a->f26;
    a->f74 = 0x800;
    a->f64 = 0x10;
    hw_ffff5cd6(a);
    a->f45 = 0;
    /* braf tail-dispatch (offset 0x00000F7E) in ROM omitted */
    if (*(s32 *)0x06098670 == 1 && *(s32 *)0x06098830 >= 1) {
        o = (char *)*(void **)((char *)*(void **)0x0609882C + 8);
        *(s32 *)0x06098830 = 1;
        *(s8 *)((char *)*(void **)0x0609882C + 0xD) = 1;
        *(s16 *)(o + 0xAE) = 0;
        *(s16 *)(o + 0xB0) = 0;
        *(s16 *)(o + 0xB2) = 0;
    }
}
