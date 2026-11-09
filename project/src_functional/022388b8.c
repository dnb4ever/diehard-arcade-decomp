/* effect_spawn_owner_variant_b [0x022388b8]: Spawns child, facing selects signed angle and seeds rate/pose fields. */
#include "dh.h"
/* 0x022388b8 - spawn/initialise a sub-object via func_0220ff28, then fill it.
 * sub  = *(u32 *)(*(u32 *)(arg0 + 20) + 8)   (owner/descriptor block)
 * call func_0220ff28(*(u32 *)0x06099580); result pointer -> dst = *(u32 *)(ret + 8)
 * dst+0x40 = (s8)*(u8 *)(sub + 68) ? (s16)0xe000 sign-ext : 0x2000
 * dst+0x70 = 0x200; dst+0x60 = 5;
 * idx = (s8)*(u8 *)(sub + 52); triple at 0x06097da8 + (idx << 8)
 *   copied to dst+8/+12/+16; dst+0x30 = 0x3b;
 * dst+0x68 = *(u32 *)(0x0604ebe4 + idx*4); dst+0x34 = (u8)idx;
 * dst+0x62 = 0x41; dst+0x64 = 0x1999; finally *(s32 *)arg0 = 0. */
extern u32 task_spawn_list_work7c(u32 arg);  /* 0x0220ff28 */

void effect_spawn_owner_variant_b(u32 arg0) {
    u32 sub = MU32(MU32(arg0, 20), 8);
    u32 r0 = task_spawn_list_work7c(MU32(0x06099580, 0));
    u32 dst = MU32(r0, 8);
    s32 v;

    if ((s8)MU8(sub, 68) != 0) {
        v = (s32)(s16)0xe000;
    } else {
        v = 0x2000;
    }
    M32(dst, 0x40) = v;
    M32(dst, 0x70) = 0x200;
    M16(dst, 0x60) = 5;
    {
        u32 tbl = 0x06097DA8 + ((s32)(s8)MU8(sub, 52) << 8);
        M32(dst, 8) = MU32(tbl, 0);
        M32(dst, 12) = MU32(tbl, 4);
        M32(dst, 16) = MU32(tbl, 8);
    }
    M16(dst, 0x30) = 0x3b;
    M32(dst, 0x68) = MU32(DH_ACTOR_SLOT_MASKS_ADDR + ((s32)(s8)MU8(sub, 52) << 2), 0);
    MU8(dst, 52) = MU8(sub, 52);
    M16(dst, 0x62) = 0x41;
    M32(dst, 0x64) = 0x1999;
    M32(arg0, 0) = 0;
}
