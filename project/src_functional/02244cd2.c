/* 0x02244cd2  actor_init_grab: initialise an actor's grab/throw state, run the big teardown
 * helper, then tail-call the template-copy init. First real entry of the 0x244cd2 fn-unit
 * (the unit also holds the obj_latch_reset leaf at 0x244d0a).
 *   *(s16*)(actor+0x60) = 0x1000;
 *   *(s16*)(actor+0x62) = *(s16*)(actor+0x26);
 *   *(s32*)(actor+0x74) = 0x800;
 *   *(s16*)(actor+0x64) = 16;
 *   func_0223a9d0(actor, 3);          (bsrf; r5=3)
 *   *(u8*)(actor+0x45) = 0;
 *   func_02245c88(actor);              (braf tail -> 0x245c88)
 * fidelity: HIGH - 2-arg helper ABI pinned this batch (func_0223a9d0(actor, mode)); the
 *   bsrf delta resolves to 0x23a9d0 and the braf tail to 0x245c88; widths verified
 *   (mov.w 0x60/0x62/0x64, mov.l 0x74, mov.b 0x45). */
#include "decomp.h"

void actor_commit_effect_handoffs(u8 *actor, s32 mode);   /* 0x0223a9d0 */
void enemy_load_action_descriptor(u8 *actor);              /* 0x0245c88 */

void actor_init_grab(u8 *actor) {
    *(s16 *)(actor + 0x60) = 0x1000;
    *(s16 *)(actor + 0x62) = *(s16 *)(actor + 0x26);
    *(s32 *)(actor + 0x74) = 0x800;
    *(s16 *)(actor + 0x64) = 16;
    actor_commit_effect_handoffs(actor, 3);
    *(actor + 0x45) = 0;
    enemy_load_action_descriptor(actor);   /* tail */
}
