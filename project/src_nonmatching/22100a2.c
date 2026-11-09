/* 0x022100a2  draw text row to tilemap @0x060997B0: dst=(y<<7)+(x<<1)+base, writes (char|attr) per char. Leaf; equivalence-verified. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

void f_2100a2(s32 x, s32 y, s32 attr, char *str) {
    short *dst = (short *)(0x060997B0 + (y << 7) + (x << 1));
    char c;
    do {
        c = *str++;
        *dst++ = c | attr;
    } while (*str != 0);
}
