/* effect_tick_depth_clip [0x0223875c]: Seeds pose from owner frame and drives depth/clip sprite phases. */
#include "dh.h"
/* 0x0223875c - animated-object depth/clip state machine on record `arg0`.
 * obj   = *(u32 *)(arg0 + 8)   (guest address of the object block)
 * state = (s8)*(u8 *)(arg0 + 16)
 * state 0: seed obj+0x30/+0x32 (s16 pair = 1,0); index (s8)*(u8 *)(sub+52) << 8
 *   selects a triple at 0x06097d68 + idx, copied to obj+8/+12/+16;
 *   obj+0x40 = 0x60000, obj+0x44 = 4, state++.
 * state 1 (fallthrough from 0): obj+0x40 += 0x10000; obj+0x44 -= 1; ret = 1;
 *   if obj+0x44 == 0 then state++; tail-call func_022153f6(obj, obj+0x40, 1)
 *   (braf, epilogue in the delay slot).
 * state 2: *(s32 *)arg0 = ret (0 unless reached via fallthrough). */
extern void sprite_draw_child(void *obj, s32 scale, s32 flags);

void effect_tick_depth_clip(u32 arg0) {
    u32 obj;
    s32 ret = 0;

    obj = MU32(arg0, 8);
    switch ((s8)MU8(arg0, 16)) {
    case 0:
        M16(obj, 0x30) = 1;
        M16(obj, 0x32) = 0;
        {
            u32 sub = MU32(MU32(arg0, 20), 8);
            u32 tbl = 0x06097D68 + ((s32)(s8)MU8(sub, 52) << 8);
            M32(obj, 8)  = MU32(tbl, 0);
            M32(obj, 12) = MU32(tbl, 4);
            M32(obj, 16) = MU32(tbl, 8);
        }
        M32(obj, 0x40) = 0x60000;
        M32(obj, 0x44) = 4;
        MU8(arg0, 16) = (u8)((s8)MU8(arg0, 16) + 1);
        /* fallthrough */
    case 1:
        M32(obj, 0x40) = M32(obj, 0x40) + 0x10000;
        M32(obj, 0x44) = M32(obj, 0x44) - 1;
        ret = 1;
        if (M32(obj, 0x44) == 0) {
            MU8(arg0, 16) = (u8)((s8)MU8(arg0, 16) + 1);
        }
        sprite_draw_child((void *)obj, M32(obj, 0x40), ret);
        return;
    case 2:
        M32(arg0, 0) = ret;
        return;
    default:
        return;
    }
}
