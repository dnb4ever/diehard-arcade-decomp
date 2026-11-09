/* actor_begin_effect_action_one [0x0223d0fe]: Allocates fixed effect action one and swaps the secondary animation channel. */
#include "dh.h"
/* 0x0223d0fe - begin the fixed action #1 state on an actor. Returns 0 on
 * success, -1 on failure. arg0 = actor.
 * Specialized sibling of func_0223cfe4 with the action hardcoded to 1 and
 * without the func_0223b254 / unk34<3 tail.
 * Fails (-1) if func_0223e4a0() == 0 or if func_0223cbb0(1,0,0) == NULL.
 * On success: oent = obj->unk8; arg0->unk8C |= 2; arg0->unk6D = 1;
 *   arg0->unk6F = oent->unk34. Swap anim field .unkE (g_06096248 stride 32):
 *   save current g_06096248[unk34].unkE into g_060e11ea[unk34] (stride 4), then
 *   load g_060e10a8[1*8 + (u16)arg0->unk68*2] into g_06096248[unk34].unkE.
 * bsrf resolved: 0x23e4a0; bsr func_0223cbb0. fidelity: HIGH. */
extern u8   g_06096248[];
extern u8   g_060e11ea[];
extern u8   g_060e10a8[];
extern s32  effect_pool_make_room(void);
extern void *effect_pool_spawn(s32 a, s32 b, s32 c);

s32 actor_begin_effect_action_one(void *arg0) {
    void *obj;
    void *oent;
    void *anim;

    if (effect_pool_make_room() == 0)
        return -1;
    obj = effect_pool_spawn(1, 0, 0);
    if (obj == 0)
        return -1;
    oent = MP(obj, 8);

    M32(arg0, 0x8C) |= 2;
    M8(arg0, 0x6D) = 1;
    M8(arg0, 0x6F) = M8(oent, 0x34);

    anim = &g_06096248[M8(arg0, 0x34) * 32];
    M16(&g_060e11ea[M8(arg0, 0x34) * 4], 0) = M16(anim, 14);
    M16(anim, 14) = M16(&g_060e10a8[1 * 8 + (u16)MU16(arg0, 0x68) * 2], 0);
    return 0;
}
