/* actor_substate_init [0x02228536]: Existing map corroborated by source: (member) (re)init of an actor's per-state block on */
#include "dh_work.h"
#include "dh.h"
/* 0x02228536 actor_substate_init(entity) - (re)initialises an actor's per-state block on
 * (re)entering the main step; called by the actor FSM step (0x228508).
 *   func_02227704(entity);
 *   entity->unk84 = 1; entity->unk88 = 0; entity->unk46 = 0; entity->unk47 = 0;
 *   entity->unk38 = 1;
 *   g_0609979C[unk34-1] = 0; g_060997A0[unk34-1] = 0;      (screen pos)
 *   entity->unkC0 = g_0609979A ? ((unk34==1)?2:4) : 6;
 *   entity->unkC4 = 0;
 *   if (unk6D == 0 && (unk8C & 0x02000000)) {
 *       func_0223a6dc(entity); func_02227704(entity); entity->unk43 = 0x0D;
 *   } else {
 *       entity->unk5C = (s16)g_06096C74[unk34]; entity->unk43 = 1;
 *   }
 *   entity->unk48 = 0;
 *   func_02223e70(entity);
 *   func_0221707c(entity, 0x5999, (s16)0xB334, 0x4000, 0x14CCC);
 * fidelity: HIGH - all five callees exist with matching ABI (1-arg, and 0x221707c 5-arg);
 * bsrf targets resolved; mov.w 0xB334 sign-extends to 0xFFFFB334; else-branch unk43 = 1
 * (r4 still holds the #1 from the unk84/unk38 inits). Matches m2c. */
extern void actor_resolve_facing(void *entity);
extern void actor_load_pose_animation(void *entity);
extern void actor_reset_pose_history(void *arg0);
extern void actor_register_movement_slot(void *entity, s32 a1, s32 a2, s32 a3, s32 a4);
extern s16  g_0609979C[];
extern s16  g_060997A0[];
extern s16  g_actor_pose_set;
extern s32  g_06096C74[];

void actor_substate_init(void *entity) {
    s32 slot;

    actor_resolve_facing(entity);
    ACTOR_WORK(entity)->action_category = 1;
    ACTOR_WORK(entity)->action_state = 0;
    M8(entity, 0x46) = 0;
    M8(entity, 0x47) = 0;
    ACTOR_M32(entity, render_flags) = 1;
    slot = ACTOR_WORK(entity)->slot_id - 1;
    g_0609979C[slot] = 0;
    g_060997A0[slot] = 0;
    if (g_actor_pose_set != 0) {
        M32(entity, 0xC0) = (ACTOR_WORK(entity)->slot_id == 1) ? 2 : 4;
    } else {
        M32(entity, 0xC0) = 6;
    }
    M32(entity, 0xC4) = 0;
    if (ACTOR_M8(entity, secondary_effect_type) == 0 && (ACTOR_M32(entity, interaction_flags) & 0x02000000)) {
        actor_load_pose_animation(entity);
        actor_resolve_facing(entity);
        ACTOR_WORK(entity)->state_handler = 0x0D;
    } else {
        ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[ACTOR_WORK(entity)->slot_id];
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
    }
    ACTOR_M8(entity, action_pending) = 0;
    actor_reset_pose_history(entity);
    actor_register_movement_slot(entity, 0x5999, (s16)0xB334, 0x4000, 0x14CCC);
}
