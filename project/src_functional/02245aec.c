/* enemy_select_farther_player_side [0x02245aec]: Selects the ready player's distance/side record, choosing the farther side when both are ready. */
#include "dh.h"
/* 0x02245aec  compute a value from g_060995d0[field[0x34]] (32-byte elems),
 * selected by the global mode g_06098564.
 *   mode 0 -> 0
 *   mode 1 -> elem[+4] & ~1
 *   mode 2 -> 1 | (elem[+8] & ~1)
 *   mode 3 -> a=elem[+4], b=elem[+8]; (a>b) ? a&~1 : 1|(b&~1)   (unsigned cmp)
 *   other -> field[0x34] (the raw index byte)
 * fidelity: HIGH - exact (shll2;shll2;shll = *32 stride; cmp/hi = unsigned). */
extern s32 g_060995d0[];   /* array of 32-byte structs */
extern u32 g_ready_player_mask;     /* selector / mode */
s32 enemy_select_farther_player_side(void *p) {
    s32 idx = M8(p, 0x34);
    s32 *elem = &g_060995d0[idx * 8];   /* 32 bytes == 8 * s32 */
    switch (g_ready_player_mask) {
    case 0:
        return 0;
    case 1:
        return elem[1] & ~1;
    case 2:
        return 1 | (elem[2] & ~1);
    case 3: {
        u32 a = (u32)elem[1], b = (u32)elem[2];
        if (a > b)
            return (s32)(a & ~1u);
        return (s32)(1 | (b & ~1u));
    }
    default:
        return idx;
    }
}
