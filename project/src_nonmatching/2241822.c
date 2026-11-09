/* 0x02241822  spawn effect A via PIC thunk (HWRAM 0xFFFD470C), gated on 0x060E0DDC. Functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void hw_fffd470c(s32, s32);
void f_241822(Actor *a) {
    s32 flag;
    s32 face = (u8)a->f49;
    flag = (a->f64 > 0x10) ? 1 : 0;
    if (*(s32 *)0x060E0DDC != 0) {
        hw_fffd470c(((*(s16 *)(0x060542B8 + (((face << 1) + flag) << 1)) << 8) | 0xA00000FF), 0);
    }
}
