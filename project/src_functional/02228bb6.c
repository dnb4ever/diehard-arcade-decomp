/* player_tick_restore_idle [0x02228bb6]: Hit response or loop restores special/idle animation and facing. */
#include "dh_work.h"
#include "dh.h"
/* 0x0228bb6 (func_02228bb6) - actor state handler (task=r4, entity=r5), one of the g_06091100
 * per-state table entries. Resolves the current animation request then tails animation_update.
 *   if (entity->unk48 != 0 && (entity->unkBA & 0x40)): func_0222bd54(entity); request unk43 = 9.
 *   else if (entity->unk54 & 2):
 *       if (entity->unk8C & 0x02000000): func_0223a6dc(entity) (pose_load),
 *           func_02227704(entity) (facing_resolve); request unk43 = 13.
 *       else: func_02227704(entity); unk84 = 1; unk88 = 0; request unk43 = 1; clear unk48;
 *             restore anim unk5C = g_06096C74[unk34].
 *   always: animation_update(task, entity).
 * fidelity: HIGH - bsrf/braf resolved: func_0222bd54 (0x22bd54), func_0223a6dc (0x23a6dc),
 * func_02227704 (0x227704), animation_update actor_anim_frame_update (0x22dc2c). unk48/43/34 byte,
 * unk54/5C/BA word, unk84/88/8C long; g_06096C74 s32[] (16-bit stored). */
extern void actor_enter_pose_mode(void *entity);
extern void actor_load_pose_animation(void *entity);             /* actor_pose_load     */
extern void actor_resolve_facing(void *entity);             /* actor_facing_resolve */
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update    */
extern s32  g_06096C74[];

void player_tick_restore_idle(void *task, void *entity) {
    if (ACTOR_M8(entity, action_pending) != 0 && (ACTOR_MU16(entity, type_flags) & 0x40)) {
        actor_enter_pose_mode(entity);
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_RESPONSE;
    } else if (ACTOR_U16(entity, animation_events) & 2) {
        if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
            actor_load_pose_animation(entity);
            actor_resolve_facing(entity);
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
        } else {
            actor_resolve_facing(entity);
            ACTOR_WORK(entity)->action_category = 1;
            ACTOR_WORK(entity)->action_state = 0;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
            ACTOR_M8(entity, action_pending) = 0;
            ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[(s8)ACTOR_WORK(entity)->slot_id];
        }
    }
    actor_anim_frame_update(task, entity);
}
