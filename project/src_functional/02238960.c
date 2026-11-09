/* effect_spawn_five_table_children [0x02238960]: Five entries from 0608c580/5a8 seed child velocity and model records. */
#include "dh.h"
/* 0x02238960 - build five sub-objects via func_0220ff28 from a local table.
 * The local buffer at sp+0x10 (5 entries of 8 bytes) is seeded from 0x0608c580,
 * and an accompanying s16 table at sp+4 from 0x0608c5a8.
 * sub = *(u32 *)(*(u32 *)(arg0 + 20) + 8) (owner/descriptor block).
 * Per entry i: r0 = func_0220ff28(*(u32 *)0x06099580, 0x060419dc); dst = *(u32 *)(r0 + 8);
 *   dst+20 = *(u32 *)(sp+0x10+8*i); dst+28 = *(u32 *)(sp+0x10+8*i+4);
 *   dst+38 = (s16)entry_s16[i]; if (s8)*(u8 *)(sub + 68) != 0 negate the two
 *   words and add 0x8000 to the halfword; dst+0x60 = 0x10;
 *   idx = (s8)*(u8 *)(sub + 52); triple at 0x06097d88 + (idx << 8) copied to
 *   dst+8/+12/+16; dst+0x30 = 0x2b1;
 *   dst+0x68 = *(u32 *)(0x0604ebe4 + idx*4); dst+52 = (u8)idx;
 *   dst+0x62 = 0; dst+0x64 = 0x1999.
 * Finally *(s32 *)arg0 = 0. */
extern u32 task_spawn_list_work7c(u32 arg, u32 handler);  /* 0x0220ff28 */

void effect_spawn_five_table_children(u32 arg0) {
    u32 buf[10];   /* sp+0x10 .. sp+0x37 : five 8-byte entries */
    u16 vals[5];   /* sp+4 .. : five s16 companions */
    u32 sub;
    s32 i;

    for (i = 0; i < 10; i++) {
        ((u32 *)buf)[i] = MU32(0x0608C580, (u32)(4 * i));
    }
    /* ROM copies two longwords and one halfword: five contiguous s16s. */
    for (i = 0; i < 5; i++) {
        vals[i] = MU16(0x0608C5A8, i * 2);
    }

    sub = MU32(MU32(arg0, 20), 8);

    for (i = 0; i < 5; i++) {
        u32 r0 = task_spawn_list_work7c(MU32(0x06099580, 0), 0x060419dc);
        u32 dst = MU32(r0, 8);

        M32(dst, 20) = ((u32 *)buf)[2 * i];
        M32(dst, 28) = ((u32 *)buf)[2 * i + 1];
        M16(dst, 38) = vals[i];
        if ((s8)MU8(sub, 68) != 0) {
            M32(dst, 20) = (s32)M32(dst, 20) * -1;
            M32(dst, 28) = (s32)M32(dst, 28) * -1;
            M16(dst, 38) = (u16)((s16)M16(dst, 38) + 0x8000);
        }
        M16(dst, 0x60) = 0x10;
        {
            s8 idx = (s8)MU8(sub, 52);
            u32 tbl = 0x06097D88 + ((s32)idx << 8);
            M32(dst, 8) = MU32(tbl, 0);
            M32(dst, 12) = MU32(tbl, 4);
            M32(dst, 16) = MU32(tbl, 8);
            M16(dst, 0x30) = 0x2B1;
            M32(dst, 0x68) = MU32(DH_ACTOR_SLOT_MASKS_ADDR + ((s32)idx << 2), 0);
            MU8(dst, 52) = (u8)idx;
        }
        M16(dst, 0x62) = 0;
        M32(dst, 0x64) = 0x1999;
    }

    M32(arg0, 0) = 0;
}
