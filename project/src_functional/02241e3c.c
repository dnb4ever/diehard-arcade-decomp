/* 0x02241e3c  actor_enter_statewalk: set up a few actor fields and call a sub-init.
 *   *(s32*)(actor+0xd4) = 16;  *(s32*)(actor+0xd8) = 5;  *(s32*)(actor+0x84) = 0x201;
 *   func_0223d0c6(actor);
 *   *(s32*)(actor+0xe0) &= 0x320;
 * fidelity: HIGH - field offsets are mov.w halves of packed pool longs; the callee
 *   gets r4 = actor in the bsrf delay slot; all stores mov.l-width. */
#include "dh_band21.h"

void actor_load_transition_pose(u8 *actor);

void actor_enter_statewalk(u8 *actor) {
    *(s32 *)(actor + 0xd4) = 16;
    *(s32 *)(actor + 0xd8) = 5;
    *(s32 *)(actor + 0x84) = 0x201;
    actor_load_transition_pose(actor);
    *(s32 *)(actor + 0xe0) &= 0x320;
}
