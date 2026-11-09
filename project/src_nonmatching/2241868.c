/* 0x02241868  spawn effect B via PIC thunk (HWRAM 0xFFFD46F2), gated on 0x060E0DDC. Functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void hw_fffd46f2(s32);
void f_241868(void) {
    if (*(s32 *)0x060E0DDC != 0) {
        hw_fffd46f2(0xA01318FF);
    }
}
