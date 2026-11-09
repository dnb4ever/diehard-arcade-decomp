#include "dh_work.h"
#include "dh.h"
/* 0x0229074 (actor_main_state_step) - the main per-actor state handler (task=r4, entity=r5),
 * dispatched by the actor system. Bails if busy (unk4C != 0). Clears unk52, then selects on the
 * HP/ready word unk56 and the state selector unk88:
 *   unk56 == 0 (drained): a small transition set on unk88 (60/68/73/74/75, else default). Most
 *     run animation_update(task, entity); every path then tail-calls func_02216110(entity).
 *   unk56 != 0 (live): the full state table on unk88 (40/59/60/61/63/64/65/66/67/68/73/74/75/84).
 *     The "flip/attack" cases (40/59/60/63) branch on the grabbed flag unk8C & 0x02000000 to either
 *     func_0223a6dc(entity) (pose_load, request 13) or a reset-to-idle; others set timers/anim ids
 *     and most tail animation_update(task, entity). Cases 61/64/65/68/73 return without it.
 * fidelity: HIGH - animation_update fp resolved via ptr_resolve (0x2290a0 + 0x4b8c = 0x22dc2c);
 * bsrf/braf resolved: func_02216110 (0x216110), func_0222adaa (0x22adaa), func_0222bd90 (0x22bd90),
 * func_0223a6dc (0x23a6dc), func_0222c322 (0x22c322). unk34/41/43/4A/4C/67 byte, unk26/52/56/5C
 * word, unk84/88/8C/C4 long; g_06096C74 s32[] (16-bit stored). */
extern void actor_anim_frame_update(void *task, void *entity);  /* animation_update */
extern void audio_queue_actor_slot_cue(void *entity);
extern void actor_dispatch_pose_and_type(void *entity);              /* actor_pose_dispatch */
extern void player_init_thrown_physics(void *entity);
extern void actor_load_pose_animation(void *entity);              /* actor_pose_load */
extern void actor_fall_setup(void *entity);
extern s32  g_06096C74[];

void actor_main_state_step(void *task, void *entity) {
    s32 sel;

    if (ACTOR_M8(entity, hit_pending) != 0)
        return;
    ACTOR_M16(entity, interaction_latch) = 0;
    sel = ACTOR_WORK(entity)->action_state;

    if (ACTOR_M16(entity, health) == 0) {
        switch (sel) {
        case 60:
            ACTOR_WORK(entity)->animation_requested = 0x13D;
            ACTOR_WORK(entity)->angle_y = (s16)(ACTOR_WORK(entity)->angle_y + 0x8000);
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(entity)->action_category = 0x20000000;
            ACTOR_WORK(entity)->state_handler = 29;
            actor_anim_frame_update(task, entity);
            break;
        case 68:
            ACTOR_M8(entity, hit_pending) = M8(entity, 0x67);
            actor_anim_frame_update(task, entity);
            ACTOR_WORK(entity)->state_handler = 26;
            ACTOR_WORK(entity)->action_state = 0;
            break;
        case 73:
            ACTOR_M8(entity, hit_source_slot) = (s8)ACTOR_WORK(entity)->slot_id;
            actor_dispatch_pose_and_type(entity);
            ACTOR_M16(entity, interaction_latch) = 0;
            break;
        case 74:
            M32(entity, 0xC4) = 32;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_RESPONSE;
            actor_anim_frame_update(task, entity);
            break;
        case 75:
            player_init_thrown_physics(entity);
            actor_anim_frame_update(task, entity);
            break;
        default:
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_DEFEATED;
            ACTOR_WORK(entity)->action_category = 0x20000000;
            break;
        }
        audio_queue_actor_slot_cue(entity);
    } else {
        switch (sel) {
        case 40:
            if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
                actor_load_pose_animation(entity);
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
            } else {
                ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(entity)->action_category = 1;
                ACTOR_WORK(entity)->action_state = 0;
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
            }
            actor_anim_frame_update(task, entity);
            break;
        case 59:
        case 63:
            ACTOR_WORK(entity)->angle_y = (s16)(ACTOR_WORK(entity)->angle_y + 0x8000);
            if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
                actor_load_pose_animation(entity);
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
            } else {
                ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(entity)->action_category = 1;
                ACTOR_WORK(entity)->action_state = 0;
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
            }
            actor_anim_frame_update(task, entity);
            break;
        case 60:
            ACTOR_WORK(entity)->angle_y = (s16)(ACTOR_WORK(entity)->angle_y + 0x8000);
            if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
                actor_load_pose_animation(entity);
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
            } else {
                ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[(s8)ACTOR_WORK(entity)->slot_id];
                ACTOR_WORK(entity)->action_category = 1;
                ACTOR_WORK(entity)->action_state = 0;
                ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
            }
            actor_anim_frame_update(task, entity);
            break;
        case 61:
            ACTOR_WORK(entity)->action_category = 0x1000;
            M32(entity, 0xC4) = 32;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_TIMED_RECOVERY;
            break;
        case 64:
        case 65:
            ACTOR_WORK(entity)->action_category = 0x1000;
            M32(entity, 0xC4) = 12;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_TIMED_RECOVERY;
            break;
        case 66:
            ACTOR_M16(entity, health) = 0;
            ACTOR_WORK(entity)->animation_requested = 0x2EB;
            ACTOR_WORK(entity)->state_handler = 28;
            ACTOR_WORK(entity)->action_category = 0x20000000;
            M32(entity, 0xC4) = 4;
            actor_anim_frame_update(task, entity);
            break;
        case 67:
            ACTOR_M16(entity, health) = 0;
            ACTOR_WORK(entity)->animation_requested = 0x2F0;
            ACTOR_WORK(entity)->state_handler = 28;
            ACTOR_WORK(entity)->action_category = 0x20000000;
            M32(entity, 0xC4) = 4;
            actor_anim_frame_update(task, entity);
            break;
        case 68:
            ACTOR_M8(entity, hit_pending) = M8(entity, 0x67);
            actor_anim_frame_update(task, entity);
            ACTOR_WORK(entity)->state_handler = 26;
            ACTOR_WORK(entity)->action_state = 0;
            break;
        case 73:
            ACTOR_M8(entity, hit_source_slot) = (s8)ACTOR_WORK(entity)->slot_id;
            actor_dispatch_pose_and_type(entity);
            ACTOR_M16(entity, interaction_latch) = 0;
            break;
        case 74:
            M32(entity, 0xC4) = 32;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_RESPONSE;
            actor_anim_frame_update(task, entity);
            break;
        case 75:
            player_init_thrown_physics(entity);
            actor_anim_frame_update(task, entity);
            break;
        case 84:
            actor_fall_setup(entity);
            actor_anim_frame_update(task, entity);
            break;
        default:
            break;
        }
    }
}
