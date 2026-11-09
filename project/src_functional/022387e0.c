/* effect_finish_owner_action [0x022387e0]: Follows parent task to actor, calls completion helper then cancels self. */
#include "dh.h"
/* 0x022387e0 - call-through helper:
 *   data = *(u32 *)(*(u32 *)(arg0 + 20) + 8);
 *   func_0223ac9c((void *)data);
 *   *(s32 *)arg0 = 0;
 * fidelity: HIGH - delay slot loads @(20,r4) then @(8,r4) before the bsrf;
 * store of 0 back to *arg0 after the call; returns void (r3 unused). */
extern void actor_spawn_special_debris(void *entity);  /* 0x0223ac9c */

void effect_finish_owner_action(u32 arg0) {
    actor_spawn_special_debris((void *)MU32(MU32(arg0, 20), 8));
    M32(arg0, 0) = 0;
}
