/* direction_from_yaw_pitch [0x022104f6]: Outputs -sin(pitch), sin(yaw)*cos(pitch), cos(yaw)*cos(pitch). */
#include "dh_band21.h"

/* 0x022104f6 - build a unit direction vector from two angles a1 (yaw) and
 * a2 (pitch), writing three 16.16 components:
 *     *a3 = -sin(a2)
 *     *a4 =  sin(a1) * cos(a2)        (16.16 fixmul)
 *     *a5 =  cos(a1) * cos(a2)
 * sin/cos via the trig LUT helpers; products via 16.16 fixmul. a1 is used as a
 * 16-bit angle (stored/reloaded as s16); a2 is passed whole to the LUT. cos(a2)
 * is recomputed for each product line, and in the asm it is evaluated BEFORE
 * the a1 lookup (pushed at 210520/21053c, popped into r5 in the fixmul delay
 * slot). a5 is the 5th arg (stack: @(28,r15) after the saved frame). */

extern s32 sin_lookup_entry(s32 ang);      /* 0x213190 sin_lookup */
extern s32 cos_lookup_entry(s32 ang);      /* 0x2131a8 cos_lookup */
extern s32 fixed_mul16(s32 a, s32 b); /* 0x2131c8 fixmul 16.16 */

void direction_from_yaw_pitch(s32 a1, s32 a2, s32 *a3, s32 *a4, s32 *a5)
{
    s32 cos_a2;

    *a3 = -sin_lookup_entry(a2);

    cos_a2 = cos_lookup_entry(a2);
    *a4 = fixed_mul16(sin_lookup_entry((s16)a1), cos_a2);

    cos_a2 = cos_lookup_entry(a2);
    *a5 = fixed_mul16(cos_lookup_entry((s16)a1), cos_a2);
}

/* fidelity: HIGH. All bsrf/mova+add calls resolved (sin/cos/fixmul); per-line
 * call order preserved: cos(a2) is called before the a1 lookup in each fixmul
 * line, matching the push at 210520/21053c and r5 delay-slot pop. a1 truncated
 * to s16 per the mov.w store/reload; a2 passed full-width. */
