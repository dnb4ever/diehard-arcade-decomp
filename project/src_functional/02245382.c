/* actors_test_directional_range [0x02245382]: X distance in [m,m+n] and absolute Z within q return signed side or zero. */
#include "dh.h"
/* 0x02245382  directional proximity test between objects a and b on x (off 8)
 * and z (off 0x10). dx = b[8]-a[8]; sign = (dx>0)?+1:-1; dx=|dx|.
 * Require m <= dx <= m+n (args r6=m, r7=n). Then require |b[0x10]-a[0x10]| <= q
 * (5th arg, stack). Returns sign (+1/-1) on hit, 0 on miss.
 * fidelity: HIGH - exact; r6/r7 are args 3/4, q is the 5th (stacked) arg; the
 * dead spill of dz to the stack slot is omitted. */
s32 actors_test_directional_range(void *a, void *b, s32 m, s32 n, s32 q) {
    s32 dx = M32(b, 8) - M32(a, 8);
    s32 sign;
    if (dx > 0) {
        sign = 1;
    } else {
        dx = -dx;
        sign = -1;
    }
    s32 rx = dx - m;
    if (rx < 0)
        return 0;
    if (rx > n)
        return 0;
    s32 dz = M32(b, 0x10) - M32(a, 0x10);
    s32 adz = (dz >= 0) ? dz : -dz;
    if (adz > q)
        return 0;
    return sign;
}
