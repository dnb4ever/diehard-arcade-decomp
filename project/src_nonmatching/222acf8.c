/* 0x0222acf8  actor physics/position update: 3 PIC thunk calls (HWRAM 0xFFFE84xx) + indirect fn. Approximate (PIC args). */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

extern s16 hw_fffe8474(s16);
extern s16 hw_fffe8482(s16);
extern s16 hw_fffe842c(s16);
typedef s32 (*fn2)(s32, s32);
void f_22acf8(Actor *a) {
    fn2 fn = (fn2)0xFFFE84B4;   /* PIC thunk (mova + 0xFFFE84B4) */
    s32 t1, t2;
    if (a->f74 > 0x3000) {
        a->f74 = 0x3000;
    }
    t1 = hw_fffe8474(a->f62);
    t2 = hw_fffe8482(a->f60);
    a->f78 = -fn(fn((a->f74 >> 1) + (a->f64 << 8), t2), t1);
    a->fc4 = *(s32 *)(0x06096C58 + a->f34 * 4) + a->f0c - 0xC000;
    a->fc8 = fn(a->f74, hw_fffe842c(a->f60));
}
