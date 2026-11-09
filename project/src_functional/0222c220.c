/* player_try_special_move [0x0222c220]: Tests the three action-button chord and actor gates before arming the special move. */
#include "dh.h"
/* 0x0222c220 (leafC) - gate + arm a special move for an actor; returns -1 when armed, else 0.
 * idx = obj->field34; pad = PAD_RECS[(idx-1)*0x18] (0x18-byte input records @0x06098734).
 * Requires pad.f0 & 1, pad.f4 & 2, pad.f4 & 4, obj->field84 & 0x381D, !(obj->field8C & 0xC00),
 * obj->field56 > 15, and g_0609979A == 0, else returns 0. If (field84 & 0x2000) and
 * obj->fieldC < 0x1000, returns 0. Arms the move: if (field8C & 0x18) sets field84 = 0x02000000,
 * field88 = 0x55; otherwise computes a landing target fieldC4 = g_06096C58[idx] + fieldC - 0xC000,
 * fieldC8 = (0x30000 - fieldC4) >> 4, field78 = 0, field84 = 0x01000000, field88 = 0x54. Common
 * tail: field80 = 0, field5C = g_06087EB0[field45], field41 = 1, field43 = 0x2A, fieldB0 = 0,
 * fieldCC = 0; returns -1.
 * fidelity: HIGH - widths from the loads (pad f0/f4 u16, field56/5C/CC 16-bit, field41/43/45 8-bit,
 * the rest 32-bit; g_0609979A s16; g_06096C58 s32; g_06087EB0 s16). Leaf. */
extern s16 g_actor_pose_set;
extern s32 g_06096C58[];
extern s16 g_06087EB0[];
#define PAD_RECS ((char *)DH_PLAYER_INPUT_RECORDS_ADDR)
s32 player_try_special_move(void *obj) {
    s8 idx = M8(obj, 0x34);
    char *pad = PAD_RECS + (s8)((idx - 1) * 0x18);
    s32 f84, f8C;
    if (!(MU16(pad, 0) & DH_PAD_ACTION1)) return 0;
    if (!(MU16(pad, 4) & DH_PAD_ACTION2)) return 0;
    if (!(MU16(pad, 4) & DH_PAD_ACTION3)) return 0;
    f84 = M32(obj, 0x84);
    if (!(f84 & 0x381D)) return 0;
    f8C = M32(obj, 0x8C);
    if (f8C & 0xC00) return 0;
    if (M16(obj, 0x56) <= 15) return 0;
    if (g_actor_pose_set != 0) return 0;
    if ((f84 & 0x2000) && M32(obj, 0xC) < 0x1000) return 0;
    if (f8C & 0x18) {
        M32(obj, 0x84) = 0x02000000;
        M32(obj, 0x88) = 0x55;
    } else {
        M32(obj, 0xC4) = g_06096C58[idx] + M32(obj, 0xC) + 0xFFFF4000;
        M32(obj, 0xC8) = (0x30000 - M32(obj, 0xC4)) >> 4;
        M32(obj, 0x78) = 0;
        M32(obj, 0x84) = 0x01000000;
        M32(obj, 0x88) = 0x54;
    }
    M32(obj, 0x80) = 0;
    M16(obj, 0x5C) = g_06087EB0[M8(obj, 0x45)];
    M8(obj, 0x41) = 1;
    M8(obj, 0x43) = 0x2A;
    M32(obj, 0xB0) = 0;
    M16(obj, 0xCC) = 0;
    return -1;
}
