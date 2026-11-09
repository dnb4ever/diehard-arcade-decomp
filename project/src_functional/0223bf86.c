/* obj_state_machine_8 [0x0223bf86]: Existing map corroborated by source: 8-state summon/projectile machine. arg0=task, ent=arg0->unk8, other=ent->unkD0. state=M8(arg0,0x10) cmp/eq 0..7. 6 spawn sites = func_020ff0a/func_020ff28(task, (src symbols_bandbig23.txt) */
#include "dh.h"

/* 0x0223bf86 - 8-state object/spawn machine (projectile-summon actor).
 *   arg0 = task;  ent = arg0->unk8 (entity);  other = ent->unkD0 (linked entity).
 *   state = arg0->unk10 -> cmp/eq cascade 0..7 (NOT a jump table; the prior
 *   passes' "6 ROM spawn-template pools" are PIC descriptor pointers, resolved
 *   below).  All 17 bsrf/jsr computed branches resolve to plain calls.
 *
 * Dispatch (M8(arg0,0x10)):
 *   0 -> seed unk5E/unk30/unk56, sub-dispatch on g_060E0F2C[idx]
 *        (1:spawn+state1 / 3:state2 / 0:state4), draw
 *   1 -> hover/charge: retire paths (block_40), spawn-and-die path, or the
 *        state->7 launch transition (trig setup + spawn 0x022319B2)
 *   2 -> ballistic fall (gravity 0x360), land check -> state 3 / bounce
 *   3 -> settle; if alive spawn+state1 else func_0223cd0c + state5
 *   4 -> sub-dispatch on g_060E0F2C[idx] (1:spawn+state1 / 3,4:state2 / 5:state6)
 *   5 -> countdown unkC4; expire -> state6; draw on even frames
 *   6 -> arg0->unk0 = 0 (done)
 *   7 -> launched arc: integrate unk18/24/26/28, func_022173f4, land -> reset+state1
 *
 * Calls resolved (m2c emitted M2C_ERROR / phantom .Lpool for every one):
 *   func_0220ff0a(task,desc)/func_0220ff28(task,desc) -> spawn child entity.
 *       descriptors: 0x0223C650 (data template, states 0/3/4),
 *                    0x0212350 (callback, state1 spawn-die),
 *                    0x022319B2 (callback, state1 launch helper).
 *       m2c's `.Lpool_23cNNN.unk8` == child->unk8 (the return, not a struct).
 *   sin_lookup(a)/cos_lookup(a) trig, func_022131c8(a,b) fixmul,
 *   func_022173f4(ent,rec) physics, sound_queue_push(a) sfx, func_02210020() RNG,
 *   func_0223c6bc / func_0223cd0c helpers, draw tail mtx_load_abc/0214a40/
 *   0214a06/0215a2c with g_06098878 +-0x30 (shared 0x23bxxx idiom).
 *
 * m2c CORRECTIONS (notes/functional_bandbig23.md):
 *   - state7: unkC/unk2A/unk2C writes follow func_02210020() (RNG) bsrf calls
 *     that m2c dropped, reusing stale regs.
 *   - state1 launch: the `jsr @r9` push/pop around sin_lookup is a save/
 *     restore of r5 (=(s16)other->unk64<<9) across the call; m2c fused the two
 *     distinct calls (sin_lookup then func_022131c8) into one phantom
 *     multi-arg call with `unksp0`.
 *   - the six `.Lpool_23cNNN` are spawn-descriptor pointers, not data structs.
 * fidelity: HIGH - hand-traced instruction-by-instruction against fn.py. */

extern s32   g_matrix_stack_cursor;

void *task_spawn_actor_work(void *task, void *desc);
void *task_spawn_list_work7c(void *task, void *desc);
s32   sin_lookup(s32 a);
s32   cos_lookup(s32 a);
s32   fixed_mul16(s32 a, s32 b);
void  collision_sweep_pose(void *ent, void *rec);
void  sound_queue_push(s32 a);
s32   game_random_next(void);
void  actor_draw_proximity_pose(void *ent, s32 a, s32 b);
void  effect_pool_release(void *ent, void *arg1, s32 b);
void  mtx_load_abc(void);
void  mtx_apply_vec_at8(void *obj);
void  mtx_rotate_zyx_at24(void *obj);
void  model_submit_culled(void *obj, s32 b);

static void draw_tail(void *ent)
{
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
}

/* spawn via func_0220ff0a(arg0, 0x0223C650) and link child to ent */
static void *spawn_c650(void *arg0, void *ent)
{
    void *child = task_spawn_actor_work(arg0, (void *)(uintptr_t)0x0223C650);
    void *sub;

    MP(ent, 0xD0)  = MP(child, 8);
    MP(ent, 0x118) = child;
    sub = MP(ent, 0xD0);
    M32(sub, 8)    = M32(ent, 8);
    M32(sub, 0xC)  = M32(ent, 0xC);
    M32(sub, 0x10) = M32(ent, 0x10);
    return child;
}

void obj_state_machine_8(void *arg0)
{
    void *ent   = MP(arg0, 8);
    void *other = MP(ent, 0xD0);
    s32   idx   = M8(ent, 0x34);
    s32   idx4  = idx * 4;

    switch ((s32)M8(arg0, 0x10)) {
    case 0: {
        s32 sub;
        M16(ent, 0x5E) = (s16)M32(0x060E0F0C + idx4, 0);
        M16(ent, 0x30) = M16(0x060E1080 + (s32)M16(ent, 0x5E) * 2, 0);
        M16(ent, 0x56) = 0x32;
        sub = M32(0x060E0F2C + idx4, 0);
        if (sub == 1) {
            M32(ent, 0xC) = M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0);
            M16(ent, 0x24) = 0;
            M16(ent, 0x28) = 0;
            spawn_c650(arg0, ent);
            M8(arg0, 0x10) = 1;
        } else if (sub == 3) {
            M8(arg0, 0x10) = 2;
        } else if (sub == 0) {
            M8(arg0, 0x10) = 4;
        }
        draw_tail(ent);
        return;
    }

    case 1:
        if (M32(0x060E0F2C + idx4, 0) == 0) {
            M32(other, 0x3C) = 1;
            M8(arg0, 0x10)   = 4;
            draw_tail(ent);
            return;
        }
        if (M8(other, 0x48) != 0) {
            M8(other, 0x48) = 0;
            if (M32(0x060E0F2C + idx4, 0) == 1 && M8(other, 0x4A) < 8) {
                /* decrement charge timer */
                M16(ent, 0x56) = (s16)(M16(ent, 0x56) - M16(other, 0x64));
                if (M16(ent, 0x56) <= 0) {
                    /* spawn-and-die: hand off to a falling fragment */
                    void *child = task_spawn_actor_work(arg0, (void *)(uintptr_t)0x0212350);
                    void *sub   = MP(child, 8);
                    M8(sub, 0xF)   = M8(0x060E1209 + (s32)(M16(ent, 0x5E) - 0xA) * 2, 0);
                    M32(sub, 8)    = M32(ent, 8);
                    M32(sub, 0xC)  = M32(ent, 0xC);
                    M32(sub, 0x10) = M32(ent, 0x10);
                    M8(sub, 0x48)  = 1;
                    M8(sub, 0x4A)  = M8(other, 0x4A);
                    M16(sub, 0xB8) = 0;
                    M8(sub, 0x49)  = 0;
                    M16(sub, 0x64) = 0x10;
                    M32(sub, 0x74) = 0x3000;
                    M16(sub, 0x60) = M16(other, 0x60);
                    M16(sub, 0x62) = M16(other, 0x62);
                    effect_pool_release(ent, child, 4);
                    M8(arg0, 0x10)   = 6;
                    M32(other, 0x3C) = 1;
                    return;
                }
                /* still charging: launch into state 7 */
                M32(0x060E0F2C + idx4, 0) = 3;
                M16(other, 0x64) = 0;
                if (M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0) {
                    sound_queue_push((s32)0xA01205FF);
                }
                M32(ent, 0x14) = -fixed_mul16(sin_lookup((s16)M16(other, 0x62)),
                                               M16(other, 0x64) << 9);
                {
                    s32 v = sin_lookup((s16)M16(other, 0x60));
                    s32 a = (v >= 0) ? sin_lookup((s16)M16(other, 0x60))
                                     : -sin_lookup((s16)M16(other, 0x60));
                    M32(ent, 0x18) = (a >> 3) + 0x1400;
                }
                M32(ent, 0x1C) = fixed_mul16(cos_lookup((s16)M16(other, 0x62)),
                                              M16(other, 0x64) << 9);
                M8(arg0, 0x10) = 7;
                if (!(M32(ent, 0x3C) & 0x20)) {
                    void *child = task_spawn_list_work7c(arg0, (void *)(uintptr_t)0x022319B2);
                    void *sub   = MP(child, 8);
                    M32(sub, 0x3C) = 1;
                    M32(sub, 0x68) = M32(DH_ACTOR_SLOT_MASKS_ADDR + (s32)M8(other, 0x4A) * 4, 0);
                    M8(sub, 0x5F)  = 0;
                    M16(sub, 0x60) = 0;
                    M32(sub, 0x64) = 0x9999;
                    M8(sub, 0x34)  = M8(other, 0x34);
                    M32(sub, 0x38) = 0x80000;
                    M32(ent, 0x3C) |= 0x20;
                }
                draw_tail(ent);
                return;
            }
        }
        /* block_40: slot handshake */
        if (M32(0x060E0F2C + idx4, 0) == 1) {
            actor_draw_proximity_pose(ent, 2, 4);
        } else if (M32(0x060E0F2C + idx4, 0) == 3) {
            M32(0x060E0F2C + idx4, 0) = 1;
        }
        draw_tail(ent);
        return;

    case 2:
        M32(ent, 0xC8) -= 0x360;
        M16(ent, 0x26) = (s16)(M16(ent, 0x26) + 0x400);
        M16(ent, 0x24) = (s16)(M16(ent, 0x24) + 0x100);
        M32(ent, 0x18) = M32(ent, 0xC8);
        M32(ent, 0x14) = M32(ent, 0x78);
        M32(ent, 0x1C) = M32(ent, 0x80);
        collision_sweep_pose(ent, (u8 *)(uintptr_t)0x060E0E88 + 4 + idx * 0x10);
        if (M32(ent, 0xC) < M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0)) {
            M32(ent, 0xC) = M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0);
            if (M32(ent, 0xC8) > -0x500) {
                M8(arg0, 0x10) = 3;
                M32(0x060E0F2C + idx4, 0) = 1;
            } else {
                M32(ent, 0xC8) = -(M32(ent, 0xC8) >> 2);
                M32(ent, 0x78) >>= 1;
                M32(ent, 0x80) >>= 1;
            }
        }
        draw_tail(ent);
        return;

    case 3:
        M16(ent, 0x24) = 0;
        M16(ent, 0x28) = 0;
        M32(ent, 0xC)  = M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0);
        if (M16(ent, 0x56) != 0 && M32(ent, 0x90) != 0) {
            M32(0x060E0F2C + idx4, 0) = 1;
            spawn_c650(arg0, ent);
            M8(arg0, 0x10) = 1;
        } else {
            effect_pool_release(ent, (void *)2, 4);
            M32(ent, 0xC4) = 0x28;
            M8(arg0, 0x10) = 5;
        }
        draw_tail(ent);
        return;

    case 4:
        if (M32(0x060E0F2C + idx4, 0) == 0) {
            return;
        }
        switch (M32(0x060E0F2C + idx4, 0)) {
        case 1:
            M32(ent, 0xC) = M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0);
            spawn_c650(arg0, ent);
            M8(arg0, 0x10) = 1;
            return;
        case 3:
        case 4:
            M8(arg0, 0x10) = 2;
            return;
        case 5:
            effect_pool_release(ent, (void *)2, 4);
            M8(arg0, 0x10) = 6;
            return;
        }
        return;

    case 5: {
        s32 c = M32(ent, 0xC4) - 1;
        M32(ent, 0xC4) = c;
        if (c != 0) {
            if (M32(ent, 0xC4) & 1) {
                return;
            }
        } else {
            M8(arg0, 0x10) = 6;
        }
        draw_tail(ent);
        return;
    }

    case 6:
        M32(arg0, 0) = 0;
        return;

    case 7:
        M32(ent, 0x18) -= 0x400;
        M16(ent, 0x24) = (s16)(M16(ent, 0x24) + M16(ent, 0x2A));
        M16(ent, 0x26) = (s16)(M16(ent, 0x26) + M16(ent, 0x2C));
        M16(ent, 0x28) = (s16)(M16(ent, 0x28) + M16(ent, 0x2E));
        collision_sweep_pose(ent, (u8 *)(uintptr_t)0x060E0E88 + 4 + idx * 0x10);
        if (M32(0x060B88A4, 0) & 3) {
            M32(ent, 0x14) = 0;
        }
        if (M32(0x060B88A4, 0) & 0xC) {
            M32(ent, 0x1C) = 0;
        }
        if (M32(ent, 0xC) < M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0)) {
            s32 pt  = M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0);
            s32 r   = game_random_next();
            u16 a24 = (u16)M16(ent, 0x24);   /* ROM's abs is a no-op on extu.w */
            M32(ent, 0xC)  = pt * 2 - M32(ent, 0xC);
            M16(ent, 0x28) = (s16)(-(r & 0x3FF) + 0x300);
            M32(ent, 0x14) >>= 1;
            M32(ent, 0x18) = -M32(ent, 0x18) >> 1;
            M32(ent, 0x1C) >>= 1;
            M16(ent, 0x2A) = (s16)((s32)((a24 - 0x2000) & 0x1FFF) >> 5);
            M16(ent, 0x2C) = (s16)(-(game_random_next() & 0x7FF) + 0x400);
            M32(ent, 0x3C) &= ~0x20;
            if (M32(ent, 0x18) <= 0x1500) {
                void *sub;
                M32(ent, 0xC)  = M32(0x060E120C + (s32)(M16(ent, 0x5E) - 0xA) * 4, 0);
                M16(ent, 0x28) = 0;
                M16(ent, 0x24) = 0;
                M32(ent, 0x14) = 0;
                M32(ent, 0x18) = 0;
                M32(ent, 0x1C) = 0;
                M16(ent, 0x2A) = 0;
                M16(ent, 0x2C) = 0;
                M16(ent, 0x2E) = 0;
                sub = MP(ent, 0xD0);
                M8(sub, 0x48)  = 0;
                M32(sub, 8)    = M32(ent, 8);
                M32(sub, 0xC)  = M32(ent, 0xC);
                M32(sub, 0x10) = M32(ent, 0x10);
                M32(0x060E0F2C + idx4, 0) = 1;
                M8(arg0, 0x10) = 1;
            }
        }
        draw_tail(ent);
        return;

    default:
        draw_tail(ent);
        return;
    }
}
