/* actor_state_1A_enter [0x02228d44]: Existing map corroborated by source: (member) per-state handler (g_06091100 table entry, */
#include "dh_work.h"
#include "dh.h"
/* 0x02228d44 actor_state_1A_enter(task, entity) - per-state handler (an entry in the
 * g_06091100 dispatch table). Advances the shared animation; when the anim run flags
 * entity->unk54 bit1, seeds unk5C = 0xDD and transitions the actor to state 0x1A.
 *   actor_anim_frame_update(task, entity);           (animation_update)
 *   if (entity->unk54 & 2) { entity->unk5C = 0xDD; entity->unk43 = 0x1A; }
 * fidelity: HIGH - 2-arg handler ABI (r4=task, r5=entity) matching the g_06091100 call
 * site in 0x22863E; actor_anim_frame_update exists; unk54 read as u16. Matches asm. */
extern s32 actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void actor_state_1A_enter(void *task, void *entity) {
    actor_anim_frame_update(task, entity);
    if (ACTOR_U16(entity, animation_events) & 2) {
        ACTOR_WORK(entity)->animation_requested = 0x00DD;
        ACTOR_WORK(entity)->state_handler = 0x1A;
    }
}
