/* actor_fill_pose_history [0x022260c8]: Updates the ten per-slot history values without clearing their indices. */
#include "dh.h"
/* 0x022260c8  fill 10 6-byte slots of the per-id record @0x06096CB0 (stride 60) with the
 * low 16 bits of table word 0x06096C74[id]. (Same fill as 0x02223e70 without the table clears.)
 * fidelity: HIGH - exact. */
extern u8 g_06096C74[];
extern u8 g_06096CB0[];
void actor_fill_pose_history(void *arg0) {
    s32 off = 0;
    s32 i = 0;
    do {
        s32 idx = M8(arg0, 0x34);
        s16 val = (s16)M32(&g_06096C74[idx * 4], 0);
        u8 *rec = &g_06096CB0[(s16)(idx * 60)];
        i += 2;
        M16(&rec[off], 0)     = val;
        M16(&rec[off + 6], 0) = val;
        off += 12;
    } while (i < 10);
}
