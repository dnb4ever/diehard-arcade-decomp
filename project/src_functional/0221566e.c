/* 0x0221566e  emit_sprite_cmd: if rec[+0x14] (depth) is in [0x10000, 0x1fbfff), build a
 * VDP command entry into the buffer *(void**)0x060cf0c8 and advance that slot by -36.
 *   dst[+32] = depth
 *   ti  = rec.s16[+2];  idx = s16table[0x060cf0cc][ti];  ent = 0x060d28cc + idx*16
 *   dst.s16[+0]  = rec.s16[+26] | 2
 *   dst.s16[+4]  = ent.s16[+8] | rec.s16[+24]
 *   dst.s16[+6]  = 0
 *   dst.s16[+8]  = ent.s16[+0];  dst.s16[+10] = ent.s16[+2]
 *   dst.s32[+12..+24] = rec.s32[+4..+16]
 *   (*0x0609881c)++   [asm reads the 0x2609881c cache-through mirror, same cell]
 * fidelity: faithful - cmp/ge range gate, table lookup (shll idx, shll2 x2 = *16), mov.w/mov.l
 *   field copies, counter increment, slot -= 36. Leaf (no calls). */
#include "dh_band21.h"

void emit_sprite_cmd(s8 *rec) {
    s32 depth = *(s32 *)(rec + 0x14);
    s8 **slot;
    s8 *dst;
    s16 ti, idx;
    s8 *ent;

    if (depth < 0x10000) return;
    if (depth >= 0x1fbfff) return;

    slot = PTR(s8 *, 0x060cf0c8);
    dst = *slot;
    *(s32 *)(dst + 32) = depth;

    ti  = *(s16 *)(rec + 2);
    idx = MEM16(0x060cf0cc + ti * 2);
    ent = PTR(s8, 0x060d28cc) + idx * 16;

    *(s16 *)(dst + 0)  = (s16)(*(s16 *)(rec + 26) | 2);
    *(s16 *)(dst + 4)  = (s16)(*(s16 *)(ent + 8) | *(s16 *)(rec + 24));
    *(s16 *)(dst + 6)  = 0;
    *(s16 *)(dst + 8)  = *(s16 *)(ent + 0);
    *(s16 *)(dst + 10) = *(s16 *)(ent + 2);
    *(s32 *)(dst + 12) = *(s32 *)(rec + 4);
    *(s32 *)(dst + 16) = *(s32 *)(rec + 8);
    *(s32 *)(dst + 20) = *(s32 *)(rec + 12);
    *(s32 *)(dst + 24) = *(s32 *)(rec + 16);

    MEM32(0x0609881c) = MEM32(0x0609881c) + 1;
    *slot = dst - 36;
}
