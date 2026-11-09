/* 0x0222adaa  actor spawn/reset dispatch via two fn-ptr tables (0x060911BC/0x060911C4) + func_22aadc. Functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void func_22aadc(Actor *a);
typedef void (*fpt)(Actor *, s32);
void f_22adaa(Actor *a) {
    ((fpt *)0x060911BC)[*(s16 *)0x0609979A](a, a->f64);
    func_22aadc(a);
    a->f80 = 0;
    a->f7c = 0;
    a->f48 = 0;
    a->f52 = 0;
    a->f41 = 1;
    if ((u32)(*(s32 *)0x060987BC - a->f110) > 30) {
        a->f10c = 0;
    }
    a->f110 = *(s32 *)0x060987BC;
    ((fpt *)0x060911C4)[a->fba & 0x3F](a, 0x060987BC);
}
