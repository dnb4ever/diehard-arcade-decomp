/* player_tick_damage_recovery [0x02228874]: Hit/capability gate enters handler 9; animation loop chooses recovery or zero-health state. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228874 (actor FSM step) - first run animation_update(a0,entity). If entity->unk48 is set
 * and unkBA bit 0x40 is set: run func_0222bd54(entity), then animation_update(a0,entity) again
 * (refreshed arg0 from stack), set state 0x43=9, return. Else if unk54 bit1 is set: if unk56
 * != 0 -> unkC4=12, unk43=0xA; else -> unk43=0x1F, unk84=0x20000000; then tail-call
 * func_0222ad7a(entity). Otherwise plain return.
 * fidelity: HIGH - bsrf animation_update (0x22dc2c), func_0222bd54 (0x22bd54), braf tail
 * func_0222ad7a (0x22ad7a). unk48/43 byte, unkBA/54/56 word, unk84 long. */
extern void actor_enter_pose_mode(void *entity);
extern void actor_decode_pose_direction(void *entity);
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */

void player_tick_damage_recovery(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0 && (ACTOR_MU16(entity, type_flags) & 0x40)) {
        actor_enter_pose_mode(entity);
        actor_anim_frame_update(a0, entity);
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_RESPONSE;
        return;
    }
    if (ACTOR_U16(entity, animation_events) & 2) {
        if (ACTOR_M16(entity, health) != 0) {
            M32(entity, 0xC4) = 12;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_TIMED_RECOVERY;
        } else {
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_DEFEATED;
            ACTOR_WORK(entity)->action_category = 0x20000000;
        }
        actor_decode_pose_direction(entity);
    }
}
