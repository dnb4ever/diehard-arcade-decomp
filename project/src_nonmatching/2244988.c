/* 0x02244988  large multi-entry input/state handler; ROM shares one bitfield tail across entries via internal bsr. Functional model of entry+tail. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

/* f_244988: large multi-entry state handler (internal bsr-shared tails in ROM).
   Functional model of the entry path + shared bitfield helper (equivalence
   approximation; the ROM body has several entry points sharing one tail). */
void f_244988(Actor *a, s16 *arg1) {
    s16 t;
    a->f118 = arg1;
    a->f11c = 0;
    a->f11e = 0;
    a->f58 = 0;
    a->f41 = 1;
    t = *arg1;
    a->f5c = 0x1FFF & t;
    if (0x8000 & (u16)t) {
        a->f84 |= 0x200;
    } else if (0x200 & a->f84) {
        a->f84 &= ~0x200;
    }
    if (0x4000 & (u16)t) {
        a->fe0 |= 0x08000000;
    } else if (0x08000000 & a->fe0) {
        a->fe0 &= 0xF7FFFFFF;
    }
    if ((u16)t & 0x2000) {
        a->f108 = 6;
        return;
    }
    if (a->f108 == 6) {
        a->f108 = 0;
    }
}
