/* scene_tick_view_selected_branch [0x0221da60]: View task then work+ae selects branch; retained round-4 pointer-order correction. */
/* Round 4: ROM 06021a6c loads the view task pointer before its +8 work pointer. Restore the dereference order. Evidence: work/decomp4/pointer_bugs.asm. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221da60 - state machine on task state byte (task+0x10), work record w = task->unk8.
 * sel = (s16)(*0x0609882c)->unk8->unkAE (current selection index).
 *   0: w->unk30 = func_0222a878(); func_0222a5d0(w); w->unk18 = w->unk30->unk4;
 *      scan slots i=0..3: base 0x06099584; if MU32(base, 24+4*i) != 0 and
 *      (s16)M16(MU32(base, 56+4*i) + 8, 0x5E) == 7, store that record into
 *      0x060973F8 and stop; advance state.
 *   1: if (sel == 1) advance state.
 *   2: if (sel == 1) { func_0222a708(); advance state; }
 *   3: func_0222a5d0(w); *(0x06098580) |= 2; advance state.
 *   4/default: nothing.
 * fidelity: HIGH - bsrf targets resolved (0x22a878, 0x22a5d0, 0x22a708); slot scan
 *   offsets from add #12/add #44 with r4=r5=12 base (r2=base+12+r5, r6=base+44+r4);
 *   loop runs 4 iterations (counter 3..7, continue while <= 6). */
extern s32 scene_select_player_parameter(void);
extern void process_spawn_list_entry(u32 w);
extern u32  actor_mark_slots_for_refresh(void);

void scene_tick_view_selected_branch(u32 task)
{
    u32 w = MU32(task, TASK_WORK);
    s16 sel = M16(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0xae);

    switch ((s8)TASK_S8(task, state)) {
    case 0: {
        u32 i;
        SEQUENCE_M32(w, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list_entry(w);
        SEQUENCE_M32(w, main_ticks) = M32(SEQUENCE_MU32(w, spawn_cursor), 4);
        for (i = 0; i < 4; i++) {
            if (MU32(DH_ACTOR_SLOT_REGISTRY_ADDR, 24 + i * 4) != 0) {
                u32 rec = MU32(MU32(DH_ACTOR_SLOT_REGISTRY_ADDR, 56 + i * 4), 8);
                if ((s16)M16(rec, 0x5e) == 7) {
                    M32(0x060973f8, 0) = rec;
                    break;
                }
            }
        }
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    }

    case 1:
        if (sel == 1)
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;

    case 2:
        if (sel == 1) {
            actor_mark_slots_for_refresh();
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;

    case 3:
        process_spawn_list_entry(w);
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 2;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;

    default:
        break;
    }
}
