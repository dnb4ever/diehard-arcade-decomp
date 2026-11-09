/* stats_add_duration_histogram [0x0220a9c2]: Adds work+68 then increments bucket min(arg/1200,30) at work+280. */
#include "dh.h"
/* 0x0220a9c2 resident_query_seq(arg0) - runs a sequence of resident-manager queries
 * through the RAM function pointer g_06000644, keyed on *g_06098804 (a base pointer).
 *   r = g_06000644(1, base+0x68, 0);
 *   g_06000644(3, base+0x68, arg0 + r);
 *   q = (u32)arg0 / 1200u;                  (unsigned div via helper 0x24A120, div0u)
 *   dst = (q < 30u) ? base + q*4 + 0x280 : base + 0x2F8;
 *   return g_06000644(2, dst, (u16)(g_06000644(0, dst, 0) + 1));
 * fidelity: HIGH - the computed-branch "call" into 0x24A120 is the compiler unsigned
 * divide helper (div0u/div1 loop computing r1/r0), rendered as /; cmp/hs => unsigned
 * compare; every g_06000644 fp call + offsets taken directly from the asm. */
extern s32 (*g_06000644)(s32 sel, void *p, s32 v);  /* resident-manager query fp */
extern void *g_06098804;                            /* base pointer global */

s32 stats_add_duration_histogram(s32 arg0) {
    void *base = g_06098804;
    void *dst;
    u16 v6;
    u32 q;
    s32 r;

    r = g_06000644(1, (u8 *)base + 0x68, 0);
    g_06000644(3, (u8 *)base + 0x68, arg0 + r);
    q = (u32)arg0 / 1200U;
    if (q < 30U) {
        dst = (u8 *)base + q * 4 + 0x280;
    } else {
        dst = (u8 *)base + 0x2F8;
    }
    v6 = (u16)(g_06000644(0, dst, 0) + 1);
    return g_06000644(2, dst, v6);
}
