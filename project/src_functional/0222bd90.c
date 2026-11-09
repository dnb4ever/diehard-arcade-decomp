/* player_init_thrown_physics [0x0222bd90]: Heading/facing, horizontal motion and fall accumulator seeded from current actor state. */
#include "dh.h"
/* 0x0222bd90  init a thrown/launched actor's physics fields from its current state.
 * fidelity: HIGH - exact; field widths: 0x62/0x26=16b, 0x44/0x34/0x43=8b, 0x78/0x7C/0x84/0x88/0xC8=32b. */
extern u8 g_06096C58[]; /* 32-bit per-id table, 4-byte stride */
void player_init_thrown_physics(void *arg0) {
    s16 topbit = (s16)((u16)MU16(arg0, 0x62) >> 15);
    M16(arg0, 0x26) = (s16)(M8(arg0, 0x44) != 0 ? 0x4000 : 0xC000);
    M32(arg0, 0x78) = (topbit != 0) ? 0x8000 : -0x8000;
    {
        s32 idx = M8(arg0, 0x34);
        M32(arg0, 0x7C) = M32(&g_06096C58[idx * 4], 0) + 0xFFFF0000;
    }
    M8(arg0, 0x43)  = 7;
    M32(arg0, 0x84) = 0x2000;
    M32(arg0, 0x88) = 8;
    M32(arg0, 0xC8) = 0;
}
