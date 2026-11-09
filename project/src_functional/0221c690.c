#include "dh_work.h"
/* boss_intro_director [0x0221c690]: Existing map corroborated by source: (u32 task) 9-state timed spawn/aim sequence; spawns handlers 0x237828/0x207bcc/0x219c44; ashl_var-scaled timer; crosshair func_0221af30 (src symbols_band210big.txt) */
/* 0x0221c690 - multi-stage scripted-sequence task handler (r4 = task, ent = task->unk8).
 * A 9-state machine driving a timed actor-spawn/aim sequence. Countdowns live in
 * ent->unk18 (main timer) and ent->unk1C (sub timer); ent->unk30 is the current script
 * cursor and ent->unk3C caches the selected branch node.
 *   0: ent->unk30 = scene_value_select(); process_spawn_list(ent); spawn two companion
 *      tasks (0x6c-rec handler 0x237828, 0x40-rec handler 0x207bcc);
 *      ent->unk18 = (*ent->unk30)->unk4; state++.
 *   1: if (--ent->unk18 >= 0x2C) stay; else ent->unk1C = *0x06098570 * 8; state++.
 *   2: if (ent->unk1C-- was 0) { spawn 0x88-rec handler 0x219c44; state++; }  falls into 3.
 *   3: if (--ent->unk18 == 0) { sel = *0x0609856C & 7; func_0222a708();
 *        (*0x0609882C)->unk8->unk3C = 0x10;
 *        if (sel&1) { *0x06098570 = min(*0x06098570+1,4); func_0221af30(0); }
 *        else *0x06098570 = 0;
 *        node = *(ent->unk30->unk14 + sel*4); ent->unk3C = ent->unk30 = node;
 *        *0x06096C90 = 1; ent->unk8 = 1; task->unkF = 1; task->unk10 = 4; }
 *   4,6: process_spawn_list(ent); ent->unk30 = ent->unk3C;
 *        ent->unk18 = ashl_var(ent->unk3C->unk4, *0x06096C90); task->unk12 = 1; state++.
 *   5: func_0221c8a0(task); if (--ent->unk18 == 0) { func_0222a708();
 *        (*0x0609882C)->unk8->unk3C = 0x10; state++. }
 *   7: func_0221c8a0(task); if (--ent->unk18 == 0) {
 *        (*0x0609882C)->unk8->unk3C = 0x10; ent->unk8 = 1; *0x06096C90 = 0; task->unk10 = 8. }
 *   8: task->unk0 = 0 (deactivate).  other: no-op.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py. bsrf resolved (braf_resolve): scene_value_select
 *   0x22a878, process_spawn_list 0x22a5d0 x2, 0x20fece/0x20ff54/0x20feec constructors,
 *   0x22a708 reset-slots x2, 0x21af30 aim-step, ashl_var 0x24a360; direct bsr 0x21c8a0.
 *   mova+add pointer args (ptr_resolve): handlers 0x237828, 0x207bcc, 0x219c44. State 2
 *   always falls through into state 3 as in the SH-2 (shared .L21c758 tail). */
#include "dh_band21.h"

extern s32   scene_select_player_parameter(void);                        /* scene_value_select */
extern void  process_spawn_list(u32 entity);
extern void *task_spawn_current_work6c(void *parent, void *tmpl);    /* 0x6c-record constructor */
extern void *task_spawn_current_work40(void *parent, void *tmpl);    /* 0x40-record constructor */
extern void *task_spawn_child_work88(void *parent, void *tmpl);    /* 0x88-record constructor */
extern void  actor_mark_slots_for_refresh(void);                        /* reset active actor slots */
extern void  player_update_aim_limits(s32 arg0);                    /* crosshair aim step */
extern s32   shc_shift_left(s32 value, s32 count);        /* ashl_var */
extern void  event_spawn_matching_frame(u32 task);                    /* per-frame sub-step */
extern void  hud_update_player_meters(void);                        /* companion task handler */
extern void  screen_fx_builder_task(void);                        /* companion task handler */
extern void  prompt_tick_random_accept(void);                        /* companion task handler */

void boss_intro_director(u32 task)
{
    u32 ent = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0:
        SEQUENCE_MEM32(ent, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list(ent);
        task_spawn_current_work6c((void *)(uintptr_t)task, (void *)hud_update_player_meters);
        task_spawn_current_work40((void *)(uintptr_t)task, (void *)screen_fx_builder_task);
        SEQUENCE_MEM32(ent, main_ticks) = MEM32(SEQUENCE_MEMU32(ent, spawn_cursor) + 4);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 1: {
        s32 v = SEQUENCE_MEM32(ent, main_ticks) - 1;
        SEQUENCE_MEM32(ent, main_ticks) = v;
        if (v >= 0x2c)
            break;
        SEQUENCE_MEM32(ent, phase_ticks) = MEM32(0x06098570) * 8;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    }

    case 2: {
        s32 c = SEQUENCE_MEM32(ent, phase_ticks) - 1;
        SEQUENCE_MEM32(ent, phase_ticks) = c;
        if (c + 1 == 0) {
            task_spawn_child_work88((void *)(uintptr_t)task, (void *)prompt_tick_random_accept);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
    }
    /* fallthrough */
    case 3: {
        s32 t = SEQUENCE_MEM32(ent, main_ticks) - 1;
        SEQUENCE_MEM32(ent, main_ticks) = t;
        if (t == 0) {
            s32 sel = MEM32(0x0609856c) & 7;
            u32 node;
            actor_mark_slots_for_refresh();
            MEM32(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0x3c) = 0x10;
            if (sel & 1) {
                s32 n = MEM32(0x06098570) + 1;
                MEM32(0x06098570) = n;
                if (n > 4)
                    MEM32(0x06098570) = 4;
                player_update_aim_limits(0);
            } else {
                MEM32(0x06098570) = 0;
            }
            node = MEMU32(MEMU32(SEQUENCE_MEMU32(ent, spawn_cursor) + 0x14) + sel * 4);
            SEQUENCE_MEM32(ent, branch_cursor) = node;
            SEQUENCE_MEM32(ent, spawn_cursor) = node;
            MEM32(0x06096c90) = 1;
            MEM32(ent + 0x08) = 1;
            MEM8(task + 15) = 1;
            MEM8(task + 16) = 4;
        }
        break;
    }

    case 4:
    case 6:
        process_spawn_list(ent);
        SEQUENCE_MEM32(ent, spawn_cursor) = SEQUENCE_MEM32(ent, branch_cursor);
        SEQUENCE_MEM32(ent, main_ticks) = shc_shift_left(MEM32(SEQUENCE_MEMU32(ent, branch_cursor) + 4), MEM32(0x06096c90));
        MEM8(task + 18) = 1;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 5: {
        s32 t;
        event_spawn_matching_frame(task);
        t = SEQUENCE_MEM32(ent, main_ticks) - 1;
        SEQUENCE_MEM32(ent, main_ticks) = t;
        if (t == 0) {
            actor_mark_slots_for_refresh();
            MEM32(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0x3c) = 0x10;
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    }

    case 7: {
        s32 t;
        event_spawn_matching_frame(task);
        t = SEQUENCE_MEM32(ent, main_ticks) - 1;
        SEQUENCE_MEM32(ent, main_ticks) = t;
        if (t == 0) {
            MEM32(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0x3c) = 0x10;
            MEM32(ent + 0x08) = 1;
            MEM32(0x06096c90) = 0;
            MEM8(task + 16) = 8;
        }
        break;
    }

    case 8:
        MEM32(task + 0) = 0;
        break;

    default:
        break;
    }
}
