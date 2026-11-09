/* Round 4: ROM 06048298..482a4 uses peer coordinates directly for the path-C angle; the projected offset is used only by path B. Evidence: work/decomp4/43fe0.asm. */
#include "dh_work.h"
/* enemy_sweep_toward_target [0x02243fe0]: Selects target and obstacle policy for the three combat approach sweeps. */
#include "dh.h"

/* 0x02243fe0 - per-actor "sweep toward target" path/collision resolver,
 * the TRIPLED sibling of func_022444b6. Census entry is the real prologue.
 *
 * idx = actor->unk34 selects two parallel per-player scratch tables:
 *   tbl10 = 0x06097128 + (s16)(idx*0x24)  (unk0 = step-selector word;
 *                                          +4.. = per-slot step word;
 *                                          +0x10.. = found-target ptrs)
 *   tbl13 = 0x060B6D9C + (s16)(idx*0x48)  (collision box: +0x18,+0x1C,+0x20,+0x24)
 *
 * flags = actor->unkE0 pick one of three sweep variants:
 *   (flags & 0x40)                   -> PATH D: target from g_060E0F4C[unk12C],
 *                                       single sweep (<5), func_02244670, ret 2 on fail
 *   !(flags&0x40) &&  (flags&0x10)   -> PATH B: target from g_060995B0[unk128],
 *                                       publishes step to g_0609740C, single sweep (<5),
 *                                       func_02244670, ret 2 on fail
 *   !(flags&0x40) && !(flags&0x10)   -> PATH C: gated by func_02244874 reachability:
 *                                       reachable  -> sweep (<3) off g_0608DBD4, status
 *                                                     g_0608DBF4 -> maybe flip selector
 *                                       blocked    -> sweep (<5) off g_0608DBC0, then
 *                                                     switch g_0608DBDC {0:func_02244820,
 *                                                     2:flip} -> write selector
 *   all paths then run the shared direction-rehome tail (block_42/52).
 *
 * Calls (all resolved; m2c emitted M2C_ERROR for every bsrf):
 *   atan2_fixed(dy,dx)                 angle (atan2-like)       3x bsrf -> 0x22130b4
 *   dispatch_cmd(&pos,&box,0,idx)       collision hit-test       jsr @r8 (mova+0xFFFD37F4)
 *   func_02210020()                      PRNG                     bsrf -> 0x210020
 *   func_02244670(actor,tgt,tbl10,n,aw)  reaction-slot finalize (5th arg s16, on stack)
 *   func_02244820(actor,tgt)             move-permit flag (2 args, NOT 4)
 *   func_02244874(actor,tgt,&pos)        AABB reachability (nonzero=ok)
 *
 * m2c CORRECTIONS (see notes/functional_bandbig23.md):
 *   - `tst #128` is bit 7 only: `flags & 0x80`, NOT m2c's `flags & ~0x7F`.
 *   - the final bsrf (0x24447e) is func_02210020(): the random re-pick is
 *     `var_r4 = func_02210020() & 7`.  m2c dropped the call and reused the stale
 *     compare value `(var_r4+4)&7` -> a real logic bug in m2c's listing.
 *   - func_02244820 takes 2 args (prior 4-arg draft was wrong).
 * fidelity: HIGH - hand-traced against fn.py / m2c_fn.py --asm. */

s32   atan2_fixed(s32 dy, s32 dx);                             /* 0x022130b4 */
void *dispatch_cmd(void *pos, void *box, s32 z, s32 idx);       /* 0x02217838 */
s32   game_random_next(void);                                       /* 0x02210020 PRNG */
s32   enemy_dispatch_counter_reaction(void *actor, void *tgt, void *tbl10,
                    s32 count, s16 angle);                       /* 0x02244670 */
s16   enemy_test_dodge_side(void *actor, void *tgt);                     /* 0x02244820 */
void *aabb_overlap_box(void *actor, void *tgt, void *pos);          /* 0x02244874 */

s32 enemy_sweep_toward_target(void *actor)
{
    s8    idx   = ACTOR_M8(actor, slot_id);
    void *tbl10 = (u8 *)(uintptr_t)0x06097128 + (s16)(idx * 0x24);
    void *tbl13 = (u8 *)(uintptr_t)DH_ACTOR_MOVEMENT_RECORDS_ADDR + (s16)(idx * 0x48);
    void *p158  = ACTOR_MP(actor, action_config_tables);
    s32   flags = ACTOR_M32(actor, state_flags);
    s16   sp0   = (s16)M32(tbl10, 0);          /* selector word; reused in PATH C */
    u32   sel4  = (u32)(u16)sp0 * 4;
    void *var11 = (void *)0;
    u32   step  = 0;
    s16   anglew = 0;
    s32   fr[14];                              /* 0x38-byte call frame */

    M32(fr, 0x1C) = M32(tbl13, 0x18);
    M32(fr, 0x20) = M32(tbl13, 0x1C);
    M32(fr, 0x24) = M32(tbl13, 0x20);
    M32(fr, 0x28) = M32(tbl13, 0x24);
    M32(fr, 0x30) = 0;                         /* pos.y */

    if (flags & 0x40) {
        /* PATH D - target from g_060E0F4C[unk12C] */
        s32   count;
        s32   angle;
        void *seq;

        var11 = MP(*(void **)(uintptr_t)(0x060E0F4C + (u32)ACTOR_M32(actor, effect_target_slot) * 4), 8);
        {
            s32 cx = ACTOR_M32(actor, target_offset_x) + M32(var11, 8);   /* spC */
            s32 cz = M32(var11, 0x10) + ACTOR_M32(actor, target_offset_z);
            angle  = atan2_fixed(cz - ACTOR_M32(actor, position_z), cx - ACTOR_M32(actor, position_x));
        }
        anglew = (s16)(-angle);
        step   = ((u32)(u16)(-angle) >> 0xC) + 1;
        seq    = MP(0x0608DBC0 + sel4, 0);
        count  = 0;
        do {
            void *seqp = seq;
            void *hit;
            s32   po;

            seq  = (u8 *)seqp + 2;
            step = 0xE & (M16(seqp, 0) + step);
            po   = (s32)step * 4;
            M32(fr, 0x2C) = ACTOR_M32(actor, position_x)    + M32(0x0608DB6C + po, 0);
            M32(fr, 0x34) = ACTOR_M32(actor, position_z) + M32(0x0608DB6C + po, 4);
            hit = dispatch_cmd((u8 *)fr + 0x2C, (u8 *)fr + 0x1C, 0, idx);
            if (hit == 0) {
                M16((u8 *)tbl10 + 4 + count * 2, 0) = -1;
                break;
            }
            M16((u8 *)tbl10 + 4 + count * 2, 0)    = (s16)step;
            MP((u8 *)tbl10 + 0x10 + count * 4, 0)  = MP(hit, 8);
            count++;
        } while (count < 5);

        if (enemy_dispatch_counter_reaction(actor, var11, tbl10, count, anglew) == 0) {
            return 2;
        }
    } else {
        s32 var_r5 = ACTOR_M32(actor, target_offset_x);
        s32 diff;
        s32 cx, cz;

        var11 = MP(*(void **)(uintptr_t)(DH_ACTOR_SLOT_TASKS_ADDR + (u32)ACTOR_M32(actor, player_target_slot) * 4), 8);
        diff  = ACTOR_M32(actor, position_x) - M32(var11, 8);
        if (!(flags & 0x80)) {                 /* bit 7 only (m2c showed ~0x7F) */
            s32 ad = diff < 0 ? -diff : diff;
            if (ad > 0x2CCCC) {
                var_r5 = (diff >> 1) + (diff >> 2);
            }
        }
        cx = var_r5 + M32(var11, 8);           /* spC */
        cz = M32(var11, 0x10) + ACTOR_M32(actor, target_offset_z);   /* sp18 */

        if (flags & 0x10) {
            /* PATH B - single sweep, publishes step to g_0609740C */
            s32   count;
            s32   angle;
            void *seq;

            angle  = atan2_fixed(cz - ACTOR_M32(actor, position_z), cx - ACTOR_M32(actor, position_x));
            anglew = (s16)(-angle);
            step   = (((u32)(u16)(-angle) >> 0xC) + 1) & 0xE;
            M32(0x0609740C, 0) = (s32)step;
            seq    = MP(0x0608DBC0 + sel4, 0);
            count  = 0;
            do {
                void *seqp = seq;
                void *hit;
                s32   po;

                seq  = (u8 *)seqp + 2;
                step = 0xE & (M16(seqp, 0) + step);
                po   = (s32)step * 4;
                M32(fr, 0x2C) = ACTOR_M32(actor, position_x)    + M32(0x0608DB6C + po, 0);
                M32(fr, 0x34) = ACTOR_M32(actor, position_z) + M32(0x0608DB6C + po, 4);
                hit = dispatch_cmd((u8 *)fr + 0x2C, (u8 *)fr + 0x1C, 0, idx);
                if (hit == 0) {
                    M16((u8 *)tbl10 + 4 + count * 2, 0) = -1;
                    break;
                }
                M16((u8 *)tbl10 + 4 + count * 2, 0)   = (s16)step;
                MP((u8 *)tbl10 + 0x10 + count * 4, 0) = MP(hit, 8);
                count++;
            } while (count < 5);

            if (enemy_dispatch_counter_reaction(actor, var11, tbl10, count, anglew) == 0) {
                return 2;
            }
        } else {
            /* PATH C - reachability-gated, no func_02244670 / no early ret 2 */
            s32 angle = atan2_fixed(M32(var11, 0x10) - ACTOR_M32(actor, position_z),
                                    M32(var11, 8) - ACTOR_M32(actor, position_x));
            step = ((u32)(u16)(-angle) >> 0xC) + 1;

            if (aabb_overlap_box(actor, var11, (u8 *)actor + 8) != 0) {
                /* reachable: short sweep (<3) off g_0608DBD4 */
                s32   count = 0;
                void *seq   = MP(0x0608DBD4 + sel4, 0);
                do {
                    void *seqp = seq;
                    void *hit;
                    s32   po;

                    seq  = (u8 *)seqp + 2;
                    step = 0xE & (M16(seqp, 0) + step);
                    po   = (s32)step * 4;
                    M32(fr, 0x2C) = ACTOR_M32(actor, position_x)    + M32(0x0608DB6C + po, 0);
                    M32(fr, 0x34) = ACTOR_M32(actor, position_z) + M32(0x0608DB6C + po, 4);
                    hit = dispatch_cmd((u8 *)fr + 0x2C, (u8 *)fr + 0x1C, 0, idx);
                    if (hit == 0) {
                        break;
                    }
                    MP((u8 *)tbl10 + 0x10 + count * 4, 0) = MP(hit, 8);
                    count++;
                } while (count < 3);

                if (M32(0x0608DBF4 + count * 4, 0) == 2) {
                    M32(tbl10, 0) = (s32)(u16)(sp0 ^ 1);
                }
            } else {
                /* blocked: sweep (<5) off g_0608DBC0, then switch g_0608DBDC */
                s32   count = 0;
                void *seq   = MP(0x0608DBC0 + sel4, 0);
                s32   sel;
                do {
                    void *seqp = seq;
                    void *hit;
                    s32   po;

                    seq  = (u8 *)seqp + 2;
                    step = 0xE & (M16(seqp, 0) + step);
                    po   = (s32)step * 4;
                    M32(fr, 0x2C) = ACTOR_M32(actor, position_x)    + M32(0x0608DB6C + po, 0);
                    M32(fr, 0x34) = ACTOR_M32(actor, position_z) + M32(0x0608DB6C + po, 4);
                    hit = dispatch_cmd((u8 *)fr + 0x2C, (u8 *)fr + 0x1C, 0, idx);
                    if (hit != 0) {
                        MP((u8 *)tbl10 + 0x10 + count * 4, 0) = MP(hit, 8);
                    } else {
                        if (aabb_overlap_box(actor, var11, (u8 *)fr + 0x2C) == 0) {
                            break;
                        }
                        MP((u8 *)tbl10 + 0x10 + count * 4, 0) = hit;   /* NULL */
                    }
                    count++;
                } while (count < 5);

                sel = M32(0x0608DBDC + count * 4, 0);
                if (sel == 0) {
                    sp0 = enemy_test_dodge_side(actor, var11);
                } else if (sel == 2) {
                    sp0 = (s16)((u16)sp0 ^ 1);
                }
                M32(tbl10, 0) = (s32)(u16)sp0;
            }
        }
    }

    /* block_42 - shared direction rehome */
    if (ACTOR_M8(actor, facing_right) != 0) {
        if (flags & 0x20000) {
            ACTOR_M32(actor, state_flags) &= (s32)0xFFFDFFFF;
            step += 8;
        } else if (ACTOR_M32(actor, position_x) >= M32(var11, 8)) {
            step += 8;
        } else {
            return 1;
        }
    } else {
        if (flags & 0x20000) {
            ACTOR_M32(actor, state_flags) &= (s32)0xFFFDFFFF;
        } else if (ACTOR_M32(actor, position_x) > M32(var11, 8)) {
            return 1;
        }
    }

    {
        void *dirtab = MP(p158, 0);
        s32   var_r4 = (s32)((step >> 1) & 7);
        s16   dir    = M16(dirtab, var_r4 * 2);

        if ((u16)ACTOR_M16(actor, animation_current) != (u16)dir) {
            if (((var_r4 + 4) & 7) == (u16)ACTOR_M32(actor, queued_attack_kind)) {
                var_r4 = game_random_next() & 7;
                dir    = M16(dirtab, var_r4 * 2);
            }
            ACTOR_M8(actor, animation_control)   = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_M16(actor, animation_requested)  = dir;
            ACTOR_M32(actor, queued_attack_kind) = var_r4;
        }
    }
    return 0;
}
