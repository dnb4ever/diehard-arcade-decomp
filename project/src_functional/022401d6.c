/* actor_dispatch_action_preserve_motion [0x022401d6]: Resets action fields and dispatches its type/action handler before shared poststeps. */
#include "dh.h"
/* 0x022401d6  (re)enter an actor's scripted behaviour for one frame.
 * Reset flags (unk48=0, unk41=1, unk88=0), then dispatch the behaviour handler
 * picked by state:
 *   tbl = g_060918A0[actor->unkBA & 0x3F];   (per-state handler table)
 *   fn  = tbl[actor->unk108];                 (sub-state handler)
 *   fn(actor, &frame);                         (callee writes frame[0] = result)
 * followed by fixed post-steps actor_deactivate_if_active/ce(actor). If a pending hit is set
 * (unkFC != 0 && unk56 < 0x96): run reaction func_02245c88, clear unkFC, set
 * unk46=1, stamp g_0609624A[unk34] = 0x295, and func_02241a10(arg0, actor).
 * Always: func_022418fa(arg0, actor, frame[0]); func_02241b2c(actor); clear the
 * motion accumulators unk14/18/1C; return frame[0].
 * fidelity: HIGH - indirect handler called (actor, &local 12-byte frame) with
 *   r5 = frame pointer (mov r15,r5); its written frame[0] is the return value. */
extern void *g_060918A0[];   /* per-state handler tables */
extern u8 g_0609624A[];      /* per-actor word scratch, 0x20 stride (word at +0) */
void actor_deactivate_if_active(void *actor);                  /* 0x022418a0 */
void actor_dispatch_type_completion(void *actor);                  /* 0x022418ce */
void actor_finish_arrival(s32 arg0, void *actor, s32 r); /* 0x022418fa */
void actor_spawn_aimed_shot(s32 arg0, void *actor);        /* 0x02241a10 */
void actor_latch_action_completion(void *actor);                  /* 0x02241b2c */
void enemy_load_action_descriptor(void *actor);                  /* 0x02245c88 */

s32 actor_dispatch_action_preserve_motion(s32 arg0, void *actor) {
    u8 frame[12];
    void (*fn)(void *, void *);

    M8(actor, 0x48) = 0;
    M8(actor, 0x41) = 1;
    M32(actor, 0x88) = 0;
    M32(frame, 4) = arg0;

    fn = (void (*)(void *, void *))
         MP(g_060918A0[M16(actor, 0xBA) & DH_ACTOR_TYPE_MASK], M32(actor, 0x108) * 4);
    MP(frame, 8) = (void *)fn;
    fn(actor, frame);

    actor_deactivate_if_active(actor);
    actor_dispatch_type_completion(actor);

    if (M32(actor, 0xFC) != 0 && M16(actor, 0x56) < 0x96) {
        M8(actor, 0x6D) = 3;
        enemy_load_action_descriptor(actor);
        M8(actor, 0x6D) = 0;
        M32(actor, 0xFC) = 0;
        M8(actor, 0x46) = 1;
        M16(g_0609624A, (s32)M8(actor, 0x34) << 5) = 0x295;
        actor_spawn_aimed_shot(arg0, actor);
    }

    actor_finish_arrival(arg0, actor, M32(frame, 0));
    actor_latch_action_completion(actor);
    M32(actor, 0x14) = 0;
    M32(actor, 0x18) = 0;
    M32(actor, 0x1C) = 0;
    return M32(frame, 0);
}
