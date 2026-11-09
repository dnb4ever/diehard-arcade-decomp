/* actor_combo_reset [0x022275e4]: Existing map corroborated by source: leaf; clears halfword slot unk34*2 across tables 0x06096E90/CA0/EA0/EB0 and actor->unk52. sibling helper tail-called by actor_clash_state. */
#include "dh.h"
/* 0x022275e4  clear four 16-bit table slots indexed by arg0->unk34, and arg0->unk52.
 * fidelity: HIGH - exact. */
extern u8 g_06096E90[];
extern u8 g_06096CA0[];
extern u8 g_06096EA0[];
extern u8 g_06096EB0[];
void actor_combo_reset(void *arg0) {
    s32 i = M8(arg0, 0x34) * 2;
    M16(&g_06096E90[i], 0) = 0;
    M16(&g_06096CA0[i], 0) = 0;
    M16(&g_06096EA0[i], 0) = 0;
    M16(&g_06096EB0[i], 0) = 0;
    M16(arg0, 0x52) = 0;
}
