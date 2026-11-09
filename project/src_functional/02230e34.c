/* effect_tick_explosion_debris [0x02230e34]: Two-state burst spawner optionally emits 0xa01403ff and registers debris. */
#include "dh_work.h"
#include "dh.h"
/* 0x02230e34 - explosion / debris spawner task (2-state machine + list tail).
 * task->unk8 = entity; task->unk10 = state byte.
 *   state 0: (optionally) kick a sound (sound_queue_push(0xA01403FF)) when the
 *            global trigger *0x060E0DDC is set; seed the entity's own motion fields
 *            (unk58=0, unk5D=0, unk68=g_0604ebe4[unk34], unk64=0x20000, unk62=0x53,
 *            unk60=0x14, unk38=0x40000, unkC=0x10000); spawn an obj_expand_draw4
 *            child (spawn_obj_a, handler 0x0223115c) seeded with the entity's
 *            position; advance state (fallthrough to state 1).
 *   state 1: if unk5D is set, propagate to the paired actor (func_0222f3c0);
 *            advance a per-frame counter unk58; once it exceeds 2, spawn a 0x7c-byte
 *            scatter child (spawn_obj_0x7c, handler 0x02230f74) copying position and
 *            the unk4C/50/54 block, seed its fields, then deactivate this task.
 *            Otherwise fall through to the list tail.
 *   other states: fall straight to the tail.
 *   tail: prepend the entity to the per-frame drawlist (func_0222eea2).
 * fidelity: HIGH - verified vs m2c_fn.py --asm; all bsrf/braf targets resolved by
 *   hand (0x02215f6c, 0x0220feec, 0x0222f3c0, 0x0220ff28, tail braf 0x0222eea2) and
 *   both spawn-handler pointers (mova+offset -> 0x0223115c, 0x02230f74). */
extern void  sound_queue_push_entry(s32 request);           /* 0x02215f6c sound_queue_push */
extern void *task_spawn_child_work88(void *task, void *handler); /* 0x0220feec spawn_obj_a */
extern void *task_spawn_list_work7c(void *task, void *handler); /* 0x0220ff28 spawn_obj_0x7c */
extern void  actor_publish_linked_slot_value(void *ent);              /* 0x0222f3c0 actor_pair_propagate */
extern void  collision_queue_actor(void *ent);              /* 0x0222eea2 drawlist_prepend */
extern void  obj_expand_draw4(void *actor);            /* 0x0223115c obj_expand_draw4 (handler) */
extern void  effect_scatter_task(void *task);             /* 0x02230f74 effect_scatter_task (handler) */

extern s32 g_sound_effects_enabled;     /* global sound trigger */
extern s32 g_actor_slot_masks[];   /* per-type parameter table (keyed by unk34) */

void effect_tick_explosion_debris(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);
    void *child;

    switch (state) {
    case 0:
        if (g_sound_effects_enabled != 0)
            sound_queue_push_entry(DH_SOUND_EXPLOSION_BURST_COMMAND);
        M16(ent, 0x58) = 0;
        M8(ent, 0x5D) = 0;
        M32(ent, 0x68) = g_actor_slot_masks[M8(ent, 0x34)];
        M32(ent, 0x64) = 0x20000;
        M16(ent, 0x62) = 0x53;
        M16(ent, 0x60) = 0x14;
        M32(ent, 0x38) = 0x40000;
        M32(ent, 0xC)  = 0x10000;
        child = MP(task_spawn_child_work88(task, (void *)obj_expand_draw4), 8);
        M32(child, 0x8)  = M32(ent, 0x8);
        M32(child, 0xC)  = M32(ent, 0xC);
        M32(child, 0x10) = M32(ent, 0x10);
        TASK_S8(task, state)++;
        /* fallthrough */
    case 1:
        if (M8(ent, 0x5D) != 0)
            actor_publish_linked_slot_value(ent);
        M16(ent, 0x58)++;
        if ((s16)(M16(ent, 0x58) - 1) > 1) {
            child = MP(task_spawn_list_work7c(task, (void *)effect_scatter_task), 8);
            M32(child, 0x8)  = M32(ent, 0x8);
            M32(child, 0xC)  = M32(ent, 0xC);
            M32(child, 0x10) = M32(ent, 0x10);
            M32(child, 0x4C) = M32(ent, 0x4C);
            M32(child, 0x50) = M32(ent, 0x50);
            M32(child, 0x54) = M32(ent, 0x54);
            M32(child, 0x68) = g_actor_slot_masks[M8(ent, 0x34)];
            M16(child, 0x60) = 0x1E;
            M16(child, 0x62) = 0x14;
            M8(child, 0x34)  = M8(ent, 0x34);
            M32(child, 0x38) = 0x40000;
            M32(child, 0x64) = 0xA000;
            M32(child, 0x70) = 0xA0000;
            TASK_PTR(task, handler) = (void *)0;
            return;
        }
        break;
    default:
        break;
    }
    collision_queue_actor(ent);
}
