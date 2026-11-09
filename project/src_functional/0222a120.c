#include "dh_work.h"
#include "dh.h"
/* 0x022a120 (actor_spawn_list_tick) - task handler that drives a timed/paced actor spawn list
 * (task=r4, entity=task->unk8). A small state machine on task->unk16:
 *   state 0: pick the spawn-list cursor for the current scene. entity->unk44 counts the active
 *            join players (g_06098674[0]/[1]==2, each also pokes func_0220ac5a(scene->unk28)); if
 *            entity->unk48 != 0 it is forced to 2. The cursor is a 12-byte-strided row
 *            scene->unk1C[(g_0609856C & 7)] whose slot (+0/+4/+8) is chosen by unk44 and the join
 *            byte; entity->unk64 = cursor, entity->unk40 = cursor->unk4 & 0xFFFF, entity->unk68=0;
 *            task->unk16 -> 1 (falls through into state 1).
 *   state 1: paced spawn loop. If the last spawned obj (entity->unk68) has finished
 *            (unk3C==16) the pacing counter entity->unk40 is reset to 0. While entity->unk40==0,
 *            spawn one actor from the cursor (type 0 -> func_02227b60(&func_0222a300) dynamic-id;
 *            else func_02227bc4(type-1) fixed-id), copy the descriptor fields into the entity,
 *            seed its HP word unk56 (from g_06099794<<1 if g_0609979A!=0, else g_06099788[unk34-1]),
 *            run func_0222ca54(obj), advance the 24-byte cursor; when the next cursor->unk1==0 the
 *            list is done (task->unk16 -> 3), else refresh entity->unk64/unk40 from it. On exit
 *            entity->unk40 is decremented (the inter-spawn frame delay).
 *   state 3: task->unk0 = 0 (terminate the driver task).
 * fidelity: HIGH - spawn fps resolved (ptr_resolve: bsrf 0x227bc4/0x227b60, jsr r8=0x22ca54,
 * bsrf func_0220ac5a 0x20ac5a); descriptor arg &func_0222a300 resolved from PIC mova anchors.
 * unk16/44? note: unk44 is a long here (mov.l). unk40/44/64/68/D0/8/C/10/3C long, unk34/4F/1 byte,
 * unk5E/56 word; g_06099788/94/9A s16, g_06098674 bytes, g_0609856C/g_0609866C longs. */
extern void *actor_spawn_dynamic_slot(void *state_handler);
extern void *actor_spawn_fixed_slot(s32 fixed_idx);
extern void  actor_load_part_ids(void *obj);
extern s32   resident_increment_counter104(s32 idx);
extern void  actor_spawn_arm_task(void *task);        /* actor arm-state handler */
extern void *g_scene_descriptor;      /* -> scene descriptor */
extern s32   g_0609856C;      /* scene-row selector */
extern u8    g_player_join_states[];    /* per-player join bytes */
extern s16   g_actor_pose_set;
extern s16   g_06099794;
extern s16   g_06099788[];    /* per-slot HP seed words */

void actor_spawn_list_tick(void *task) {
    void *entity = TASK_PTR(task, work);
    void *cursor;
    void *obj;

    switch (TASK_S8(task, state)) {
    case 0: {
        void *scene = g_scene_descriptor;
        void *tbl;
        s32 off;

        if (M32(entity, 0x48) != 0) {
            M32(entity, 0x44) = 2;
        } else {
            M32(entity, 0x44) = 0;
            if (g_player_join_states[0] == 2) {
                M32(entity, 0x44) += 1;
                resident_increment_counter104((s16)SCENE_M16(scene, pose_index));
            }
            if (g_player_join_states[1] == 2) {
                M32(entity, 0x44) += 1;
                resident_increment_counter104((s16)SCENE_M16(scene, pose_index));
            }
        }
        off = (g_0609856C & 7) * 12;
        tbl = MP(scene, 0x1C);
        if (M32(entity, 0x44) == 1) {
            if (g_player_join_states[0] == 2)
                cursor = MP((u8 *)tbl + off, 0);
            else
                cursor = MP((u8 *)tbl + off, 4);
        } else {
            cursor = MP((u8 *)tbl + off, 8);
        }
        MP(entity, 0x64) = cursor;
        M32(entity, 0x40) = M32(cursor, 4) & 0xFFFF;
        MP(entity, 0x68) = (void *)0;
        TASK_S8(task, state) += 1;
    }
    /* fallthrough */
    case 1: {
        void *last = MP(entity, 0x68);
        if (last != (void *)0 && M32(last, 0x3C) == 16)
            M32(entity, 0x40) = 0;

        while (M32(entity, 0x40) == 0) {
            void *res;
            s8 type;

            cursor = MP(entity, 0x64);
            type = M8(cursor, 0);
            if (type != 0)
                res = actor_spawn_fixed_slot(type - 1);
            else
                res = actor_spawn_dynamic_slot((void *)actor_spawn_arm_task);

            obj = MP(res, 8);
            MP(entity, 0x68) = obj;
            M32(obj, 0xD0) = M32(cursor, 0x14);
            M32(obj, 0x8)  = M32(cursor, 0x8);
            M32(obj, 0xC)  = M32(cursor, 0xC);
            M32(obj, 0x10) = M32(cursor, 0x10);
            M16(obj, 0x5E) = (s8)M8(cursor, 1);
            if (g_actor_pose_set != 0)
                M16(obj, 0x56) = (s16)(g_06099794 << 1);
            else
                M16(obj, 0x56) = g_06099788[(s8)M8(obj, 0x34) - 1];
            M8(obj, 0x4F) = (s8)M8(cursor, 2);
            actor_load_part_ids(obj);

            cursor = (u8 *)cursor + 24;
            if ((s8)M8(cursor, 1) == 0) {
                TASK_S8(task, state) = 3;
                break;
            }
            MP(entity, 0x64) = cursor;
            M32(entity, 0x40) = M32(cursor, 4) & 0xFFFF;
        }
        M32(entity, 0x40) = M32(entity, 0x40) - 1;
        break;
    }
    case 3:
        TASK_PTR(task, handler) = (void *)0;
        break;
    default:
        break;
    }
}
