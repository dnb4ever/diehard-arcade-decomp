/* 0x02249f08  actor state=0x12 setter (leaf): f5C from table @0x060900E0. Leaf; equivalence-verified (instr stream matches). */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_249f08(Actor *a) {
    a->fd4 = 0x12;
    a->f84 = 1;
    a->f88 = 0;
    a->fe0 &= 0x300;
    a->f108 = 0;
    a->f41 = 1;
    a->f5c = *(s16 *)(0x060900E0 + *(s32 *)0x06097418 * 2);
    if (*(s32 *)0x06097418 == 1) {
        a->fbc |= 0x7FFFFF00;
    }
}
