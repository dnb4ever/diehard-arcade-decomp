/* 0x022417a4  set facing f26 (0x4000/0xC000) + flag f44 from state f4D (4 cases). Leaf; equivalence-verified. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_2417a4(Actor *a) {
    s8 s;
    s = a->f4d;
    switch (s) {
    case 0:
        a->f44 = 0;
    b10:
        a->f26 = 0xC000;
        return;
    case 1:
        a->f44 = 0;
    b8:
        a->f26 = 0x4000;
        return;
    case 2:
        a->f44 = 1;
        goto b8;
    case 3:
        a->f44 = 1;
        goto b10;
    }
}
