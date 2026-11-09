/* player_tick_input_recovery [0x02228b68]: Hit capability gate refreshes animation; otherwise selects input animation and timestamps slot. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228b68 (actor FSM body; tail target of func_02228b38) - if entity->unk48 is set AND
 * entity->unkBA bit6 (0x40) is set: run func_0222bd54(entity), animation_update(a0,entity),
 * set state 0x43=9 and return. Otherwise run func_0222bdf2(entity) and store the global frame
 * counter g_060987BC into the per-type slot g_06099780[unk34 - 1].
 * fidelity: HIGH - unk48 byte, unkBA word (unsigned bit test), unk43 byte, unk34 byte index;
 * g_06099780[unk34-1] and g_060987BC are long. Computed calls via braf_rewrite.py. */
extern void actor_enter_pose_mode(void *entity);
extern void player_input_anim_select(void *entity);
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern s32  g_06099780[];
extern s32  g_frame_tick;        /* global frame counter */

void player_tick_input_recovery(void *a0, void *entity) {
    if (ACTOR_M8(entity, action_pending) != 0 && (ACTOR_MU16(entity, type_flags) & 0x40)) {
        actor_enter_pose_mode(entity);
        actor_anim_frame_update(a0, entity);
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_RESPONSE;
        return;
    }
    player_input_anim_select(entity);
    g_06099780[ACTOR_WORK(entity)->slot_id - 1] = g_frame_tick;
}
