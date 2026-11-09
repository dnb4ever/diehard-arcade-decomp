/* actor_reset_pose_history [0x02223e70]: Fills ten slot history entries and clears the four associated history indices. */
#include "dh.h"
/* 0x02223e70  fill 10 6-byte slots of the per-id record @0x06096CB0 (stride 60) with the
 * low 16 bits of table word 0x06096C74[id], zero the +2 sub-field, then clear 4 index tables.
 * fidelity: HIGH - exact; table word read 32-bit, stored 16-bit (constant across the loop). */
extern u8 g_06096C74[]; /* 32-bit table, 4-byte stride */
extern u8 g_06096CB0[]; /* 60-byte-per-id record array */
extern u8 g_06096E90[];
extern u8 g_06096CA0[];
extern u8 g_06096EA0[];
extern u8 g_06096EB0[];
void actor_reset_pose_history(void *arg0) {
    s32 idx = M8(arg0, ACTOR_SLOT_ID);
    s16 val = (s16)M32(&g_06096C74[idx * 4], 0);
    u8 *dst = &g_06096CB0[(s16)(idx * 60)];
    s32 i = 0;
    do {
        M16(dst, 0x0) = val;
        i += 2;
        M16(dst, 0x2) = 0;
        M16(dst, 0x6) = val;
        M16(dst, 0x8) = 0;
        dst += 12;
    } while (i < 10);
    {
        s32 j = idx * 2;
        M16(&g_06096E90[j], 0) = 0;
        M16(&g_06096CA0[j], 0) = 0;
        M16(&g_06096EA0[j], 0) = 0;
        M16(&g_06096EB0[j], 0) = 0;
    }
}
