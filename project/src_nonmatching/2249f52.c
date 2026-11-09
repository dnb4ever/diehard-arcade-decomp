/* 0x02249f52  actor per-tick dispatch: PIC thunks + tail call via computed ptr. Functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern void hw_ffffea5e(void *);
extern s8 hw_ffffc28c(void *);
extern void hw_ffffca5a(void *);
extern void func_249a82(void *);
typedef void (*fp2)(s32, void *);

void f_249f52(s32 arg0, Actor *a) {
    fp2 end = (fp2)0xFFFE3CCC;
    if (a->f4c != 0) {
        hw_ffffea5e(a);
    } else if (hw_ffffc28c(a) != 0) {
        func_249a82(a);
    } else if ((u16)a->f54 & 2) {
        hw_ffffca5a(a);
        a->fbc = a->f140;
    }
    end(arg0, a);
}
