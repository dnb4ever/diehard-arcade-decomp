/* actor_follow_target_depth [0x0224174e]: Moves actor Z toward a live player target with the original asymmetric clamp. */
#include "dh.h"
/* 0x0224174e  move field[0x10] toward a target, with an asymmetric clamp.
 * Only acts when 1 <= field[0x4a] <= 2. obj = g_060995b0[field[0x4a]];
 * sub = *(obj+8); delta = sub->field[0x10] - field[0x10]; clamp delta to
 * +0x4000 when positive, to -0x4000 when negative; field[0x10] += delta.
 * fidelity: HIGH - clamp is literally faithful (the negative branch always
 * forces -0x4000 since a negative delta is never >= 0x4000). */
extern void *g_actor_slot_tasks_legacy[];  /* runtime object table (HWRAM) */
void actor_follow_target_depth(void *p) {
    s8 k = M8(p, 0x4A);
    if (k < 1 || k > 2)
        return;
    void *obj = g_actor_slot_tasks_legacy[k];
    void *sub = MP(obj, 8);
    s32 delta = M32(sub, 0x10) - M32(p, 0x10);
    if (delta >= 0) {
        if (delta > 0x4000)
            delta = 0x4000;
    } else if (delta < 0x4000) {
        delta = -0x4000;
    }
    M32(p, 0x10) += delta;
}
