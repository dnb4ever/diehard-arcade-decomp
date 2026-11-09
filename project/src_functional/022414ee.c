#include "dh_work.h"
/* actor_enter_lateral_reaction [0x022414ee]: Sets reaction flags, lateral velocity and the direction-selected animation. */
#include "dh.h"
/* 0x022414ee - leaf facing-init (sibling of the func_0224124e family). No calls.
 * Args: obj=r4, out=r5 (s32*).
 *   obj->0xBC  = -1;
 *   obj->0x108 = 1;
 *   obj->0x88  = 8;
 *   obj->0x84  = 0x1000;
 *   obj->0xE0 |= 0x08000000;
 *   *out = 9;
 *   obj->0xEC  = ((u16)obj->0x62 > 0x8000) ? 0x8000 : -0x8000;   // 32-bit
 *   bit = ((u16)obj->0x62 >> 15) & 1;      // top bit of the facing angle
 *   if (obj->0x44) bit ^= 1;                // asm applies ~ then & 1
 *   obj->0x5C = *(s16*)(0x0608672C + bit*2);
 * fidelity: HIGH - 0xEC store is 32-bit (0x00008000 vs sign-extended
 * 0xFFFF8000); the >>15 is built from shlr8 + 3*shlr2 + shlr1; register-form
 * tst only (no mask trap). */
void actor_enter_lateral_reaction(void *obj, s32 *out)
{
    s32 bit;

    M32(obj, 0xBC)  = -1;
    ACTOR_M32(obj, action_phase) = 1;
    ACTOR_M32(obj, action_state)  = 8;
    ACTOR_M32(obj, action_category)  = 0x1000;
    ACTOR_M32(obj, state_flags) |= 0x08000000;
    *out = 9;

    M32(obj, 0xEC) = ((u16)ACTOR_M16(obj, motion_heading) > 0x8000) ? 0x8000 : -0x8000;

    bit = ((u16)ACTOR_M16(obj, motion_heading) >> 15) & 1;
    if (ACTOR_M8(obj, facing_right))
        bit = (~bit) & 1;
    ACTOR_M16(obj, animation_requested) = *(s16 *)(uintptr_t)(0x0608672C + bit * 2);
}
