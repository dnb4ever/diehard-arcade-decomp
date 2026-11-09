/* player_tick_hit_recovery [0x02228aa6]: Animation then hit dispatch; loop selects recovery countdown or zero-health state. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228aa6 (actor FSM step) - first call func_0222dc2c(a0, entity) (result unused);
 * if entity->unk48 is set, tail-call func_0222adaa(entity). Otherwise, if unk54 & 2:
 *   unk56 (s16) == 0 -> unk43 = 0x1F, unk84 = 0x20000000
 *   unk56 != 0        -> unkC4 = 0xC,  unk43 = 0xA
 * then tail-call func_0222ad7a(entity); if unk54 bit 1 clear, plain return.
 * fidelity: HIGH - unk48 byte, unk54 word unsigned bit test, unk56 word sign-extended
 * tested via tst (nonzero incl. negative takes the C4 path), unk43 byte, unk84/C4 long.
 * Computed branches (bsrf/braf) via braf_rewrite.py. */
extern s32  actor_anim_frame_update_entry(s32 a0, void *entity);
extern void actor_dispatch_pose_and_type(void *entity);
extern void actor_decode_pose_direction(void *entity);

void player_tick_hit_recovery(void *a0, void *entity) {
    actor_anim_frame_update_entry((s32)a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);   /* bra tail-jump */
        return;
    }
    if (ACTOR_U16(entity, animation_events) & 2) {
        if (ACTOR_M16(entity, health) == 0) {
            ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_DEFEATED;
            ACTOR_WORK(entity)->action_category = 0x20000000;
        } else {
            M32(entity, 0xC4) = 0xC;
            ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_TIMED_RECOVERY;
        }
        actor_decode_pose_direction(entity);   /* bra tail-jump */
    }
}
