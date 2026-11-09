/* effect_spawn_owner_variant_a [0x022387fc]: Spawns 06041558 child and seeds pose/parameters from owner and tables. */
#include "dh.h"
/* 0x022387fc - initialize a child object obtained via func_0220ff28.
 * sub = M32(M32(arg0,20),8);
 * node = func_0220ff28(*(u32 *)0x06099580, 0x06041558);
 * obj = M32(node,8) is then filled in from sub and fixed tables. */
extern u32 task_spawn_list_work7c(u32 list_obj, u32 def); /* 0x0220ff28 */

void effect_spawn_owner_variant_a(u32 arg0)
{
    u32 sub = MU32(MU32(arg0, 20), 8);
    u32 node = task_spawn_list_work7c(MU32(0x06099580, 0), 0x06041558);
    u32 obj = MU32(node, 8);
    s32 dir;

    if ((s8)MU8(sub, 68) != 0)
        dir = (s32)(s16)0xC800; /* -0x3800 */
    else
        dir = 0x3800;
    M32(obj, 64) = dir;
    M32(obj, 112) = 0x1600;
    M16(obj, 96) = 0xF;
    {
        u32 tbl = 0x06097DA8 + ((s32)(s8)MU8(sub, 52) << 8);
        M32(obj, 8) = M32(tbl, 0);
        M32(obj, 12) = M32(tbl, 4);
        M32(obj, 16) = M32(tbl, 8);
    }
    M16(obj, 48) = 0x3B;
    M32(obj, 104) = MU32(DH_ACTOR_SLOT_MASKS_ADDR + ((s32)(s8)MU8(sub, 52) << 2), 0);
    MU8(obj, 52) = MU8(sub, 52);
    M16(obj, 98) = 0x41;
    M32(obj, 100) = 0x1999;
    M32(arg0, 0) = 0;
}
