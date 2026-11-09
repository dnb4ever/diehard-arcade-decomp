/* display_mark_channel_bits [0x02238dde]: Sets bit five in all three shared display accumulator words. */
#include "dh.h"
/* 0x02238dde (leafC) - OR 0x20 into each of three consecutive 16-bit words at 0x0609861E.
 * fidelity: HIGH - exact (mov.w @r4/@(2,r4)/@(4,r4), or #32). */
extern s16 g_0609861E;
void display_mark_channel_bits(void) {
    void *p = &g_0609861E;
    M16(p, 0) = (s16)(M16(p, 0) | 32);
    M16(p, 2) = (s16)(M16(p, 2) | 32);
    M16(p, 4) = (s16)(M16(p, 4) | 32);
}
