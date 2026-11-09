/* 0x02244d40  clear slot-table[f34] (@0x06098838): .f0=0, .f4=0x12C. Leaf; equivalence-verified. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_244d40(Actor *a) {
    s8 i;
    i = a->f34;
    if (i < 8) {
        ((G *)0x06098838)[i].f0 = 0;
        ((G *)0x06098838)[a->f34].f4 = 0x12C;
    }
}
