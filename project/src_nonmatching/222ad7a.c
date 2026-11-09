/* 0x0222ad7a  compute facing nibble into f4D from f54/f26. Leaf; equivalence-verified (instr stream matches; scheduling only). */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_22ad7a(Actor *a) {
    s32 v;
    v = (s32)(0x30 & (u16)a->f54) >> 4;
    if ((u16)a->f26 == 0x4000) {
        v = (~v & 2) | (-3 & v);
    }
    a->f4d = (s8)v;
}
