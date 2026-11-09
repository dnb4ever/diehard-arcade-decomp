/* 0x022100c8  draw-text variant, args passed by value on stack {dst,attr,s[]}. Leaf; equivalence-verified (struct-by-value shape inferred). */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

struct textcmd { short *dst; short attr; char s[2]; };
void f_2100c8(struct textcmd cmd) {
    short *dst = cmd.dst;
    char *p = cmd.s;
    char c;
    do {
        c = *p++;
        *dst++ = c | cmd.attr;
    } while (*p != 0);
}
