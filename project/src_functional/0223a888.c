/* effect_project_source_motion [0x0223a888]: Projects source pitch, heading and magnitude into effect motion components. */
#include "dh.h"
/* 0x0223a888 - project a source (arg1) polar motion into output fields on arg0.
 * Reads source angles unk60/unk62 and magnitudes unk64/unk74; clamps
 *   m = min(unk74 + 0x1000, 0x3000) and writes:
 *   r = func_022131c8((m>>1) + (unk64<<8), cos_lookup(unk60));
 *   arg0->unk78 = -func_022131c8(r, sin_lookup(unk62));
 *   arg0->unk80 =  func_022131c8(r, cos_lookup(unk62));
 *   arg0->unkC4 =  arg0->unkC;
 *   arg0->unkC8 =  func_022131c8(m, sin_lookup(unk60)) + 0x1000;
 * cos_lookup / sin_lookup are the unary (cos/sin-style) lookups,
 * func_022131c8 the binary fixed-point combine (reached via mova-computed ptr).
 * branches resolved: bsrf 0x2131a8 (x2), 0x213190 (x2); jsr 0x2131c8.
 * fidelity: HIGH (helper roles inferred from argument shape). */
extern s32 cos_lookup(s16 angle);
extern s32 sin_lookup(s16 angle);
extern s32 fixed_mul16(s32 mag, s32 trig);

void effect_project_source_motion(void *arg0, void *arg1) {
    s32 m;
    s32 r;

    m = M32(arg1, 0x74) + 0x1000;
    if (m > 0x3000)
        m = 0x3000;

    r = fixed_mul16((m >> 1) + (M16(arg1, 0x64) << 8), cos_lookup(M16(arg1, 0x60)));
    M32(arg0, 0x78) = -fixed_mul16(r, sin_lookup(M16(arg1, 0x62)));
    M32(arg0, 0x80) =  fixed_mul16(r, cos_lookup(M16(arg1, 0x62)));
    M32(arg0, 0xC4) =  M32(arg0, 0xC);
    M32(arg0, 0xC8) = fixed_mul16(m, sin_lookup(M16(arg1, 0x60)));
    M32(arg0, 0xC8) += 0x1000;
}
