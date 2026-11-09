/* enemy_find_latched_effect_target [0x02245d1c]: Returns the linked effect target when slot, state, mask and XZ range tests pass. */
#include "dh.h"
/* 0x02245d1c  try to strike the actor currently latched in slot arg0->unk12C.
 * idx = arg0->unk12C. Three parallel global arrays indexed by idx*4:
 *   g_060E0F0C[idx] = target's collision-type id (0 -> no target)
 *   g_060E0F2C[idx] = 1 when the slot is active
 *   g_060E0F4C[idx] = pointer to the target wrapper (->unk8 = target object)
 * Fire only when: !(arg0->unk8C & 0x3FFFF), id != 0, slot active,
 * wrapper->unk10 == 1, and arg0->unkFC collides with g_0604EBE4[id].
 * Then point_in_box(arg0, target, 0x8000, 0x8000): on hit return target, else 0.
 * fidelity: HIGH - masks/fields verified vs asm; point_in_box = func_0224492c. */
extern s32 g_060E0F0C[];     /* per-slot collision-type id */
extern s32 g_060E0F2C[];     /* per-slot active flag */
extern void *g_060E0F4C[];   /* per-slot target wrapper pointer */
extern u32 g_actor_slot_masks_legacy[];     /* type id -> collision bitmask */
s32 actor_test_target_rectangle(void *box_owner, void *point, s32 hx, s32 hz); /* point_in_box 0x0224492c */

void *enemy_find_latched_effect_target(void *arg0) {
    s32 idx = M32(arg0, 0x12C);
    s32 id = g_060E0F0C[idx];
    if (!(M32(arg0, 0x8C) & 0x3FFFF) && id != 0 &&
        g_060E0F2C[idx] == 1 && M32(g_060E0F4C[idx], 0x10) == 1 &&
        (MU32(arg0, 0xFC) & g_actor_slot_masks_legacy[id])) {
        void *target = MP(g_060E0F4C[idx], 8);
        if (actor_test_target_rectangle(arg0, target, 0x8000, 0x8000) != 0)
            return target;
    }
    return 0;
}
