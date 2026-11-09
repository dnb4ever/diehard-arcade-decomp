/* enemy_choose_dash_recovery [0x02247924]: Compares target facing and chooses turn-around, continuation or timed recovery. */
#include "dh_work.h"
#include "dh.h"
/* 0x02247924  dash move: decide turn-around vs. continue. actor = arg0; the chase
 * target is g_060995b0[actor->unk120]->unk8. Compute the facing needed to reach it
 * (0 if target is to the right, else 1). If the actor already faces that way
 * (== unk44), commit to the recover state (unkD4=5, unkD8=0, unk84=1, unkE0 flag
 * 0x100000, recover timer unkC4 = g_06090118[unkE8]) and snap heading to unk148
 * when it differs. Otherwise re-pick a move via func_022460c8:
 *   0 turn: actor_set_state_7, heading = table[0] (actor->unk160).
 *   1 feint: actor_set_state_7, heading = table[rng & 3], set flag 0x100000.
 *   2 -> func_02247a16.  3 -> func_02247a3e.
 *
 * bounds: [0x02247924, 0x247a16); split out of func_022473ee's over-swallowed
 *   extent. notes: func_02210020 takes no args. fidelity: HIGH - vs fn.py --asm. */
extern void *g_actor_slot_tasks_legacy[];     /* per-slot anchor pointers; deref +8 -> actor */
extern s32  g_06090118[];      /* recover-duration table (by unkE8) */

s32  game_random_next(void);                        /* 0x02210020 rng */
s32  enemy_classify_linked_effect(void *actor);                 /* 0x022460c8 move-mode select */
void actor_set_state_7(void *actor);                 /* 0x0224840a actor_set_state_7 */
void enemy_enter_dash_recovery_a(void *actor);                 /* 0x02247a16 */
void enemy_enter_dash_recovery_b(void *actor);                 /* 0x02247a3e */

void enemy_choose_dash_recovery(void *actor) {
    void *target = TASK_PTR(g_actor_slot_tasks_legacy[ACTOR_M32(actor, target_slot)], work);
    s16 *table;
    s32 face;

    face = (M32(target, 0x08) - ACTOR_M32(actor, position_x)) > 0 ? 0 : 1;
    if (face == ACTOR_M8(actor, facing_right)) {
        ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_RECOVERY;
        ACTOR_M32(actor, behavior_phase) = 0;
        ACTOR_M32(actor, action_category) = 1;
        ACTOR_M32(actor, action_state) = 0;
        ACTOR_M32(actor, state_flags) = 0x100000;
        M32(actor, 0xC4) = g_06090118[M32(actor, 0xE8)];
        if (ACTOR_M16(actor, animation_current) != ACTOR_M16(actor, idle_animation)) {
            ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_M16(actor, animation_requested) = ACTOR_M16(actor, idle_animation);
        }
        return;
    }

    table = (s16 *)MP(actor, 0x160);
    switch (enemy_classify_linked_effect(actor)) {
    case 0:
        actor_set_state_7(actor);
        ACTOR_M16(actor, animation_requested) = table[0];
        return;
    case 1:
        actor_set_state_7(actor);
        ACTOR_M16(actor, animation_requested) = table[game_random_next() & 3];
        ACTOR_M32(actor, state_flags) |= 0x100000;
        return;
    case 2:
        enemy_enter_dash_recovery_a(actor);
        return;
    case 3:
        enemy_enter_dash_recovery_b(actor);
        return;
    }
}
