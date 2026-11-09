/* effect_cancel_on_timer5a [0x022301ac]: Decrements work halfword+5a and clears task handler exactly at zero. */
#include "dh.h"
/* 0x022301ac (leafC) - decrement 16-bit timer at off 90 of obj *(arg0+8);
 * when it reaches 0, clear *arg0.
 * fidelity: HIGH - exact (mov.w @(r0,r5) r0=90, exts.w+tst for zero test). */
void effect_cancel_on_timer5a(void *arg0) {
    void *o = MP(arg0, 0x8);
    s16 t = (s16)(M16(o, 90) - 1);
    M16(o, 90) = t;
    if (t == 0)
        M32(arg0, 0) = 0;
}
