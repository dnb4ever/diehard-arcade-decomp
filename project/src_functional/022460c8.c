/* enemy_classify_linked_effect [0x022460c8]: Classifies the linked effect count against its limit and current enemy state. */
#include "dh.h"
/* 0x022460c8  classify actor vs animation-frame limit -> 0/1/2/3.
 * flags = field[0x8c];
 *   if (flags & 0x18)  -> 1
 *   if (!(flags & 6))  -> 0
 * i = field[0x6f]; obj = *(g_060e0f4c[i] + 8); cur = obj->field[0x56] (s16);
 * lim = g_0608ddd4[i] (s16);
 *   if (cur >= lim)          -> 1
 *   if (field[0xe4] != 6)    -> 2
 *   if (cur <= 0)            -> 3
 *   else                     -> 1
 * fidelity: HIGH - exact (the 0x2460d6-0x246136 bytes are data/pool, not code;
 * cmp/ge & cmp/pl are signed on the s16 cur). */
extern void *g_060e0f4c[];   /* pointer table (HWRAM) */
extern s16 g_0608ddd4[];     /* per-frame limit table (cart rodata) */
s32 enemy_classify_linked_effect(void *p) {
    s32 flags = M32(p, 0x8C);
    if (flags & 0x18)
        return 1;
    if (!(flags & 6))
        return 0;
    s32 i = M8(p, 0x6F);
    void *obj = MP(g_060e0f4c[i], 8);
    s16 cur = M16(obj, 0x56);
    s16 lim = g_0608ddd4[i];
    if (cur >= lim)
        return 1;
    if (M32(p, 0xE4) != 6)
        return 2;
    if (cur <= 0)
        return 3;
    return 1;
}
