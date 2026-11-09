/* actor_tick_scattered_projectile [0x0221279c]: Four states randomize pose/velocity, apply gravity, bounce then blink out. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221279c - 4-state projectile / debris actor (muzzle-flash shot that scatters,
 * arcs under gravity, bounces off a floor, then blinks out). Installed as a task
 * handler by the scripted spawner 0x02212350; task->unk8 = entity, task->unk10 = state,
 * task->unkF = a signed "power/spread" exponent. Sibling of 0x02211108 (same gravity /
 * bounce / matrix-draw cluster, same trail-spawn handler func_0223194a, same g_0604ebe4).
 *
 * state 0 (init/scatter): with shift sh = (s8)task->unkF >> 1, jitter 9 fields by
 *   ashl(rng-window, sh): position unk8/unkC/unk10 (+-0x4000), rotation unk24/26/28
 *   (+-0x2000), velocity unk14 (+-0x1000), unk18 (0..0x1FFF, always up), unk1C (+-0x1000);
 *   seed rotation-velocity unk2A/2C/2E and unk38 = 0x80000; if (unk80 & 1) spawn a trail
 *   object (func_0220ff28 / handler func_0223194a) with child->unk68 = g_0604ebe4[unk36],
 *   child->unk64 = unk60, child->unk3C = 1, and set unk3C bit5. -> state 1, run on.
 * state 2 entry only: clear rotation-velocity unk2A/2C/2E, then run physics.
 * state 1 (physics, also reached by 0 and 2): gravity unk18 -= 0x400; integrate rotation
 *   (unk24 += unk2A, etc.); move/draw-prep func_022173f4(entity, &g_060521f4[unk64<<4]);
 *   reflect velocity on wall hits (g_060B88A4 & 3 -> unk14 = -unk14, & 12 -> unk1C = -unk1C);
 *   on floor (unkC < unk60): clamp unkC, rebound (unk14>>=1, unk18 = -(unk18>>1), unk1C>>=1),
 *   re-randomize spins, clear unk3C bit5, and once the rebound is weak (unk18 < 0x1500) and
 *   not already dying, enter state 2 with countdown unk68 = 18.
 * shared tail: in state 2, count unk68 down; at < 0 clear unk3C bit5, deactivate (task->unk0 = 0),
 *   return. Draw (push matrix g_06098878, load/transform/rotate, submit sprite, pop) unless
 *   dying and on an even unk68 frame (blink).
 * state 3 (or other): draw only.
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - transcribed vs fn.py address-annotated SH-2 disasm [0x21279c,0x212b20).
 * func_0224a360 (r0/r1 ABI) verified as a variable left shift (modeled by ashl below, incl.
 * its count<0 -> identity and count>=32 -> 0 guards); r12 = rng_next (0x02210020). The
 * state-0 random writes to unk2A/2C/2E are overwritten by the state-2 clear it falls into -
 * their rng side effects (PRNG advance) are preserved by issuing the calls. The binary stores
 * child->unk68 twice with the identical value (compiler artifact); written once here. */

extern s32   game_random_next(void);                      /* rng_next */
extern void *task_spawn_list_work7c(void *task, void *handler); /* spawn 0x7c-byte object */
extern void  effect_follow_parent_task(void);                      /* trail-object handler template */
extern void  collision_sweep_pose(void *entity, void *param); /* move / draw-prep */
extern void  mtx_load_abc(void);                       /* 0x022149aa */
extern void  mtx_apply_vec_at8(void *entity);          /* 0x02214a40 */
extern void  mtx_rotate_zyx_at24(void *entity);        /* 0x02214a06 */
extern void  model_submit_culled(void *entity, s32 flag);    /* sprite submit */

extern s32 g_actor_slot_masks[];   /* per-index trail parameter table */
extern u8  g_060521f4[];   /* 16-byte move-parameter records, indexed by unk64 */
extern s32 g_060b88a4;     /* collision-result flags from the mover */
extern s32 g_matrix_stack_cursor;     /* active matrix-stack pointer */

/* models func_0224a360: variable arithmetic left shift (r0 = val, r1 = cnt) */
static s32 ashl(s32 val, s32 cnt)
{
    if (cnt < 0) {
        return val;
    }
    if (cnt >= 32) {
        return 0;
    }
    return val << cnt;
}

void actor_tick_scattered_projectile(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32 sh = (s32)M8(task, 0xF) >> 1;

    switch (TASK_S8(task, state)) {
    case 0:
        M32(ent, 0x08) += ashl((s32)(game_random_next() & 0x7FFF) - 0x4000, sh);
        M32(ent, 0x0C) += ashl((s32)(game_random_next() & 0x7FFF) - 0x4000, sh);
        M32(ent, 0x10) += ashl((s32)(game_random_next() & 0x7FFF) - 0x4000, sh);
        M16(ent, 0x24) += ashl((s32)(game_random_next() & 0x3FFF) - 0x2000, sh);
        M16(ent, 0x26) += ashl((s32)(game_random_next() & 0x3FFF) - 0x2000, sh);
        M16(ent, 0x28) += ashl((s32)(game_random_next() & 0x3FFF) - 0x2000, sh);
        M32(ent, 0x14) += ashl((s32)(game_random_next() & 0x1FFF) - 0x1000, sh);
        M32(ent, 0x18) += ashl((s32)(game_random_next() & 0x1FFF), sh);
        M32(ent, 0x1C) += ashl((s32)(game_random_next() & 0x1FFF) - 0x1000, sh);
        M16(ent, 0x2A) = (game_random_next() & 0xFFF) + 0xF800;
        M16(ent, 0x2C) = (game_random_next() & 0xFFF) + 0xF800;
        M16(ent, 0x2E) = (game_random_next() & 0xFFF) + 0xF800;
        M32(ent, 0x38) = 0x80000;
        if (MU32(ent, 0x80) & 1) {
            void *child = MP(task_spawn_list_work7c(task, (void *)&effect_follow_parent_task), 0x8);
            M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x36)];
            M8 (child, 0x5F) = 0;
            M16(child, 0x60) = 0;
            M16(child, 0x62) = 0;
            M32(child, 0x64) = M32(ent, 0x60);
            M32(child, 0x3C) = 1;
            M32(ent, 0x3C) |= 0x20;
        }
        TASK_S8(task, state) = 1;
        /* fallthrough */
    case 2:
        M16(ent, 0x2A) = 0;
        M16(ent, 0x2C) = 0;
        M16(ent, 0x2E) = 0;
        /* fallthrough */
    case 1:
        M32(ent, 0x18) -= 0x400;
        M16(ent, 0x24) += M16(ent, 0x2A);
        M16(ent, 0x26) += M16(ent, 0x2C);
        M16(ent, 0x28) += M16(ent, 0x2E);
        collision_sweep_pose(ent, &g_060521f4[M32(ent, 0x64) << 4]);
        if (g_060b88a4 & 3) {
            M32(ent, 0x14) = -M32(ent, 0x14);
        }
        if (g_060b88a4 & 12) {
            M32(ent, 0x1C) = -M32(ent, 0x1C);
        }
        if (M32(ent, 0x0C) < M32(ent, 0x60)) {
            M32(ent, 0x0C) = M32(ent, 0x60);
            M32(ent, 0x14) >>= 1;
            M32(ent, 0x18) = -(M32(ent, 0x18) >> 1);
            M32(ent, 0x1C) >>= 1;
            M16(ent, 0x2A) = (game_random_next() & 0xFFF) + 0xF800;
            M16(ent, 0x2C) = (game_random_next() & 0xFFF) + 0xF800;
            M16(ent, 0x2E) = (game_random_next() & 0xFFF) + 0xF800;
            M32(ent, 0x3C) &= ~0x20;
            if (M32(ent, 0x18) < 0x1500 && TASK_S8(task, state) != 2) {
                TASK_S8(task, state) = 2;
                M32(ent, 0x68) = 18;
            }
        }
        break;
    default: /* state 3 */
        goto tail;
    }

    if (TASK_S8(task, state) == 2) {
        if (--M32(ent, 0x68) < 0) {
            M32(ent, 0x3C) &= ~0x20;
            TASK_PTR(task, handler) = 0;
            return;
        }
    }

tail:
    if (TASK_S8(task, state) == 2 && (MU32(ent, 0x68) & 1) == 0) {
        return;
    }
    g_matrix_stack_cursor += 48;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 48;
}
/* fidelity: HIGH - full 4-state transcription vs SH-2 disasm [0x21279c,0x212b20); ashl models func_0224a360 variable left shift, rng side effects preserved. */
