/* actor_motion_init_source_alias [0x022415ee]: Existing map corroborated by source: (member) actor_facing_clamp(actor); clamp f74<=0x3000 (then dead <=0x4000); actor+0xec = -fixmul(fixmul((f74>>1)+(a64<<8), cos_lookup(a60)), sin_lookup(a62)); actor+0xf0 = fixmul(f74, sin_lookup(a60)); actor+0xc = g_06096c58[(s8)actor+0x34] (-0xC000 unless actor+0xe0&0x80000); actor+0x62 writeback; actor+0xf4=0. Sibling of actor_motion_resolve 0x22acf8 */
#include "dh_work.h"
/* 0x022415ee  actor_motion_init - clamp speed and derive per-axis motion vectors.
 * ROM call order: func_022416c4(actor); clamp +0x74 to 0x3000; sample +0x60/+0x62 (s16);
 * clamp +0x74 to 0x4000 (dead after first clamp; kept faithful); sin62=func_02213190(a62);
 * cos60=func_022131a8(a60); a64=+0x64; +0xEC = -fixmul(fixmul((f74>>1)+(a64<<8), cos60), sin62);
 * sin60=func_02213190(a60); +0xF0 = fixmul(f74, sin60);
 * base = g_06096C58[(s8)+0x34]; if (!(+0xE0 & 0x80000)) base += 0xFFFF4000 (-0xC000);
 * +0x0C = base; +0x62 = (s16)a62 write-back; +0xF4 = 0.
 * fidelity: HIGH - calls pinned: sin=0x213190, cos=0x2131a8, fixmul=0x2131c8 (jsr @r11
 * = mova 0x241624 + 0xfffd1ba4); every call sequenced in ROM order via temporaries. */
#include "dh.h"

s32 sin_lookup_entry(s32 angle);      /* sin_lookup: trig table */
s32 cos_lookup_entry(s32 angle);      /* cos_lookup: trig table +0x800 */
s32 fixed_mul16(s32 a, s32 b);   /* 16.16 fixed multiply: (a*b)>>16 */
void actor_clamp_target_heading(void *a);       /* actor_facing_clamp */
extern s32 g_06096C58[];           /* per-type base value table */

void actor_motion_init_source_alias(void *actor) {
    s32 a60, a62, a64, sin62, cos60, inner, sin60, base;

    actor_clamp_target_heading(actor);

    if (ACTOR_M32(actor, motion_speed) > 0x3000)
        ACTOR_M32(actor, motion_speed) = 0x3000;
    a60 = ACTOR_M16(actor, motion_pitch);
    a62 = ACTOR_M16(actor, motion_heading);
    if (ACTOR_M32(actor, motion_speed) > 0x4000)
        ACTOR_M32(actor, motion_speed) = 0x4000;

    sin62 = sin_lookup_entry(a62);    /* 0x241628: first trig call (r4=r13=a62) */
    cos60 = cos_lookup_entry(a60);    /* 0x241630: second trig call (r4=r12=a60) */

    a64 = M16(actor, 0x64);        /* 0x24163c */
    inner = fixed_mul16((ACTOR_M32(actor, motion_speed) >> 1) + (a64 << 8), cos60); /* 0x241642 */
    M32(actor, 0xEC) = -fixed_mul16(inner, sin62); /* 0x241648 + neg 0x24164e */

    sin60 = sin_lookup_entry(a60);    /* 0x241656 */
    M32(actor, 0xF0) = fixed_mul16(ACTOR_M32(actor, motion_speed), sin60); /* 0x24165e */

    base = g_06096C58[ACTOR_M8(actor, slot_id)]; /* s8 index, <<2, load 0x06096C58 */
    if ((ACTOR_M32(actor, state_flags) & 0x80000) == 0)
        base -= 0xC000;            /* add 0xFFFF4000 */
    ACTOR_M32(actor, position_y) = base;

    ACTOR_M16(actor, motion_heading) = (s16)a62;   /* 16-bit write-back of sampled angle */
    M32(actor, 0xF4) = 0;
}
