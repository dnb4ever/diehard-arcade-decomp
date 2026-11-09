/* player_join_arm [0x02229b60]: Existing map corroborated by source: (extra) player-actor FSM state: arms the join countdown that */
#include "dh_work.h"
#include "dh.h"
/* 0x02229b60 (player_join_arm) - a player-actor FSM state that ARMS the join countdown consumed by
 * the join-commit state func_02229c00. Handler ABI (a0, entity): entity = r5.
 *   animation_update(a0, entity);                 (actor_anim_frame_update)
 *   if (entity->unk54 & 2) {                        (ready bit)
 *       if (entity->unk56 != 0) entity->unk56 = 0;
 *       entity->unk c8 = 60;    (join countdown - func_02229c00 decrements this)
 *       entity->unk cc = 1;     (blink phase)
 *       entity->unk43 = 0x20;
 *       func_0222ad7a(entity);
 *       release_slot_entry(entity);
 *       func_0223abd0(entity);  (tail)
 *   }
 * Computed branches resolved (braf_resolve): bsrf->0x22dc2c/0x22ad7a/0x2170b4, braf->0x23abd0.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. unk54 u16 (bit 1 test), unk56 word. */
extern s32  actor_anim_frame_update(void *a0, void *entity);  /* 0x0222dc2c animation_update */
extern void actor_decode_pose_direction(void *entity);            /* 0x0222ad7a (undecompiled) */
extern void release_slot_entry(void *entity);            /* 0x022170b4 (undecompiled) */
extern void actor_spawn_counted_effect(void *entity);            /* 0x0223abd0 (undecompiled) */

void player_join_arm(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_U16(entity, animation_events) & 2) {
        if (ACTOR_M16(entity, health) != 0)
            ACTOR_M16(entity, health) = 0;
        M32(entity, 0xc8) = 60;
        M16(entity, 0xcc) = 1;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_JOIN_COUNTDOWN;
        actor_decode_pose_direction(entity);
        release_slot_entry(entity);
        actor_spawn_counted_effect(entity);
    }
}
