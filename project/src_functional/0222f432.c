/* effect_tick_lifetime_or_owner_flags3800 [0x0222f432]: Decrements +58 and expires on owner action bits 0x3800. */
#include "dh.h"
/* 0x0222f432  decrement counter at o+0x58; -1 if expired OR (p->flags@0x84 & 0x3800).
 * fidelity: HIGH - exact. */
s32 effect_tick_lifetime_or_owner_flags3800(void *o, void *p) {
    s16 v = (s16)(M16(o, 0x58) - 1);
    M16(o, 0x58) = v;
    if ((s16)(v + 1) <= 0 || (M32(p, 0x84) & 0x3800)) {
        return -1;
    }
    return 0;
}
