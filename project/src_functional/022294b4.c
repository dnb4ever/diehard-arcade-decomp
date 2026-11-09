/* player_tick_hit_grab_body [0x022294b4]: Shared hit/grab continuation checks busy, health, facing and action states 0x45..49. */
#include "dh_work.h"
#include "dh.h"
/* 0x02294b4 (actor hit/grab state step, variant) - per-actor state handler dispatched by the actor
 * system (entity = r5, ctx = r4). Bails if busy (unk4C != 0), clears unk52, then:
 *  - if no hit timer is pending (unk56 == 0): for state unk88 == 0x49 flip facing unk26 += 0x8000,
 *    latch unk4A = unk34, run actor_pose_dispatch(entity) and re-clear unk52; otherwise request
 *    unk43 = 0x1F and set unk84 = 0x20000000. Either way tail into func_02216110(entity).
 *  - otherwise switch on unk88:
 *      0x45/0x46: restore anim unk5C = g_06096C74[unk34], unk41 = 1, actor_facing_resolve; if
 *                 unk8C & 0x02000000 run actor_pose_load and request unk43 = 0xD, else reset to
 *                 idle (unk41=1, unk84=1, unk88=0, unk43=1); tail animation_update(ctx, entity).
 *      0x47:      flip unk44 ^= 1, unk4B = unk67, animation_update, unk84 = 0x20000, spawn a
 *                 partner/effect actor via func_0220ff72(g_06099580, template@0x02226EB8), link
 *                 spawned->unk8->unk10 = entity and ->unk14 = g_060995B0[unk4B]->unk8; unk43=0x19.
 *      0x48:      unk84 = 0x1000, unkC4 = 0x20, unk43 = 0xA.
 *      0x49:      unk26 += 0x8000, unk4A = unk34, tail actor_pose_dispatch(entity).
 * fidelity: HIGH - widths from the loads/stores (unk43/41/44/4A/4B/34/4C/67 byte, unk26/52/56/5C
 * word, unk88/84/8C/C4 long); bsrf/braf targets resolved (0x227704, 0x23a6dc, 0x22adaa, 0x22dc2c,
 * 0x220ff72, 0x216110); spawn template @0x02226EB8 resolved via mova 0x2295dc + 0xffffd8dc;
 * differs from sibling 0x0229488 in that it does not stamp unk43 = 0x1B up front and re-clears
 * unk52 after actor_pose_dispatch in the unk56 == 0 / unk88 == 0x49 path. */
extern void *task_spawn_child_work40(s32 ctx, void *template);  /* spawn/alloc object */
extern void audio_queue_actor_slot_cue(void *entity);
extern void actor_resolve_facing(void *entity);             /* actor_facing_resolve */
extern void actor_dispatch_pose_and_type(void *entity);             /* actor_pose_dispatch */
extern void actor_load_pose_animation(void *entity);             /* actor_pose_load */
extern void actor_anim_frame_update(void *ctx, void *entity); /* animation_update */
extern s32  g_06099580;      /* spawn pool/list context */
extern s32  g_06096C74[];    /* per-slot restore-anim id */
extern void *g_actor_slot_tasks[];   /* per-slot object pointers */
extern u8   g_02226EB8[];    /* partner/effect spawn template (in-ROM) */

void player_tick_hit_grab_body(void *ctx, void *entity) {
    void *spawned;

    if (ACTOR_M8(entity, hit_pending) != 0)
        return;
    ACTOR_M16(entity, interaction_latch) = 0;
    if (ACTOR_M16(entity, health) == 0) {
        if (ACTOR_WORK(entity)->action_state == 0x49) {
            ACTOR_WORK(entity)->angle_y = (s16)(ACTOR_WORK(entity)->angle_y + 0x8000);
            ACTOR_M8(entity, hit_source_slot) = ACTOR_WORK(entity)->slot_id;
            actor_dispatch_pose_and_type(entity);
            ACTOR_M16(entity, interaction_latch) = 0;
        } else {
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_DEFEATED;
            ACTOR_WORK(entity)->action_category = 0x20000000;
        }
        audio_queue_actor_slot_cue(entity);
        return;
    }
    switch (ACTOR_WORK(entity)->action_state) {
    case 0x45:
    case 0x46:
        ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[ACTOR_WORK(entity)->slot_id];
        ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
        actor_resolve_facing(entity);
        if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
            actor_load_pose_animation(entity);
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
        } else {
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(entity)->action_category = 1;
            ACTOR_WORK(entity)->action_state = 0;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
        }
        actor_anim_frame_update(ctx, entity);
        return;
    case 0x47:
        ACTOR_WORK(entity)->facing_right = (s8)(ACTOR_WORK(entity)->facing_right ^ 1);
        ACTOR_M8(entity, paired_slot) = M8(entity, 0x67);
        actor_anim_frame_update(ctx, entity);
        ACTOR_WORK(entity)->action_category = 0x20000;
        spawned = task_spawn_child_work40(g_06099580, g_02226EB8);
        MP(MP(spawned, 8), 0x10) = entity;
        MP(MP(spawned, 8), 0x14) = TASK_PTR(g_actor_slot_tasks[ACTOR_M8(entity, paired_slot)], work);
        ACTOR_WORK(entity)->state_handler = 0x19;
        return;
    case 0x48:
        ACTOR_WORK(entity)->action_category = 0x1000;
        M32(entity, 0xC4) = 0x20;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_TIMED_RECOVERY;
        return;
    case 0x49:
        ACTOR_WORK(entity)->angle_y = (s16)(ACTOR_WORK(entity)->angle_y + 0x8000);
        ACTOR_M8(entity, hit_source_slot) = ACTOR_WORK(entity)->slot_id;
        actor_dispatch_pose_and_type(entity);
        return;
    }
}
