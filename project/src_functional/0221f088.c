#include "dh_work.h"
/* evt_sequence_step [0x0221f088]: Existing map corroborated by source: (u32 task) 5-state scripted-event: scene_value_select->unk30, process_spawn_list, func_0222a708 reset, *0x06098580|=0x60, slots 3..6 cleared (src symbols_band210big.txt) */
/* 0x0221f088 - scripted-event sequence step (task handler, r4 = task).
 * ent = task->unk8. A gate value is read each frame:
 *   cond = (*(u32*)0x0609882C)->unk8->unkAE  (s16), compared == 1.
 * State machine at task->unk10:
 *   0: ent->unk30 = scene_value_select(); process_spawn_list(ent); state++.
 *   1: if (cond == 1) { func_0222a708() [reset active actor slots]; state++. }
 *   2: process_spawn_list(ent); *0x06098580 |= 0x60; state++.
 *   3: if (cond == 1) state++.
 *   4: if (cond == 1) { for i in 3..6: if g_06099590[i] != 0
 *        (*g_060995B0[i])->unk8->unk3C = 0x10; state++. }
 *   other: no-op.
 * The case-4 loop is the func_0222a708 body (k=1..6) inlined for indices 3..6.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py. bsrf resolved (braf_resolve):
 *   0x22a878 scene_value_select(void)->s32, 0x22a5d0 process_spawn_list(ent) x2,
 *   0x22a708 func_0222a708(void). The unrolled x4 loop (r14 3->7, step 4) runs one
 *   pass over slots 3..6; r4/r5 index the flag/pointer arrays in lockstep (step 4). */
#include "dh_band21.h"

extern s32  scene_select_player_parameter(void);        /* scene_value_select */
extern void process_spawn_list(u32 entity);
extern void actor_mark_slots_for_refresh(void);        /* reset active actor slots */

void evt_sequence_step(u32 task)
{
    u32 ent = MEMU32(task + 8);
    s16 cond = MEM16(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0xae);
    s32 i;

    switch (MEM8(task + 16)) {
    case 0:
        SEQUENCE_MEM32(ent, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list(ent);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    case 1:
        if (cond == 1) {
            actor_mark_slots_for_refresh();
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 2:
        process_spawn_list(ent);
        MEM32(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x60;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    case 3:
        if (cond == 1)
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    case 4:
        if (cond == 1) {
            for (i = 3; i <= 6; i++) {
                if (MEM32(DH_ACTOR_SLOT_IDS_ADDR + i * 4) != 0)
                    MEM32(MEMU32(MEMU32(DH_ACTOR_SLOT_TASKS_ADDR + i * 4) + 8) + 0x3c) = 0x10;
            }
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    default:
        break;
    }
}
