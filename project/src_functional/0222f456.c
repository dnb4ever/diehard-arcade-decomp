/* effect_tick_lifetime [0x0222f456]: Signed halfword+58 post-decrement; returns -1 when old value is nonpositive. */
#include "dh.h"
/* 0x0222f456  decrement 16-bit counter at off 0x58; return 0 while it was >0, else -1.
 * fidelity: HIGH - exact (mov.w @(0x58,r4); dec; store; exts.w; cmp/pl). */
s32 effect_tick_lifetime(void *o) {
    s16 v = (s16)(M16(o, 0x58) - 1);
    M16(o, 0x58) = v;
    if ((s16)(v + 1) <= 0) {
        return -1;
    }
    return 0;
}
