/* enemy_init_jump_lane [0x02243d78]: Direction flags select signed X offset, clear motion bits and reserve lane slot. */
#include "dh_work.h"
#include "dh.h"
/* 0x02243d78  init a physics/jump record on object p, selected by flags (r6).
 * field[0x130] = (flags & 2) ? 0xFFFE999A : 0x16666; field[0x134] = 0;
 * field[0xe0] &= ~0x60; field[0xe0] |= 0x10; field[0x124] = flags;
 * g_06096ec0[flags] = field[0x34].
 * fidelity: HIGH - exact (-97 = ~0x60; last store is a signed byte). */
extern s8 g_06096ec0[];
void enemy_init_jump_lane(void *p, s32 arg1, s32 flags) {
    (void)arg1;
    if (flags & 2)
        ACTOR_M32(p, target_offset_x) = (s32)0xFFFE999A;
    else
        ACTOR_M32(p, target_offset_x) = 0x16666;
    ACTOR_M32(p, target_offset_z) = 0;
    ACTOR_M32(p, state_flags) &= ~0x60;
    ACTOR_M32(p, state_flags) |= 0x10;
    M32(p, 0x124) = flags;
    g_06096ec0[flags] = ACTOR_M8(p, slot_id);
}
