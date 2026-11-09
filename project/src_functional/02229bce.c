/* player_enter_join_countdown_source_alias [0x02229bce]: Clears health and arms sixty-frame join countdown; v1.09 binds source to 02229bca. */
#include "dh_work.h"
#include "dh.h"
/* 0x0229bce (func_02229bce) - actor state handler (entity=r5; the task arg r4 is unused). Clears the
 * HP word unk56 if set, arms a countdown unkC8 = 60 with unkCC = 1, requests state unk43 = 32, runs
 * release_slot_entry(entity), then tail-calls func_0223abd0(entity).
 * fidelity: HIGH - bsrf release_slot_entry (0x2170b4), braf func_0223abd0 (0x23abd0). unk56/CC word,
 * unkC8 long, unk43 byte. */
extern void release_slot_entry(void *entity);
extern void actor_spawn_counted_effect(void *entity);

void player_enter_join_countdown_source_alias(void *task, void *entity) {
    (void)task;
    if (ACTOR_M16(entity, health) != 0)
        ACTOR_M16(entity, health) = 0;
    M32(entity, 0xC8) = 60;
    M16(entity, 0xCC) = 1;
    ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_JOIN_COUNTDOWN;
    release_slot_entry(entity);
    actor_spawn_counted_effect(entity);
}
