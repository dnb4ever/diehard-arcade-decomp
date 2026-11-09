/* hud_load_indexed_panel [0x0223828c]: Bounds index below eight; palette and indexed texture DMA into VDP2 memory. */
#include "dh.h"
/* 0x0223828c (callC) - if (u32)arg0 >= 8 return. Else two DMA copies via sh2_dmac_copy
 * (cart 0x2145d0 -> RAM 0x060185d0):
 *   sh2_dmac_copy(*(void**)0x225B9610, (void*)0x25F003C0, 8);
 *   entry = *(0x0608C488 + 16*arg0); n = ((*(s32)entry) * 128) >> 2 (round toward 0);
 *   sh2_dmac_copy(*(void**)(entry+4), (void*)0x25E66E80, n);  [tail call]
 * fidelity: MED - arg0 unsigned; table stride 16; divide-by-4 via +3-when-negative then shar twice. */
extern s32 g_0608C488;
extern void sh2_dmac_copy(void *, void *, s32);
void hud_load_indexed_panel(s32 arg0) {
    void *entry;
    s32 n;
    if ((u32)arg0 >= 8)
        return;
    sh2_dmac_copy(*(void **)0x225B9610, (void *)0x25F003C0, 8);
    entry = MP(&g_0608C488, 16 * arg0);
    n = M32(entry, 0) * 128;
    if (n < 0)
        n += 3;
    n = (n >> 1) >> 1;
    sh2_dmac_copy(MP(entry, 4), (void *)0x25E66E80, n);
}
