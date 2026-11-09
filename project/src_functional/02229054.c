/* player_tick_loop_to_state17 [0x02229054]: Shared animation update followed by event-bit-1 handler 0x17 selection. */
#include "dh_work.h"
#include "dh.h"
/* 0x022904 (func_02229054) - call animation_update(r4, entity), then if entity->unk54
 * has bit1 set, set unk43 = 23 (0x17).
 * fidelity: HIGH - bsrf resolved via ptr_resolve (0x22905a + 4 + 0x4bce = 0x22dc2c);
 * unk54 word (unsigned bit test), unk43 byte. */
extern s32 actor_anim_frame_update(void *task, void *entity);  /* animation_update */

void player_tick_loop_to_state17(void *task, void *entity) {
    actor_anim_frame_update(task, entity);
    if (ACTOR_U16(entity, animation_events) & 2)
        ACTOR_WORK(entity)->state_handler = 23;
}
