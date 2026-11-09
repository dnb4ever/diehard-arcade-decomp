/* actor_compute_motion_components [0x0222acf8]: Clamps motion speed and derives horizontal/vertical components from heading and pitch. */
#include "dh.h"
/* 0x0222acf8 (actor_motion_resolve) - resolve an the actor driven motion/position.
 * Clamp a->f74 (speed) to <= 0x3000, then:
 *   a->f78 = -fixmul(fixmul((a->f74>>1) + (a->f64<<8), cos_lookup(a->f60)),
 *                    sin_lookup(a->f62));
 *   a->fc4 = g_06096C58[a->f34] + a->f0c - 0xC000;
 *   a->fc8 = fixmul(a->f74, sin_lookup(a->f60));
 * sin_lookup / cos_lookup are the in-cart trig lookups; fixmul = func_022131c8.
 * fidelity: HIGH - PIC thunks resolved (bsrf -> 0x213190/0x2131a8, r13 -> fixmul
 * 0x2131c8); f74/f78/fc4/fc8/f0c long, f60/f62/f64 s16, f34 s8 index. */
extern s32 sin_lookup(s16 angle);    /* trig lookup */
extern s32 cos_lookup(s16 angle);    /* trig lookup */
extern s32 fixed_mul16(s32 a, s32 b); /* fixmul_16_16 */
extern s32 g_06096C58[];

void actor_compute_motion_components(void *a) {
    s32 t1, t2;

    if (M32(a, 0x74) > 0x3000)
        M32(a, 0x74) = 0x3000;

    t1 = sin_lookup(M16(a, 0x62));
    t2 = cos_lookup(M16(a, 0x60));
    M32(a, 0x78) = -fixed_mul16(
        fixed_mul16((M32(a, 0x74) >> 1) + (M16(a, 0x64) << 8), t2), t1);
    M32(a, 0xC4) = g_06096C58[M8(a, 0x34)] + M32(a, 0x0C) - 0xC000;
    M32(a, 0xC8) = fixed_mul16(M32(a, 0x74), sin_lookup(M16(a, 0x60)));
}
