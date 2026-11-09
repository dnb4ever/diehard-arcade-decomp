/* actor_tick_thrown_body [0x02211108]: Task states handle re-hit, gravity, bounce and eventual removal. */
#include "dh_work.h"
#include "dh.h"
/* 0x02211108 (actor_update_thrown) - entity-behaviour state machine for a thrown /
 * tumbling enemy that can be re-hit, bounces, then dies. Installed as a task handler:
 * task->unk8 = entity, task->unk10 = state. Draws and (except while dying) registers
 * into the spatial grid via the tail func_0222eee4.
 *
 * Pre-update (states 1,2, when unk48 is set): if unk4A < 8, launch along the aim:
 *   unk14 = -fixmul(sin(unk62), unk64<<9); unk18 = (|sin(unk60)|>>3)+0x1400;
 *   unk1C = fixmul(cos(unk62), unk64<<9); -> state 2; unk56 -= unk64; and once per
 *   actor (unk3C bit 5) spawn a trail object (func_0220ff28 w/ handler func_0223194a)
 *   seeded with child->unk3C=1, unk68=g_0604ebe4[unk4A], unk5F=0, unk60=0, unk64=0x9999.
 *   (unk4A >= 8 just clears unk48.)
 * state 0 (init): unkC/unkA8 = 0x8CCC; zero velocity/rotation; unk30 = 41; alloc a
 *   collision slot (func_02228106 -> unk34, deactivate on failure); unk38 = 0x100000;
 *   clear unk3C bit5; unk56 = 100; unkBC = 0x7FFFFF00; register slot w/ g_06052244[0..3];
 *   -> state 1, run state 1.
 * state 1: move/collide func_022170ea(entity, 1).
 * state 2 (tumble/bounce): unk18 -= 0x400; integrate rotation (unk24+=unk2A etc.);
 *   move/collide; kill the velocity component that hit a wall (g_060B88A4 & 3 -> unk14=0,
 *   & 12 -> unk1C=0); once unkC drops below 0x8CCC, rebound: unkC = 0x11998-unkC, pick
 *   random spins/velocity (halved & reflected), and when the rebound is weak enough
 *   (unk18 <= 0x1500) settle back to state 1 - or, when the bounce budget unk56 is spent,
 *   go to state 3 with unk98 = 18.
 * state 3 (dying): count unk98 down; while alive blink (draw only on odd unk98) and skip
 *   the grid insert; at 0 run release_slot_entry, release g_060996D4[unk34-8] (unk34 > 8) and
 *   deactivate (task->unk0 = 0).
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; indirect jsr r10 resolved to
 * sin_lookup (0x02213190) and the spawn handler to func_0223194a (ptr_resolve). The
 * |sin(unk60)| uses one (pure) sin_lookup call where the SH-2 recomputes it. */

extern s32   sin_lookup(s32 angle);     /* sin_lookup */
extern s32   cos_lookup(s32 angle);     /* cos-phase table sibling */
extern s32   fixed_mul16(s32 a, s32 b);  /* fixmul_16_16 */
extern s32   actor_allocate_effect_slot(void);
extern void  actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3);
extern void  actor_resolve_movement(void *entity, s32 mode);
extern void  release_slot_entry(void *entity);
extern s32   game_random_next(void);          /* rng_next */
extern void *task_spawn_list_work7c(void *task, void *handler);
extern void  effect_follow_parent_task(void);          /* trail handler template */
extern void  mtx_load_abc(void);
extern void  mtx_apply_vec_at8(void *entity);
extern void  mtx_rotate_zyx_at24(void *entity);
extern void  model_submit_culled(void *entity, s32 flag);
extern void  collision_insert_grid(void *entity);

extern s32 g_actor_slot_masks[];    /* per-hit-index trail parameter table */
extern s32 g_06052244[];    /* actor-slot parameter block (4 words) */
extern s32 g_060b88a4;      /* collision-result flags from the mover */
extern u8  g_effect_slot_occupied_legacy[];
extern s32 g_matrix_stack_cursor;

void actor_tick_thrown_body(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);
    s8 st;

    if (state == 1 || state == 2) {
        if (M8(ent, 0x48) != 0) {
            if (M8(ent, 0x4A) >= 8) {
                M8(ent, 0x48) = 0;
            } else {
                s32 sv;
                M32(ent, 0x14) = -fixed_mul16(sin_lookup(M16(ent, 0x62)),
                                                M16(ent, 0x64) << 9);
                sv = sin_lookup(M16(ent, 0x60));
                if (sv < 0) {
                    sv = -sv;
                }
                M32(ent, 0x18) = (sv >> 3) + 0x1400;
                M32(ent, 0x1C) = fixed_mul16(cos_lookup(M16(ent, 0x62)),
                                               M16(ent, 0x64) << 9);
                TASK_S8(task, state) = 2;
                M16(ent, 0x56) -= M16(ent, 0x64);
                M8(ent, 0x48) = 0;
                if ((M32(ent, 0x3C) & 32) == 0) {
                    void *child = MP(task_spawn_list_work7c(task, (void *)&effect_follow_parent_task), 8);
                    M32(child, 0x3C) = 1;
                    M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x4A)];
                    M8(child, 0x5F) = 0;
                    M16(child, 0x60) = 0;
                    M32(child, 0x64) = 0x9999;
                    M32(ent, 0x3C) |= 32;
                }
            }
        }
    }

    switch (TASK_S8(task, state)) {
    case 0: {
        s8 slot;
        M32(ent, 0xC) = 0x8CCC;
        M32(ent, 0x14) = 0;
        M32(ent, 0x18) = 0;
        M32(ent, 0x1C) = 0;
        M16(ent, 0x24) = 0;
        M16(ent, 0x26) = 0;
        M16(ent, 0x28) = 0;
        M16(ent, 0x2A) = 0;
        M16(ent, 0x2C) = 0;
        M16(ent, 0x2E) = 0;
        M16(ent, 0x30) = 41;
        slot = (s8)actor_allocate_effect_slot();
        M8(ent, 0x34) = slot;
        if (slot < 0) {
            TASK_PTR(task, handler) = 0;
            return;
        }
        M32(ent, 0x38) = 0x100000;
        M32(ent, 0x3C) &= ~32;
        M16(ent, 0x56) = 100;
        M32(ent, 0xA8) = 0x8CCC;
        M32(ent, 0xBC) = 0x7FFFFF00;
        actor_register_movement_slot(ent, g_06052244[0], g_06052244[1], g_06052244[2], g_06052244[3]);
        TASK_S8(task, state) = 1;
    }
    /* fallthrough */
    case 1:
        actor_resolve_movement(ent, 1);
        break;
    case 2:
        M32(ent, 0x18) -= 0x400;
        M16(ent, 0x24) += M16(ent, 0x2A);
        M16(ent, 0x26) += M16(ent, 0x2C);
        M16(ent, 0x28) += M16(ent, 0x2E);
        actor_resolve_movement(ent, 1);
        {
            s32 cf = g_060b88a4;
            if (cf & 3) {
                M32(ent, 0x14) = 0;
            }
            if (cf & 12) {
                M32(ent, 0x1C) = 0;
            }
        }
        if (M32(ent, 0xC) < 0x8CCC) {
            M32(ent, 0xC) = 0x11998 - M32(ent, 0xC);
            M16(ent, 0x28) = (s16)(0x300 - (game_random_next() & 0x3FF));
            M32(ent, 0x14) >>= 1;
            M32(ent, 0x18) = (-M32(ent, 0x18)) >> 1;
            M32(ent, 0x1C) >>= 1;
            M16(ent, 0x2A) = (s16)(((((u32)(u16)M16(ent, 0x24)) - 0x2000) & 0x1FFF) >> 5);
            M16(ent, 0x2C) = (s16)(0x400 - (game_random_next() & 0x7FF));
            M32(ent, 0x3C) &= ~32;
            if (M32(ent, 0x18) <= 0x1500) {
                TASK_S8(task, state) = 1;
                M32(ent, 0xC) = 0x8CCC;
                M16(ent, 0x28) = 0;
                M16(ent, 0x24) = 0;
                M32(ent, 0x14) = 0;
                M32(ent, 0x18) = 0;
                M32(ent, 0x1C) = 0;
                M16(ent, 0x2A) = 0;
                M16(ent, 0x2C) = 0;
                M16(ent, 0x2E) = 0;
                if (M16(ent, 0x56) <= 0) {
                    TASK_S8(task, state) = 3;
                    M32(ent, 0x98) = 18;
                }
            }
        }
        break;
    case 3:
        M32(ent, 0x98) -= 1;
        if (M32(ent, 0x98) == 0) {
            release_slot_entry(ent);
            if (M8(ent, 0x34) > 8) {
                g_effect_slot_occupied_legacy[M8(ent, 0x34) - 8] = 0;
            }
            TASK_PTR(task, handler) = 0;
            return;
        }
        break;
    default:
        break;
    }

    st = TASK_S8(task, state);
    if (st != 3 || (M32(ent, 0x98) & 1) != 0) {
        g_matrix_stack_cursor += 0x30;
        mtx_load_abc();
        mtx_apply_vec_at8(ent);
        mtx_rotate_zyx_at24(ent);
        model_submit_culled(ent, 0);
        g_matrix_stack_cursor -= 0x30;
    }
    if (st != 3) {
        collision_insert_grid(ent);
    }
}
