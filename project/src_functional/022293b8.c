#include "dh_work.h"
#include "dh.h"
/* 0x02293b8 (actor_hit_state_enter) - per-actor hit/grab state entry (task=r4, entity=r5). Bails
 * if busy (unk4B != 0). Clears unk52, then switches on the state selector unk88 to arm the matching
 * entry animation/request. Cases 0x46/0x48 run animation_update(task, entity) as a tail.
 * fidelity: HIGH - bsrf/braf tails resolved via braf_resolve: func_0223a6dc (0x23a6dc,
 * actor_pose_load), actor_anim_frame_update (0x22dc2c, animation_update), func_0222adaa (0x22adaa,
 * actor_pose_dispatch). unk4B/43/41/44/4C/67/34/4A byte, unk52/5C word, unk88/84/8C/C4 long;
 * g_06096C74 is the per-slot restore-anim id table (s32 loaded, 16-bit stored). */
extern void actor_load_pose_animation(void *entity);             /* actor_pose_load     */
extern void actor_dispatch_pose_and_type(void *entity);             /* actor_pose_dispatch */
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update    */
extern s32  g_06096C74[];    /* per-slot restore-anim id */

void actor_hit_state_enter(void *task, void *entity) {
    if (ACTOR_M8(entity, paired_slot) != 0)
        return;
    ACTOR_M16(entity, interaction_latch) = 0;
    switch (ACTOR_WORK(entity)->action_state) {
    case 69:
        ACTOR_WORK(entity)->action_category = 0x1000;
        M32(entity, 0xC4) = 12;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_TIMED_RECOVERY;
        break;
    case 70:
    case 72:
        if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
            actor_load_pose_animation(entity);
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
        } else {
            ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[ACTOR_WORK(entity)->slot_id];
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(entity)->action_category = 1;
            ACTOR_WORK(entity)->action_state = 0;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
        }
        actor_anim_frame_update(task, entity);
        break;
    case 71:
        ACTOR_WORK(entity)->facing_right = (s8)(ACTOR_WORK(entity)->facing_right ^ 1);
        ACTOR_M8(entity, hit_pending) = M8(entity, 0x67);
        actor_anim_frame_update(task, entity);
        ACTOR_WORK(entity)->action_category = 0x40000;
        ACTOR_WORK(entity)->state_handler = 26;
        break;
    case 73:
        ACTOR_M8(entity, hit_source_slot) = ACTOR_WORK(entity)->slot_id;
        actor_dispatch_pose_and_type(entity);
        break;
    default:
        break;
    }
}
