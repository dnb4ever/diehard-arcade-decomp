/* actor_state_dispatch [0x02244e2e]: Dispatches the current enemy state table entry unless the freeze flag is set. */
#include "dh_work.h"
#include "dh.h"
/* 0x02244e2e  actor_state_dispatch: call current-state handler via table.
 * If field[0xe0] & 0x40 set: return 0 (actor frozen/disabled). Otherwise
 * tail-call g_0609192c[field[0xe4]](actor).
 * fidelity: HIGH - exact (tst #64 = bit 0x40; jmp modelled as tail return). */
extern void *g_enemy_state_handlers[];  /* per-actor state-handler table */
s32 actor_state_dispatch(void *actor) {
    if (ACTOR_WORK(actor)->state_flags & 0x40)
        return 0;
    s32 (*handler)(void *) = (s32 (*)(void *))g_enemy_state_handlers[ACTOR_WORK(actor)->state_index];
    return handler(actor);
}
