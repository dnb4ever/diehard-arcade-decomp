/* effect_spawn_three_table_children [0x02238a9c]: Three entries from 0608c5b4/5cc seed child velocity and model records. */
#include "dh.h"
/* 0x02238a9c - spawn 3 sub-objects via func_0220ff28 and fill each from a
 * stack-local table copied from 0x0608c5b4 (3x 8-byte entries at sp+0xC) and
 * 0x0608c5cc (3x s16 at sp+4). sub = *(*(arg0+20)+8).
 * Each iteration: r0 = func_0220ff28(*(u32*)0x06099580, 0x060419dc);
 * dst = *(u32*)(r0+8);
 *   dst+20 = *(s32*)elem; dst+24 = *(s32*)(elem+4);
 *   dst+38 (s16) = 0xC000; dst+40 (s16) = local s16;
 *   if (*(s8*)(sub+68)) { dst+20 = -dst+20; dst+38 += 0x8000; dst+40 = -dst+40; }
 *   dst+0x60 (s16) = 16;
 *   idx = *(s8*)(sub+52); triple at 0x06097d88 + (idx<<8) -> dst+8/12/16;
 *   dst+0x30 (s16) = 0x2b1; dst+0x68 = *(u32*)(0x0604ebe4 + idx*4);
 *   dst+0x34 = (s8)idx; dst+0x62 (s16) = 0; dst+0x64 = 0x1999.
 * Finally *(s32*)arg0 = 0.
 * fidelity: HIGH - verified vs disassembly; bsrf r2 (0xfffd741a) resolves to
 * 0x0220ff28 with r5 = HWRAM mova 0x0603cb04 + 0x4ed8 = 0x060419dc (second argument). */
extern u32 task_spawn_list_work7c(u32 arg0, u32 arg1);  /* 0x0220ff28 */

void effect_spawn_three_table_children(u32 arg0)
{
    s32 tbl[6];  /* 3x 8-byte entries copied from 0x0608c5b4 */
    s16 vals[3]; /* 3x s16 copied from 0x0608c5cc */
    u32 sub = MU32(MU32(arg0, 20), 8);
    s32 i;

    vals[0] = (s16)MU16(0x0608C5CC, 0);
    vals[1] = (s16)MU16(0x0608C5CC, 2);
    vals[2] = (s16)MU16(0x0608C5CC, 4);
    tbl[0] = M32(0x0608C5B4, 0);
    tbl[1] = M32(0x0608C5B4, 4);
    tbl[2] = M32(0x0608C5B4, 8);
    tbl[3] = M32(0x0608C5B4, 12);
    tbl[4] = M32(0x0608C5B4, 16);
    tbl[5] = M32(0x0608C5B4, 20);

    for (i = 0; i < 3; i++) {
        u32 elem0 = (u32)(s32)tbl[i * 2 + 0];
        u32 elem1 = (u32)(s32)tbl[i * 2 + 1];
        u32 r0 = task_spawn_list_work7c(MU32(0x06099580, 0), 0x060419DC);
        u32 dst = MU32(r0, 8);
        s32 idx;

        M32(dst, 20) = (s32)elem0;
        M32(dst, 24) = (s32)elem1;
        M16(dst, 38) = (s16)(s16)0xC000;
        M16(dst, 40) = (s16)vals[i];
        if ((s8)MU8(sub, 68) != 0) {
            M32(dst, 20) = -(s32)M32(dst, 20);
            M16(dst, 38) = (s16)((s32)(s16)MU16(dst, 38) + 0x8000);
            M16(dst, 40) = (s16)-(s16)MU16(dst, 40);
        }
        M16(dst, 96) = 16;
        idx = (s32)(s8)MU8(sub, 52);
        {
            u32 t = 0x06097D88 + ((u32)idx << 8);
            M32(dst, 8) = M32(t, 0);
            M32(dst, 12) = M32(t, 4);
            M32(dst, 16) = M32(t, 8);
        }
        M16(dst, 48) = (s16)0x2B1;
        M32(dst, 104) = M32(DH_ACTOR_SLOT_MASKS_ADDR + (u32)idx * 4, 0);
        M8(dst, 52) = (s8)idx;
        M16(dst, 98) = 0;
        M32(dst, 100) = 0x1999;
    }

    M32(arg0, 0) = 0;
}
