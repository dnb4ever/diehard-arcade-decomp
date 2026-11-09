/* 0x02249fb0  actor state=0x13 setter (leaf): fd4=0x13,f84=1,f88=0,fe0&=0x300. Leaf; equivalence-verified (instr stream matches). */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_249fb0(Actor *a) {
    a->fd4 = 0x13;
    a->f84 = 1;
    a->f88 = 0;
    a->fe0 &= 0x300;
}
