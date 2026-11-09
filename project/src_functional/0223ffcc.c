/* actor_dispatch_action_and_finish [0x0223ffcc]: Dispatches the type/action table, runs arrival hooks and clears movement accumulators. */
#include "dh.h"
/* 0x0223ffcc - reset an entity and invoke its selected action handler.
 * arg0 = handler argument (stashed for the handler to read); entity = r5.
 * Clears entity->unk48, sets unk41=1, clears unk88, then selects a handler via
 * a two-level table g_060918a0[0x3F & entity->unkBA][entity->unk108], builds a
 * 12-byte frame {out@0, handler@4, arg0@8}, calls handler(entity, frame) (which
 * writes out@0), runs fixed post-steps actor_deactivate_if_active/ce/fa and func_02241b2c,
 * clears entity->unk14/18/1C, and returns frame.out.
 * bsrf resolved: 0x22418a0, 0x22418ce, 0x22418fa, 0x2241b2c. fidelity: HIGH. */
extern void **g_060918a0[];
extern void actor_deactivate_if_active(void *entity);
extern void actor_dispatch_type_completion(void *entity);
extern void actor_finish_arrival(s32 arg0, void *entity, s32 out);
extern void actor_latch_action_completion(void *entity);

s32 actor_dispatch_action_and_finish(s32 arg0, void *entity) {
    u8 frame[12];
    void (*handler)(void *, void *);

    M8(entity, 0x48) = 0;
    M8(entity, 0x41) = 1;
    M32(entity, 0x88) = 0;

    M32(frame, 8) = arg0;
    handler = (void (*)(void *, void *))
              g_060918a0[0x3F & MU16(entity, 0xBA)][M32(entity, 0x108)];
    MP(frame, 4) = (void *)handler;
    handler(entity, frame);

    actor_deactivate_if_active(entity);
    actor_dispatch_type_completion(entity);
    actor_finish_arrival(arg0, entity, M32(frame, 0));
    actor_latch_action_completion(entity);

    M32(entity, 0x14) = 0;
    M32(entity, 0x18) = 0;
    M32(entity, 0x1C) = 0;
    return M32(frame, 0);
}
