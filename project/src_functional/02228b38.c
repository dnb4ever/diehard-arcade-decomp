/* player_enter_input_recovery [0x02228b38]: Resolves movement, sets handler 11/action flags then shared input-recovery body. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228b38 (actor FSM step) - entry wrapper: run func_022170ea(entity, 0), set state 0x43=0xB,
 * unk84=0x4000, unk88=0, then tail-fall (bra) into the shared body func_02228b68(a0,entity).
 * fidelity: HIGH - unk43 byte, unk84/88 long; the trailing `bra 0x228b68` is a tail jump into
 * the adjacent function func_02228b68 (decompiled separately). */
extern void actor_resolve_movement(void *entity, s32 arg);
extern void player_tick_input_recovery(void *a0, void *entity);

void player_enter_input_recovery(void *a0, void *entity) {
    actor_resolve_movement(entity, 0);
    ACTOR_WORK(entity)->state_handler  = 0xB;
    ACTOR_WORK(entity)->action_category = 0x4000;
    ACTOR_WORK(entity)->action_state = 0;
    player_tick_input_recovery(a0, entity);   /* bra tail-jump */
}
