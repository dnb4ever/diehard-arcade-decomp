/* enemy_tick_launched_body [0x022424c4]: Copies thrower position, seeds height and flight state then integrates/draws. */
#include "dh_work.h"
#include "dh.h"
/* 0x022424c4  in-flight handler for a thrown/launched actor (installed by
 * func_02242410 via func_0220ff0a). arg0 = task wrapper, inner = MP(task,8) is
 * the actor, src = MP(MP(task,0x14),8) is the thrower/anchor actor.
 * State machine on M8(task,0x10):
 *   0 spawn: copy src position into inner (y gets +0x10000), set unkA8=0x10000,
 *     clear unkC0/BC, 0xB4-frame timer, grab collision slot (func_02228106),
 *     unk38=0x100000, advance; fall through to spatial_grid_insert.
 *   1 fly: tick timer; while >0, on impact (unk48) clear it, flag the active
 *     anchor slot's actor (g_060995b0[g_06097414]->unk8->unk48=1) and, if sound
 *     is enabled (g_060e0ddc), push cmd 0xA01405FF; then grid-insert. On expiry
 *     release the timer slot g_060996d4[unk34-8] (if unk34>8) and clear the task
 *     handler (task->unk0 = 0).
 *   default (incl. 2): spatial_grid_insert only.
 *
 * bounds: [0x2424c4, 0x2425d0); split out of func_02242272's over-swallowed
 *   extent (the rts tail at 0x2425cc; 0x2425d0 begins func_022425d0).
 * fidelity: HIGH - verified vs m2c/--asm; func_02228106 no args, sound_queue_push
 *   takes the command only (r5/r6 at the call site are dead). */
extern s8   g_06097414;        /* current-actor playerIdx latch */
extern s32  g_sound_effects_enabled;        /* sound-enable gate */
extern u8   g_effect_slot_occupied_legacy[];      /* per-actor timer-slot flags */
extern s32  g_actor_slot_ids[];      /* per-slot active flag */
extern void *g_actor_slot_tasks_legacy[];     /* per-slot anchor pointers; deref +8 -> actor */

s32  actor_allocate_effect_slot(void);                        /* 0x02228106 collision-slot grab */
void sound_queue_push(s32 cmd);                     /* 0x02215f6c sound_queue_push */
void collision_insert_grid(void *actor);                 /* 0x0222eee4 spatial_grid_insert */

void enemy_tick_launched_body(void *task) {
    void *inner = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);

    switch (TASK_S8(task, state)) {
    case 0:
        M32(inner, 0x08) = M32(src, 0x08);
        M32(inner, 0x0C) = M32(src, 0x0C) + 0x10000;
        M32(inner, 0x10) = M32(src, 0x10);
        M32(inner, 0xA8) = 0x10000;
        M32(inner, 0xC0) = 0;
        M32(inner, 0xBC) = 0;
        M32(inner, 0xC4) = 0xB4;
        M8(inner, 0x34) = (s8)actor_allocate_effect_slot();
        M32(inner, 0x38) = 0x100000;
        TASK_S8(task, state) += 1;
        /* fallthrough */
    default:
        collision_insert_grid(inner);
        return;
    case 1: {
        s32 t = M32(inner, 0xC4) - 1;
        M32(inner, 0xC4) = t;
        if ((t + 1) > 0) {
            if (M8(inner, 0x48) != 0) {
                s8 slot = g_06097414;
                M8(inner, 0x48) = 0;
                if (g_actor_slot_ids[slot] != 0) {
                    M8(TASK_PTR(g_actor_slot_tasks_legacy[slot], work), 0x48) = 1;
                }
                if (g_sound_effects_enabled != 0) {
                    sound_queue_push(0xA01405FF);
                }
            }
            collision_insert_grid(inner);
            return;
        }
        {
            s8 n = M8(inner, 0x34);
            if (n > 8) {
                g_effect_slot_occupied_legacy[n - 8] = 0;
            }
        }
        TASK_PTR(task, handler) = (void *)0;
        return;
    }
    }
}
