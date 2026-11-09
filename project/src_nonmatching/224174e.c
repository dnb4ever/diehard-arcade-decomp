/* 0x0224174e  actor approach-target: clamp delta to +/-0x4000 toward entity-table target. Leaf; equivalence-verified. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_24174e(Actor *a) {
    s8 i;
    s32 d;
    i = a->f4a;
    if (i >= 1 && i <= 2) {
        d = ((E **)0x060995B0)[i]->f8->f10 - a->f10;
        if (d >= 0) {
            if (d > 0x4000) {
                d = 0x4000;
            }
        } else if (d < 0x4000) {
            d = -0x4000;
        }
        a->f10 += d;
    }
}
