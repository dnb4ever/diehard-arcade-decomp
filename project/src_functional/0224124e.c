/* actor_select_directional_animation [0x0224124e]: Selects requested animation through action, variant and facing-angle tables. */
#include "dh.h"
/* 0x0224124e - resolve an actor's facing sprite into obj->0x5C via a 3-level
 * pointer table. This is the shared tail of func_022402c0, called as
 * func_0224124e(obj, out, 0).
 * Args: obj=r4, out=r5 (s32*, read not written here), arg=r6 (table index).
 *   func_022416c4(obj);                              // actor_facing_clamp
 *   base = (obj->0x38 & 0x400) ? 0x0608D854 : 0x0608D824;
 *   sel  = *(s32*)(base + (u8)obj->0x49 * 4);
 *   q60  = ((u16)obj->0x60 + 0x6000) >> 14;        // 14x shar
 *   q62  = ((u16)obj->0x62 + (obj->0x44 ? 0x9000 : 0x1000)) >> 11;  // 11x shar
 *   idx  = ((u16)q62 & 0x1C) + ((u16)q60 & 3);
 *   pA   = ((void**)obj->0x164)[*out];
 *   pB   = ((void**)pA)[arg];
 *   pC   = ((void**)pB)[sel];
 *   obj->0x5C = *(s16*)((u8*)pC + idx*2);
 * fidelity: HIGH - shar counts verified (14 / 11); all shifted values are
 * non-negative so arithmetic==logical; only register-form tst (no mask trap). */
void actor_clamp_target_heading(void *a);   /* extern (actor_facing_clamp) */

void actor_select_directional_animation(void *obj, s32 *out, s32 arg)
{
    uintptr_t base;
    s32 sel, q60, q62, idx, dir;
    void *pA, *pB, *pC;

    actor_clamp_target_heading(obj);

    base = (M32(obj, 0x38) & 0x400) ? 0x0608D854 : 0x0608D824;
    sel  = *(s32 *)(uintptr_t)(base + (s32)MU8(obj, 0x49) * 4);

    q60 = ((s32)(u16)M16(obj, 0x60) + 0x6000) >> 14;
    dir = M8(obj, 0x44) ? 0x9000 : 0x1000;
    q62 = ((s32)(u16)M16(obj, 0x62) + dir) >> 11;
    idx = ((u16)q62 & 0x1C) + ((u16)q60 & 3);

    pA = ((void **)MP(obj, 0x164))[*out];
    pB = ((void **)pA)[arg];
    pC = ((void **)pB)[sel];
    M16(obj, 0x5C) = *(s16 *)((u8 *)pC + idx * 2);
}
