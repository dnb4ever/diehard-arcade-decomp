/* 0x02219c22  store_count_minus1: obj[+24] = (s16)(*(s16*)(q + 14)) - 1,
 *   where q = *(void**)((*(void**)0x0609882c)[+8] + 0xd4).
 * fidelity: faithful - deref chain (0x0609882c -> +8 -> +0xd4), mov.w @(14) sign-extended load,
 *   add #-1, mov.l store to @(24,r4) (rts delay). Leaf. */
#include "dh_band21.h"

void store_count_minus1(s8 *obj) {
    s8 *p = *(s8 **)(*(s8 **)DH_VIEW_TASK_ADDR + 8);
    s8 *q = *(s8 **)(p + 0xd4);
    *(s32 *)(obj + 24) = (s32)(*(s16 *)(q + 14)) - 1;
}
