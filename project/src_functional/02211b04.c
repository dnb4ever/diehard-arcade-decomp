/* actor_tick_bouncing_explosive [0x02211b04]: Arcing projectile task flies, bounces, re-arms and emits explosion/trail children. */
#include "dh_work.h"
#include "dh.h"
/* 0x02211b04 (actor_update_grenade) - the largest entity-behaviour state machine in
 * the family: a thrown/arcing projectile (grenade-like) that flies, bounces, re-arms on
 * hit, spawns trails/debris, then explodes. Installed as a task handler: task->unk8 =
 * entity, task->unk10 = state. Draws and (except while exploding) registers into the
 * spatial grid via the tail func_0222eee4.
 *
 * state 0 (init): unkC = 0x6666; zero velocity unk14/18/1C and unk78/7C/80; unk30 =
 *   0x40E; alloc collision slot (func_02228106 -> unk34, deactivate on failure);
 *   unk38 = 0x100000; unkA8 = 0x8CCC; unkBC = 0x7FFFFF00; register slot w/ g_06052264;
 *   unk62 = 0; clear unk3C bit5; unk9C = 0; -> state 1, run state 1.
 * state 1 (fly/arm): move func_022170ea(entity,1); on hit (unk48, unk4A < 8) spawn a
 *   trail (func_0220ff28 w/ func_0223194a, saved in unk9C) seeded unk68=g_0604ebe4[unk4A],
 *   unk64=0x599A; SFX 0xA01232FF (gated). If the charge unk64 >= 16: -> state 2, register
 *   func_02210858 w/ g_06052394, trail unk3C=2/unk58=24, and when task->unkF bit1 is set
 *   and a slot is free (func_0223e4a0) spawn an aimed child (func_0223cbb0(12,3,0) +
 *   func_0223a924). Else (< 16): -> state 3, register g_060524FC, kick unk78 = -/+0x2E14
 *   by sign of unk62, unk62 = 0, trail unk3C=1, set unk3C bit5, register g_06052274.
 * state 2 (aim): play anim (func_022109d4); snapshot velocity to unk78/7C/80; aim angle
 *   unk2C = unk62 + 0x4000; rotate the velocity vector by the aim (mtx_push_rotate_apply) ->
 *   unk14/18/1C; move; when the anim ends -> state 6 (unk98 = 18, release_slot_entry).
 * state 3 (bounce): on hit (unk48, unk4A < 8) re-arm -> state 2, reusing the unk9C trail
 *   (unk68 = g_0604ebe4[unk4A] | g_0604ebe4[unk34], unk3C=2, unk58=30), SFX + aimed child;
 *   otherwise integrate the arc (unk14 from unk78, spin unk28), move (mode 2) and, on a
 *   floor/wall hit (g_060B88A4 & 3), bounce -> state 4 (unkC=0x3333, unk78=-(unk78>>2)).
 * state 4 (settle): integrate/move; when the anim ends -> state 5, zero velocity, unk48=0.
 * state 5 (rest/arm): move; on hit (unk48, unk4A < 8) spawn a fresh trail like state 1 and
 *   -> state 2, register g_060526C4, SFX, and (task->unkF bit1 + free slot) an aimed child.
 * state 6 (explode): count unk98 down; while alive blink (draw on odd unk98) and skip the
 *   grid insert; at 0 run release_slot_entry, release g_060996D4[unk34-8] (unk34 > 8), deactivate.
 *
 * Every surviving state draws (push matrix, mtx_load_abc, apply vec @+8, rotate @+0x24,
 * submit model, pop) and tail-calls func_0222eee4 - except state 6 which blinks and skips it.
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; indirect jsr r10 = func_022170ea,
 * jsr r8 = func_0223a924, mova+add ff28 handler = func_0223194a (ptr_resolve); all bsrf
 * resolved. func_0223e4a0 nullary, func_0223a924 4-arg per their decomps. */

extern s32   actor_allocate_effect_slot(void);
extern void  actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3);
extern void  actor_resolve_movement(void *entity, s32 mode);
extern void  effect_pool_update_pose(s32 idx, s32 mode, void *src, void *save);
extern void *task_spawn_list_work7c(void *task, void *handler);
extern s32   sound_queue_push(u32 sound_id);
extern void  animation_bind_channel(void *obj, void *val);
extern s32   effect_pool_make_room(void);
extern void *effect_pool_spawn(s32 a0, s32 a1, void *a2);
extern s32   game_random_next(void);
extern s16   animation_sample_absolute(void *entity);
extern void  mtx_push_rotate_apply(s32 *vec, void *angles);
extern void  release_slot_entry(void *entity);
extern void  effect_follow_parent_task(void);
extern void  mtx_load_abc(void);
extern void  mtx_apply_vec_at8(void *entity);
extern void  mtx_rotate_zyx_at24(void *entity);
extern void  model_submit_culled(void *entity, s32 flag);
extern void  collision_insert_grid(void *entity);

extern s32 g_actor_slot_masks[];
extern s32 g_06052264[];   /* param block (state 0) */
extern s32 g_06052274[];   /* param block (state 1 branch) */
extern s32 g_06052394[];   /* config block */
extern s32 g_060524fc[];   /* config block */
extern s32 g_060526c4[];   /* config block */
extern s32 g_06052604[];   /* config block */
extern s32 g_sound_effects_enabled;
extern s32 g_060b88a4;
extern s32 g_transformed_vector[];
extern u8  g_effect_slot_occupied_legacy[];
extern s32 g_matrix_stack_cursor;

static void spawn_aimed_child(void *ent)
{
    void *o = effect_pool_spawn(12, 3, 0);
    if (o != 0) {
        M16(ent, 0x62) += (s16)((game_random_next() & 0x1FFF) + 0xF000);
        effect_pool_update_pose((s8)M8(MP(o, 8), 0x34), 3, (u8 *)ent + 8, ent);
    }
}

void actor_tick_bouncing_explosive(void *task)
{
    void *ent = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        s8 slot;
        M32(ent, 0xC) = 0x6666;
        M32(ent, 0x14) = 0;
        M32(ent, 0x18) = 0;
        M32(ent, 0x1C) = 0;
        M32(ent, 0x80) = 0;
        M32(ent, 0x7C) = 0;
        M32(ent, 0x78) = 0;
        M16(ent, 0x30) = 0x40E;
        slot = (s8)actor_allocate_effect_slot();
        M8(ent, 0x34) = slot;
        if (slot < 0) {
            TASK_PTR(task, handler) = 0;
            return;
        }
        M32(ent, 0x38) = 0x100000;
        M32(ent, 0xA8) = 0x8CCC;
        M32(ent, 0xBC) = 0x7FFFFF00;
        actor_register_movement_slot(ent, g_06052264[0], g_06052264[1], g_06052264[2], g_06052264[3]);
        M16(ent, 0x62) = 0;
        M32(ent, 0x3C) &= ~32;
        MP(ent, 0x9C) = 0;
        TASK_S8(task, state) = 1;
    }
    /* fallthrough */
    case 1:
        actor_resolve_movement(ent, 1);
        if (M8(ent, 0x48) != 0) {
            M8(ent, 0x48) = 0;
            if (M8(ent, 0x4A) < 8) {
                void *child = MP(task_spawn_list_work7c(task, (void *)&effect_follow_parent_task), 8);
                MP(ent, 0x9C) = child;
                M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x4A)];
                M8(child, 0x5F) = 0;
                M16(child, 0x60) = 0;
                M16(child, 0x62) = 0;
                M32(child, 0x64) = 0x599A;
                if (g_sound_effects_enabled != 0) {
                    sound_queue_push(0xA01232FFu);
                }
                if (M16(ent, 0x64) >= 16) {
                    TASK_S8(task, state) = 2;
                    animation_bind_channel(ent, g_06052394);
                    M32(child, 0x3C) = 2;
                    M16(child, 0x58) = 24;
                    if ((M8(task, 0xF) & 2) && effect_pool_make_room() != 0) {
                        spawn_aimed_child(ent);
                    }
                } else {
                    TASK_S8(task, state) = 3;
                    animation_bind_channel(ent, g_060524fc);
                    M32(ent, 0x78) = (M16(ent, 0x62) < 0) ? 0x2E14 : -0x2E14;
                    M16(ent, 0x62) = 0;
                    M32(child, 0x3C) = 1;
                    M32(ent, 0x3C) |= 32;
                    actor_register_movement_slot(ent, g_06052274[0], g_06052274[1],
                                  g_06052274[2], g_06052274[3]);
                }
            }
        }
        break;
    case 2: {
        s16 anim = animation_sample_absolute(ent);
        M32(ent, 0x78) = M32(ent, 0x14);
        M32(ent, 0x7C) = M32(ent, 0x18);
        M32(ent, 0x80) = M32(ent, 0x1C);
        M16(ent, 0x2A) = 0;
        M16(ent, 0x2C) = (s16)(M16(ent, 0x62) + 0x4000);
        M16(ent, 0x2E) = 0;
        mtx_push_rotate_apply((s32 *)((u8 *)ent + 0x78), (u8 *)ent + 0x2A);
        M32(ent, 0x14) = g_transformed_vector[0];
        M32(ent, 0x18) = g_transformed_vector[1];
        M32(ent, 0x1C) = g_transformed_vector[2];
        actor_resolve_movement(ent, 1);
        if (anim != 0) {
            TASK_S8(task, state) = 6;
            M32(ent, 0x98) = 18;
            release_slot_entry(ent);
        }
        break;
    }
    case 3: {
        int triggered = 0;
        if (M8(ent, 0x48) != 0) {
            M8(ent, 0x48) = 0;
            if (M8(ent, 0x4A) < 8) {
                void *child = MP(ent, 0x9C);
                triggered = 1;
                TASK_S8(task, state) = 2;
                animation_bind_channel(ent, g_060526c4);
                M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x4A)] | g_actor_slot_masks[M8(ent, 0x34)];
                M32(child, 0x3C) = 2;
                M16(child, 0x58) = 30;
                if (g_sound_effects_enabled != 0) {
                    sound_queue_push(0xA01232FFu);
                }
                if (effect_pool_make_room() != 0) {
                    spawn_aimed_child(ent);
                }
            }
        }
        if (!triggered) {
            s16 anim = animation_sample_absolute(ent);
            s32 cf;
            if (anim != 0) {
                M16(ent, 0x28) += (s16)((-M32(ent, 0x78)) >> 3);
                M32(ent, 0x14) = M32(ent, 0x78);
            } else {
                M32(ent, 0x14) += M32(ent, 0x78);
            }
            actor_resolve_movement(ent, 2);
            cf = g_060b88a4;
            if (cf & 12) {
                M32(ent, 0x80) = 0;
            }
            if (cf & 3) {
                TASK_S8(task, state) = 4;
                M32(ent, 0xC) = 0x3333;
                M32(ent, 0x78) = -(M32(ent, 0x78) >> 2);
                M32(ent, 0x3C) &= ~32;
                MP(ent, 0x9C) = 0;
                animation_bind_channel(ent, g_06052604);
            }
        }
        break;
    }
    case 4: {
        s16 anim = animation_sample_absolute(ent);
        if (anim != 0) {
            M32(ent, 0x14) = M32(ent, 0x78);
        } else {
            M32(ent, 0x14) += M32(ent, 0x78);
        }
        M16(ent, 0x28) += (s16)((-M32(ent, 0x78)) >> 3);
        actor_resolve_movement(ent, 1);
        if (anim != 0) {
            TASK_S8(task, state) = 5;
            M32(ent, 0x14) = 0;
            M32(ent, 0x18) = 0;
            M32(ent, 0x1C) = 0;
            M8(ent, 0x48) = 0;
        }
        break;
    }
    case 5:
        actor_resolve_movement(ent, 1);
        if (M8(ent, 0x48) != 0) {
            M8(ent, 0x48) = 0;
            if (M8(ent, 0x4A) < 8) {
                void *child = MP(task_spawn_list_work7c(task, (void *)&effect_follow_parent_task), 8);
                MP(ent, 0x9C) = child;
                M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x4A)];
                M8(child, 0x5F) = 0;
                M16(child, 0x60) = 0;
                M16(child, 0x62) = 0;
                M32(child, 0x64) = 0x599A;
                TASK_S8(task, state) = 2;
                animation_bind_channel(ent, g_060526c4);
                M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x4A)];
                M32(child, 0x3C) = 2;
                M16(child, 0x58) = 30;
                if (g_sound_effects_enabled != 0) {
                    sound_queue_push(0xA01232FFu);
                }
                if ((M8(task, 0xF) & 2) && effect_pool_make_room() != 0) {
                    spawn_aimed_child(ent);
                }
            }
        }
        break;
    case 6:
        M32(ent, 0x98) -= 1;
        if (M32(ent, 0x98) == 0) {
            release_slot_entry(ent);
            if (M8(ent, 0x34) > 8) {
                g_effect_slot_occupied_legacy[M8(ent, 0x34) - 8] = 0;
            }
            TASK_PTR(task, handler) = 0;
        }
        break;
    default:
        break;
    }

    {
        s8 st = TASK_S8(task, state);
        if (st != 6 || (M32(ent, 0x98) & 1) != 0) {
            g_matrix_stack_cursor += 0x30;
            mtx_load_abc();
            mtx_apply_vec_at8(ent);
            mtx_rotate_zyx_at24(ent);
            model_submit_culled(ent, 0);
            g_matrix_stack_cursor -= 0x30;
        }
        if (st != 6) {
            collision_insert_grid(ent);
        }
    }
}
