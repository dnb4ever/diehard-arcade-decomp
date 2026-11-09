/* 0x02244d0a  obj_latch_reset: if the global mode-gate is armed, latch a one-shot and clear
 * three s16 fields of the active object. Second function of the 0x244cd2 fn-unit (reached
 * only by direct call; sits after the 0x244cd2 braf tail). Leaf, no args.
 *   if (g_06098670 != 1) return;
 *   if (g_06098830 <  1) return;
 *   p = *(void**)0x0609882c;
 *   q = *(void**)(p + 8);
 *   g_06098830 = 1;
 *   *(u8*)(p + 13) = 1;
 *   *(s16*)(q+0xae) = 0; *(s16*)(q+0xb0) = 0; *(s16*)(q+0xb2) = 0;
 * fidelity: HIGH (extra - split out of fn-unit member 0x244cd2). cmp/eq #1 and cmp/ge #1
 *   gates verified; p is re-read from 0x0609882c for the +13 store (no intervening write,
 *   so a single load is equivalent); field widths match (mov.l gates, mov.b +13, mov.w +0xae..). */
#include "decomp.h"

void obj_latch_reset(void) {
    u8 *p, *q;
    if (MEM32(DH_PLAY_SESSION_MODE_ADDR) != 1) return;
    if ((s32)MEM32(DH_VIEW_REQUEST_PRIORITY_ADDR) < 1) return;
    p = *(u8 **)(uintptr_t)DH_VIEW_TASK_ADDR;
    q = *(u8 **)(p + 8);
    MEM32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 1;
    *(p + 13) = 1;
    *(s16 *)(q + 0xae) = 0;
    *(s16 *)(q + 0xb0) = 0;
    *(s16 *)(q + 0xb2) = 0;
}
