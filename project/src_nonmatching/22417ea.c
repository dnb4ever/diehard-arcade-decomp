/* 0x022417ea  set facing f26 + flag f44 from state f4D (2 pairs). Leaf; equivalence-verified. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_2417ea(Actor *a) {
    s8 s;
    s = a->f4d;
    switch (s) {
    case 0:
    case 1:
        a->f44 = 0;
        a->f26 = 0xC000;
        return;
    case 2:
    case 3:
        a->f44 = 1;
        a->f26 = 0x4000;
        return;
    }
}
