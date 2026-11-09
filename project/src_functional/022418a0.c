/* actor_deactivate_if_active_source_alias [0x022418a0]: Existing map corroborated by source: if (actor+0x8c)&0x3ffff: func_0223a9d0(actor,3); actor+0x41=0(byte); tail func_0245c88(actor); else return */
#include "dh_band21.h"

/* 0x022418a0 - if any of actor+0x8c's low 18 bits are set, run the teardown
 * helper func_0223a9d0(actor, 3), clear the state byte actor+0x45 (mov #69 = 0x45),
 * and tail-call func_02245c88(actor); otherwise return immediately.
 * fidelity: HIGH - tst (actor+0x8c) & 0x0003ffff gate; bsrf callee gets r4=actor,
 *   r5=3; byte store actor+0x45 = 0; braf tail call (actor) with r14 pop in its slot. */
void actor_commit_effect_handoffs(u8 *actor, s32 mode);
void enemy_load_action_descriptor(u8 *actor);

void actor_deactivate_if_active_source_alias(u8 *actor) {
    if ((*(s32 *)(actor + 0x8c) & 0x0003ffff) == 0)
        return;
    actor_commit_effect_handoffs(actor, 3);
    *(actor + 0x45) = 0;
    enemy_load_action_descriptor(actor);   /* tail call */
}
