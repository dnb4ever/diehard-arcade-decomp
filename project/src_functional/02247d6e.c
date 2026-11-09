/* enemy_enter_guard [0x02247d6e]: Validates the target, selects a random guard duration and enters state four. */
#include "dh_work.h"
#include "dh.h"
/* 0x02247d6e actor_block_setup - set up a slot entity's block/guard reaction.
 * arg0 = actor; arg0->unk120 = target slot index.
 *   if (g_06099590[idx] == 0) return 0;                 (slot inactive)
 *   entity = g_060995B0[idx]->unk8;
 *   if (entity->unk84 & 0x20040000) return 0;           (target busy/invuln)
 *   arg0->unkD4 = 4;  arg0->unkD8 = 0;
 *   v   = rand7() & 7;                                  (func_02210020 -> small random)
 *   lim = ~arg0->unkE8 & 7;
 *   if (v > lim) v = lim;                               (clamp to available count)
 *   if (func_02245fc4(arg0, entity) != 0) v += 2;
 *   arg0->unkC4 = v;
 *   arg0->unk84 = 0x400;
 *   arg0->unkE0 &= 0x301;
 *   if (arg0->unk5A != arg0->unk148) { arg0->unk41 = 1; arg0->unk5C = arg0->unk148; }
 *   return 1;
 * fidelity: HIGH, with two documented ABI assumptions: func_02210020 is a nilary
 * value-returning helper (result masked to 0..7 - rand-like; r4 holds leftover idx*4
 * but is not set for the call), and func_02245fc4 returns an s32 that the compiler
 * tests (tst r0,r0 directly after the bsrf, i.e. r0 = its return, not ~unkE8 as m2c
 * mis-modelled). g_06099590 s32[] active-flags, g_060995B0 void*[] slot objects (same
 * bases as 0x22a708). Matches m2c --asm. */
extern s32  game_random_next(void);                 /* small-random helper */
extern s32  enemy_classify_attack_timing(void *actor, void *entity);
extern s32  g_actor_slot_ids[];                         /* per-slot active flags */
extern void *g_actor_slot_tasks[];                        /* per-slot objects */

s32 enemy_enter_guard(void *arg0) {
    s32 idx = M32(arg0, 0x120);
    void *entity;
    s32 v, lim;

    if (g_actor_slot_ids[idx] == 0) {
        return 0;
    }
    entity = TASK_PTR(g_actor_slot_tasks[idx], work);
    if (ACTOR_M32(entity, action_category) & 0x20040000) {
        return 0;
    }
    M32(arg0, 0xD4) = DH_ENEMY_BEHAVIOR_GUARD;
    M32(arg0, 0xD8) = 0;
    v = game_random_next() & 7;
    lim = ~M32(arg0, 0xE8) & 7;
    if (v > lim) {
        v = lim;
    }
    if (enemy_classify_attack_timing(arg0, entity) != 0) {
        v += 2;
    }
    M32(arg0, 0xC4) = v;
    M32(arg0, 0x84) = 0x400;
    M32(arg0, 0xE0) = M32(arg0, 0xE0) & 0x301;
    if (M16(arg0, 0x5A) != M16(arg0, 0x148)) {
        M8(arg0, 0x41) = 1;
        M16(arg0, 0x5C) = (s16)M16(arg0, 0x148);
    }
    return 1;
}
