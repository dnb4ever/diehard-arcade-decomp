/* animation_cell_init [0x0222eb84]: Seeds counter/index/reset and destination slot from actor-indexed table. */
#include "dh.h"
/* 0x0222eb84  init record at 0x06096C94: [2]=1,[0]=0,[4]=0,[6]=[8],
 * [0xA]=word from table 0x06096266 indexed by (arg0->unk34 << 5).
 * fidelity: HIGH - exact. */
extern u8 g_06096C94[];
extern u8 g_06096266[]; /* 16-bit table, 0x20-byte stride */
void animation_cell_init(void *arg0) {
    M16(g_06096C94, 0x2) = 1;
    M8(g_06096C94, 0x0)  = 0;
    M16(g_06096C94, 0x4) = 0;
    M16(g_06096C94, 0x6) = M16(g_06096C94, 0x8);
    {
        s32 idx = M8(arg0, 0x34);
        M16(g_06096C94, 0xA) = M16(&g_06096266[idx << 5], 0);
    }
}
