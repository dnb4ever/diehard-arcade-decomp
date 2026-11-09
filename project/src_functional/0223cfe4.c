#include "dh_work.h"
/* actor_begin_effect_action [0x0223cfe4]: Allocates the requested effect action, records its slot and swaps the secondary animation channel. */
#include "dh.h"
/* 0x0223cfe4 - begin a hit/action state on an actor. Returns 0 on success,
 * -1 on failure.
 * arg0 = actor; action = requested action id (must be nonzero).
 * Fails (returns -1) if action == 0, if func_0223e4a0() == 0 (resource check),
 * or if func_0223cbb0(action, 0, 0) returns NULL.
 * On success: obj = func_0223cbb0(...); oent = obj->unk8.
 *   arg0->unk6D = action; arg0->unk6F = oent->unk34; arg0->unk45 = action;
 *   arg0->unk8C |= g_0604ebe4[action].
 *   Swap anim field .unkE (g_06096248 stride 32): save current
 *   g_06096248[unk34].unkE into g_060e11ea[unk34] (stride 4), then load new value
 *   g_060e10a8[action*8 + (u16)arg0->unk68*2] into g_06096248[unk34].unkE.
 *   func_0223b254(arg0).
 *   If arg0->unk34 < 3: h = (s16)g_0608ce74[arg0->unk6D] (stride 2);
 *     g_06096c74[unk34] (stride 4, long) = h; arg0->unk5C = h; func_022260c8(arg0).
 * fidelity: HIGH. bsrf resolved 0x23e4a0/0x23b254/0x2260c8; func_0223cbb0 call. */
extern s32 g_actor_slot_masks[];
extern u8  g_06096248[];
extern u8  g_060e11ea[];
extern u8  g_060e10a8[];
extern u8  g_0608ce74[];
extern u8  g_06096c74[];
extern s32   effect_pool_make_room(void);
extern void *effect_pool_spawn(s32 a, s32 b, s32 c);
extern void  actor_load_part_offset(void *entity);
extern void  actor_fill_pose_history(void *entity);

s32 actor_begin_effect_action(void *arg0, s32 action) {
    void *obj;
    void *oent;
    void *anim;
    s16 h;

    if (action == 0)
        return -1;
    if (effect_pool_make_room() == 0)
        return -1;
    obj = effect_pool_spawn(action, 0, 0);
    if (obj == 0)
        return -1;
    oent = MP(obj, 8);

    ACTOR_M8(arg0, secondary_effect_type) = (s8)action;
    ACTOR_M8(arg0, secondary_effect_slot) = M8(oent, 0x34);
    M8(arg0, 0x45) = (s8)action;
    ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[action];

    anim = &g_06096248[ACTOR_M8(arg0, slot_id) * 32];
    M16(&g_060e11ea[ACTOR_M8(arg0, slot_id) * 4], 0) = M16(anim, 14);
    M16(anim, 14) = M16(&g_060e10a8[action * 8 + (u16)MU16(arg0, 0x68) * 2], 0);

    actor_load_part_offset(arg0);

    if (ACTOR_M8(arg0, slot_id) < 3) {
        h = M16(&g_0608ce74[ACTOR_M8(arg0, secondary_effect_type) * 2], 0);
        M32(&g_06096c74[ACTOR_M8(arg0, slot_id) * 4], 0) = h;
        ACTOR_M16(arg0, animation_requested) = h;
        actor_fill_pose_history(arg0);
    }
    return 0;
}
