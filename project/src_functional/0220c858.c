/* camera_test_zone_turn [0x0220c858]: Tests the player zone with margins and writes the successful turn parameters. */
#include "dh.h"
/* func_0220c858 (cart 0x20c858) - leaf, returns s32.
 * Variant of the 0x20cab0 AABB test with different margins/targets and a
 * post-pass writing turn values to e->[0x70]/[0x74].
 * e = arg0->[8]; idx = (u16)e->[0xAE] - 1 then &1. Gated by
 * (0x0604EBE4[idx] & *0x0609977C). Object record at 0x06099764 + (s8)(idx*12),
 * region box at 0x0606080C + *0x060B3D54 * 44. Z band via box+0x20/+0x2C,
 * X band via box+0x14/+0x20. On success stores obj [0],[4],[8] into
 * e->[0x90..0x98], e->[0xAE]=idx+1, then turn bias 0x1C00/-0x1C00/0 into
 * e->[0x70] and 0 into e->[0x74]; returns 1.
 * Note obj address is sign-extended byte-scaled (exts.b of idx*12).
 * fidelity: HIGH. No calls. */

extern s32 g_060B3D54;   /* current room/area index */
extern s32 g_0609977C;   /* gate bitmask */

s32 camera_test_zone_turn(void *arg0)
{
    u32 e   = MU32((u32)arg0, 8);
    s32 idx = (s16)(s16)MU16(e, 0xAE) - 1;
    s32 r7  = idx & 1;
    u32 obj, box;
    s32 ox, oz;

    if (!(M32(DH_ACTOR_SLOT_MASKS_ADDR_U + r7 * 4, 0) & M32(0x0609977Cu, 0)))
        return 0;

    obj = 0x06099764u + (s8)(r7 * 12);   /* exts.b of scaled index */
    box = 0x0606080Cu + M32(0x060B3D54u, 0) * 44;

    /* r14 = box+0x14, r6 = box+0x20 */
    oz = M32(obj, 8);
    if (oz > M32(box + 0x20, 8) - 0x18000)
        return 0;

    ox = M32(obj, 0);
    if (ox < M32(box + 0x14, 0) + 0x18000)
        return 0;
    if (ox > M32(box + 0x20, 0) - 0x18000)
        return 0;

    M16(e, 0xAE) = (u16)(r7 + 1);          /* e+0xAE */
    M32(e, 0x90) = M32(obj, 0);            /* e+0x90 (r0 = 0xAE-0x1E) */
    M32(e, 0x94) = M32(obj, 4) + 0x8000;   /* e+0x94 */
    M32(e, 0x98) = M32(obj, 8);            /* e+0x98 */

    r7 = 0;
    if (ox < M32(box + 0x14, 0) + 0x28000) {
        M32(e, 0x70) = 0x1C00;
    } else if (M32(obj, 0) > M32(box + 0x20, 0) - 0x28000) {
        M32(e, 0x70) = (s32)(s16)-0x1C00;
    } else {
        M32(e, 0x70) = r7;
    }
    M32(e, 0x74) = r7;
    return 1;
}
