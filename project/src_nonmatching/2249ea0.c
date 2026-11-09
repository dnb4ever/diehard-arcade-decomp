/* 0x02249ea0  actor state=0x12 setter variant; 1 PIC thunk, branch on f38&0x7800. Functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern s32 hw_fffc6140(void);
void f_249ea0(Actor *a) {
    void *t = a->f180;
    a->fd4 = 0x12;
    a->f84 = 1;
    a->f88 = 0;
    a->fe0 &= 0x300;
    a->f108 = 6;
    a->f41 = 1;
    if ((a->f38 & 0x7800) && ((hw_fffc6140() & 0xF) == 0)) {
        a->fe0 |= 0x02000000;
        a->f5c = *(s16 *)((char *)t + 0xA);
        return;
    }
    a->f5c = *(s16 *)((char *)t + 4);
}
