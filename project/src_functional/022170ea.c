/* actor_resolve_movement [0x022170ea]: Chooses movement collision policy, resolves the sweep and clears per-frame displacements. */
#include "dh_work.h"
#include "dh.h"
/* 0x022170ea  entity_move_dispatch: per-entity movement/collision step.
 *
 * entity->unk34 (s8) selects a 0x48-stride record in g_060B6D9C (same table as
 * func_0221707c). If record->unk4 == 0 the entity is inactive -> return 0; if
 * record->unk0 != 0 a pending-release hook func_02217d22 is run first.
 *
 * The global g_060B889C is a "current mover" callback pointer that the sweep
 * core func_0221746c invokes; this function swaps it to the handler matching the
 * entity's movement flags (entity->unk84) around each sweep call and restores the
 * default mover func_02217888 afterwards. func_02217c10 is the post-move resolve,
 * run whenever the status flag g_060B88A4 has bit31 set.
 *
 * record->unk4 >= 8  -> script path, dispatched on the `mode` argument (0..3).
 * record->unk4 <  8  -> flag path driven by entity->unk84 / unk88:
 *   &0x01803000 : push-type move (handler 2179b0 if &0x800000 or unk88!=8, else 217ae4)
 *   &0x0E0000   : parented move via a g_060995B0[idx] object (idx = unk4B or unk4C),
 *                 applying the entity's position delta to that object; g_060B88A0
 *                 holds the active parent index (-1 when idle).
 *   else &0x80000000 : plain velocity integrate + resolve; otherwise plain sweep.
 * In the <8 path the velocity (unk14/18/1C) is cleared after the step.
 *
 * fidelity: HIGH. All 12 mova+add callback pointers resolved (ptr_resolve):
 * 217888/2179b0/217ae4. func_0221746c consumes only the record pointer (the stale
 * r5/r6 live across its calls are the velocity/flag temporaries, not its args).
 * The switch default (mode not 0..3) leaves the return value unset in the ROM; it
 * is unreachable for valid callers and modelled as NULL. */
extern u8    g_actor_movement_records[];                 /* 0x48-stride entity records          */
extern void *g_actor_slot_tasks[];                 /* parent-object pointer array         */

extern void *collision_resolve_sweep(void *record);  /* collision/clip sweep core          */
extern void  collision_grid_insert_live(void *record);  /* post-move resolve                   */
extern void  grid_remove_node(void *record);  /* pending-release hook                */
extern void *grid_find_overlap_any(void *a, void *b, s16 dir); /* default mover callback  */
extern void *grid_find_overlap_static(void *a, void *b, s16 dir); /* push mover callback      */
extern void *grid_find_overlap_type_zero(void *a, void *b, s16 dir); /* slide mover callback     */

#define G_MOVER  MP(0x060B889C, 0)         /* current mover callback pointer      */
#define G_PARENT M32(0x060B88A0, 0)        /* active parent index (-1 = none)     */
#define G_STATUS M32(0x060B88A4, 0)        /* status flags (bit31 = needs resolve)*/

void *actor_resolve_movement(void *entity, s32 mode) {
    s32   slot   = ACTOR_WORK(entity)->slot_id;
    void *record = g_actor_movement_records + (s16)(slot * 0x48);
    void *ret    = (void *)0;

    if (M32(record, 4) == 0) {
        return (void *)0;
    }
    if (M32(record, 0) != 0) {
        grid_remove_node(record);
    }

    if (M32(record, 4) >= 8) {
        switch (mode) {
        case 0:
            ret = collision_resolve_sweep(record);
            break;
        case 1:
            G_MOVER = (void *)grid_find_overlap_static;
            ret = collision_resolve_sweep(record);
            G_MOVER = (void *)grid_find_overlap_any;
            break;
        case 2:
            G_MOVER = (void *)grid_find_overlap_type_zero;
            ret = collision_resolve_sweep(record);
            G_MOVER = (void *)grid_find_overlap_any;
            break;
        case 3:
            ret = (void *)0;
            ACTOR_WORK(entity)->position_x += ACTOR_WORK(entity)->movement_x;
            ACTOR_WORK(entity)->position_y += ACTOR_WORK(entity)->movement_y;
            ACTOR_WORK(entity)->position_z += ACTOR_WORK(entity)->movement_z;
            G_STATUS = 0x80000000;
            break;
        }
        if (G_STATUS & 0x80000000) {
            collision_grid_insert_live(record);
        }
    } else {
        u32 flags = (u32)ACTOR_WORK(entity)->action_category;
        s32 current_action_state = ACTOR_WORK(entity)->action_state;

        if (flags & 0x01803000) {
            if ((flags & 0x800000) || (current_action_state != 8)) {
                G_MOVER = (void *)grid_find_overlap_static;
                ret = collision_resolve_sweep(record);
                G_MOVER = (void *)grid_find_overlap_any;
            } else {
                G_MOVER = (void *)grid_find_overlap_type_zero;
                ret = collision_resolve_sweep(record);
                G_MOVER = (void *)grid_find_overlap_any;
            }
        } else if (!(flags & 0x0E0000)) {
            if (flags & 0x80000000) {
                ACTOR_WORK(entity)->position_x += ACTOR_WORK(entity)->movement_x;
                ACTOR_WORK(entity)->position_y += ACTOR_WORK(entity)->movement_y;
                ACTOR_WORK(entity)->position_z += ACTOR_WORK(entity)->movement_z;
                G_STATUS = 0;
                collision_grid_insert_live(record);
                ret = (void *)0;
            } else {
                ret = collision_resolve_sweep(record);
                if (G_STATUS & 0x80000000) {
                    collision_grid_insert_live(record);
                }
            }
        } else {
            if (flags & 0x20000) {
                s32   idx = ACTOR_M8(entity, paired_slot);
                void *obj = TASK_PTR(g_actor_slot_tasks[idx], work);
                s32   px, py, pz;

                G_PARENT = idx;
                px = ACTOR_WORK(entity)->position_x + ACTOR_WORK(entity)->movement_x;
                py = ACTOR_WORK(entity)->position_y + ACTOR_WORK(entity)->movement_y;
                pz = ACTOR_WORK(entity)->position_z + ACTOR_WORK(entity)->movement_z;
                ret = collision_resolve_sweep(record);
                ACTOR_WORK(obj)->movement_x += ACTOR_WORK(entity)->position_x - px;
                ACTOR_WORK(obj)->movement_y += ACTOR_WORK(entity)->position_y - py;
                ACTOR_WORK(obj)->movement_z += ACTOR_WORK(entity)->position_z - pz;
                if (G_STATUS & 0x80000000) {
                    collision_grid_insert_live(record);
                }
                G_PARENT = -1;
            } else if (flags & 0x40000) {
                s32   idx = ACTOR_M8(entity, hit_pending);
                void *obj = TASK_PTR(g_actor_slot_tasks[idx], work);
                s32   px, py, pz;

                G_PARENT = idx;
                px = ACTOR_WORK(entity)->position_x + ACTOR_WORK(entity)->movement_x;
                py = ACTOR_WORK(entity)->position_y + ACTOR_WORK(entity)->movement_y;
                pz = ACTOR_WORK(entity)->position_z + ACTOR_WORK(entity)->movement_z;
                G_MOVER = (void *)grid_find_overlap_type_zero;
                ret = collision_resolve_sweep(record);
                G_MOVER = (void *)grid_find_overlap_any;
                ACTOR_WORK(obj)->movement_x += ACTOR_WORK(entity)->position_x - px;
                ACTOR_WORK(obj)->movement_y += ACTOR_WORK(entity)->position_y - py;
                ACTOR_WORK(obj)->movement_z += ACTOR_WORK(entity)->position_z - pz;
                if (G_STATUS & 0x80000000) {
                    collision_grid_insert_live(record);
                }
                G_PARENT = -1;
            } else {
                G_MOVER = (void *)grid_find_overlap_type_zero;
                ret = collision_resolve_sweep(record);
                G_MOVER = (void *)grid_find_overlap_any;
            }
        }

        ACTOR_WORK(entity)->movement_z = 0;
        ACTOR_WORK(entity)->movement_y = 0;
        ACTOR_WORK(entity)->movement_x = 0;
    }

    return ret;
}
