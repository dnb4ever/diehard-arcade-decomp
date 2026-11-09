/* camera_tick_dual_sine_orbit [0x0221e7d6]: Two phase accumulators drive view heading and X around saved base. */
/* 0x0221e7d6  obj_orbit_update: advance an objects two phase angles and drive a
 * linked sprite record. sub = obj[8]; spr = (*0x0609882c)->unk8.
 *   first call (obj[0x10]==0): sub[0x48] = spr[8]; obj[0x10]++.
 *   every call: sub[0x40] += 0x380; sub[0x44] += 0x280;
 *     spr[0x26] = sin_lookup(sub[0x40]) >> 6;
 *     spr[0x08] = (sin_lookup(sub[0x44]) >> 1) + sub[0x48];
 * fidelity: faithful - two bsrf resolved to sin_lookup (0x213190), shar >>6 / >>1. */
#include "dh.h"

extern s32 sin_lookup(s32 angle);

void camera_tick_dual_sine_orbit(void *obj) {
    void *sub = MP(obj, 8);
    void *spr = MP(MP(DH_VIEW_TASK_ADDR, 0), 8);

    if (M8(obj, 0x10) == 0) {
        M32(sub, 0x48) = M32(spr, 8);
        M8(obj, 0x10) = (s8)(M8(obj, 0x10) + 1);
    }
    M32(sub, 0x40) += 0x380;
    M32(sub, 0x44) += 0x280;
    M16(spr, 0x26) = (s16)(sin_lookup(M32(sub, 0x40)) >> 6);
    M32(spr, 0x08) = (sin_lookup(M32(sub, 0x44)) >> 1) + M32(sub, 0x48);
}
