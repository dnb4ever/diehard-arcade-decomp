/* 0x02219984  anim_counter_step: 3-state step driven by obj.s8[+16].
 *   a = *(void**)(obj+8);  b = *(void**)(*(void**)0x0609882c + 8);
 *   state 0: a[+24]=0; a[+28]=b[+12]; obj.s8[+16]++  (fall through to state 1)
 *   state 1: b[+12] = *(s32*)(0x060907b0 + a[+24]*4) + a[+28];
 *            a[+24]++; if (a[+24] > 11) obj.s8[+16]++
 *   state 2: *(s32*)obj = 0
 * fidelity: faithful - mov.b state load, cmp/eq #0/#1/#2 dispatch with case-0 fall-through,
 *   shll2 table index, cmp/gt #11 gate, byte state increments. Leaf. */
#include "dh_band21.h"

void anim_counter_step(s8 *obj) {
    s8 *a = *(s8 **)(obj + 8);
    s8 *b = *(s8 **)(*(s8 **)DH_VIEW_TASK_ADDR + 8);
    s32 state = *(s8 *)(obj + 16);

    switch (state) {
    case 0:
        *(s32 *)(a + 24) = 0;
        *(s32 *)(a + 28) = *(s32 *)(b + 12);
        *(s8 *)(obj + 16) = (s8)(*(s8 *)(obj + 16) + 1);
        /* fall through */
    case 1: {
        s32 idx = *(s32 *)(a + 24);
        *(s32 *)(b + 12) = MEM32(0x060907b0 + idx * 4) + *(s32 *)(a + 28);
        idx = *(s32 *)(a + 24) + 1;
        *(s32 *)(a + 24) = idx;
        if (idx > 11) {
            *(s8 *)(obj + 16) = (s8)(*(s8 *)(obj + 16) + 1);
        }
        break;
    }
    case 2:
        *(s32 *)obj = 0;
        break;
    }
}
