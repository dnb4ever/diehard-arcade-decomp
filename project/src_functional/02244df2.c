/* 0x02244df2  rec_delta_angle_neg: look up a 0x100-byte record in g_06097d58 indexed by
 * (s8)(actor+0x34), form the delta (dx,dy) between two of its points, pass them to the
 * angle helper atan2_fixed, and return the negated result.
 *   rec = (s32*)(0x06097d58 + (s8)(actor+0x34) * 0x100);
 *   dx = rec[4] - rec[0];   dy = rec[5] - rec[1];
 *   return -atan2_fixed(dx, dy);
 * fidelity: HIGH - record base scaled by 0x100 (mov.b sign-extend + shll8); dx/dy computed
 *   via stack spills (compiler scratch) then passed r4=dx, r5=dy; bsrf delta 0xfffce28e
 *   resolves to 0x2130b4; result negated in the rts delay slot. atan2_fixed is not yet
 *   decompiled but is called faithfully (likely a vector->angle helper in the 0x213xxx math
 *   band). */
#include "decomp.h"

s32 atan2_fixed(s32 x, s32 y);   /* 0x022130b4 (math band; undecompiled) */

s32 rec_delta_angle_neg(u8 *actor) {
    s32 *rec = (s32 *)(uintptr_t)(0x06097d58 + (*(s8 *)(actor + 0x34)) * 0x100);
    s32 dx = rec[4] - rec[0];
    s32 dy = rec[5] - rec[1];
    return -atan2_fixed(dx, dy);
}
