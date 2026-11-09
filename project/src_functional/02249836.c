#include "dh_work.h"
/* enemy_begin_combo [0x02249836]: Enters combo state thirteen and chooses its initial branch and linked-effect action. */
#include "dh.h"
/* 0x02249836  actor_combo_begin - combo-machine entry/init helper.
 * A distinct real entry (its own prologue/epilogue, ABI r4=entity, r5=arg1) that sits
 * inside the byte span the 0x2492f8 deferral note lumped into the "14-state machine";
 * it is NOT reached from that dispatch (no fall-through across the dispatcher's rts,
 * no in-function pointer to it). It seeds the combo machine: force state-d4=13, mask
 * entity[0xe0], then pick the start state from a RNG (func_02210020) gated on
 * entity[0x38]==30. The common path hands off to func_02239ce8(entity, arg1,
 * entity[0x12c]) - same callee the 0x249898 grab-lock handler uses as
 * func_02239ce8(entity,&task,index), so arg1 is an opaque pointer and entity[0x12c]
 * the index. Both callees resolved via braf_resolve. fidelity: HIGH - hand-traced. */
extern s32  game_random_next(void);
extern void actor_select_effect_action(void *entity, void *arg1, s32 index);

void enemy_begin_combo(void *entity, void *arg1) {
    ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_COMBO;
    ACTOR_M32(entity, state_flags) &= 0x1300;
    if ((game_random_next() & 1) != 0 && ACTOR_M32(entity, render_flags) == 30) {
        ACTOR_M32(entity, behavior_phase) = 0;
        ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_MU16(entity, animation_requested) = DH_ANIM_COMBO_START;
    } else {
        ACTOR_M32(entity, behavior_phase) = 1;
        ACTOR_M32(entity, action_phase) = 5;
        actor_select_effect_action(entity, arg1, ACTOR_M32(entity, effect_target_slot));
    }
    ACTOR_M32(entity, action_category) = 1;
}
