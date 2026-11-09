/* scene_tick_selected_enemy_gate [0x0221d59c]: View+ae selection and live slots 3..6 gate spawn-list progression. */
#include "dh_work.h"
#include "dh.h"

/* 0x0221d59c - scripted-event actor step (task FSM), variant selecting via ctx->unkAE.
 * task->unk10 = state index; ent = task->unk8; ctx = (*0x0609882c)->unk8; sel = (s16)ctx->unkAE.
 *   state 0: ent->unk30 = func_0222a878(); process_spawn_list(ent); ent->unk18 = ent->unk30->unk4;
 *            scan i=3..6: if M32(0x06099584+12+i*4)!=0 and
 *            (s16)@(94, M32(M32(0x06099584+44+i*4)+8)) == 41 then
 *            *0x060973F8 = that pointer and stop; state++.
 *   state 1: if sel==1 { process_spawn_list(ent); state++; }
 *   state 2: if sel==30 *0x06098580 |= 1; if sel==1 { *0x06098580 |= 0x10; state++; }
 *   state 3: if sel==1 { func_0222a708(); state++; }
 *   state 4: process_spawn_list(ent); func_02233388(task, 0x06074C54); state++.
 * fidelity: HIGH - bsrf targets resolved (0x22a878, 0x22a5d0, 0x22a708, 0x233388). */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(u32 entity);
extern void actor_mark_slots_for_refresh(void);
extern void scene_spawn_record_list(u32 parent, u32 record);

void scene_tick_selected_enemy_gate(u32 task)
{
    u32 ent = MU32(task, TASK_WORK);
    u32 ctx = MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8);
    s16 sel = (s16)M16(ctx, 0xae);

    switch ((s8)TASK_S8(task, state)) {
    case 0: {
        u32 list = (u32)scene_select_player_parameter();
        s32 i;
        SEQUENCE_M32(ent, spawn_cursor) = list;
        process_spawn_list(ent);
        SEQUENCE_M32(ent, main_ticks) = M32(SEQUENCE_M32(ent, spawn_cursor), 4);
        for (i = 3; i <= 6; i++) {
            if (M32(DH_ACTOR_SLOT_REGISTRY_ADDR + 12 + i * 4, 0) != 0) {
                u32 cand = MU32(MU32(DH_ACTOR_SLOT_REGISTRY_ADDR + 44 + i * 4, 0), 8);
                if ((s16)M16(cand, 94) == 41) {
                    M32(0x060973f8, 0) = cand;
                    break;
                }
            }
        }
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    }
    case 1:
        if (sel == 1) {
            process_spawn_list(ent);
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    case 2:
        if (sel == 30)
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
        if ((s16)M16(ctx, 0xae) == 1) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 0x10;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    case 3:
        if (sel == 1) {
            actor_mark_slots_for_refresh();
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    case 4:
        process_spawn_list(ent);
        scene_spawn_record_list(task, 0x06074c54);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    default:
        break;
    }
}
