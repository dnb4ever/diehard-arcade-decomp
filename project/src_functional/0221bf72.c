#include "dh_work.h"
/* setpiece_director [0x0221bf72]: Existing map corroborated by source: (u32 task) 9-state encounter director; spawns 0x207bcc/0x219f26/0x237828/0x238fdc/0x21c2aa; per-slot motion deltas (slots 1..2); SFX via r10 fn-ptr (src symbols_band210big.txt) */
/* 0x0221bf72 - boss/set-piece sequence director (task handler, r4 = task, ent = task->unk8).
 * A 9-state machine stepping a scripted encounter: spawns helper tasks, runs timed phases
 * off ent->unk18 (main countdown) / ent->unk1C (sub countdown), fires SFX, and on the big
 * phase advances the script node and applies per-slot motion deltas. ent->unk30 is the
 * current script node, ent->unk3C the cached branch node.
 *   0: *0x06098580 |= 1; ent->unk30 = scene_value_select(); process_spawn_list(ent);
 *      ent->unk18 = ent->unk30->unk4; func_0220ff54(task, h207bcc); state++.
 *   1,3: if (--ent->unk18 == 0) { (*0x0609882C)->unk8->unk3C = 0x10; func_0222a708(); state++. }
 *   2: process_spawn_list(ent); ent->unk18 = ent->unk30->unk4; state++.
 *   4: process_spawn_list(ent); ent->unk18 = ent->unk30->unk4; func_02216018(2);
 *      ent->unk1C = *0x06098570 * 8 + 0x46; state++.
 *   5: if (ent->unk18 < 0x5C) spawn a projectile obj (via *0x06099580, h21c2aa) seeded
 *      (unk14=0x8580, unk8=0x600000, unkC=0, unk10=0x400000, unk24=0, unk26=0x8000,
 *      unk28=0); state++. Then fall into 6.
 *   6: if (--ent->unk1C wrapped from 0) spawn func_02219f26 + h237828. If ent->unk18==0x32
 *      && sound-on, SFX 0xA01400FF. If ent->unk18==0x11, spawn h238fdc via *0x06099580.
 *      If (--ent->unk18 == 0): (*0x0609882C)->unk8->unk3C=0x10; func_0222a708();
 *      sel = *0x0609856C & 1; if (sel) *0x06098570=min(+1,4) else { func_0221af30(0);
 *      *0x06098570=0; } idx = sel | (*0x06098568*2); node = *(ent->unk30->field2C + idx*4);
 *      ent->unk3C = ent->unk30 = node; ent->unk8 = 1; task->unkF = 1; state++.
 *   7: process_spawn_list(ent); ent->unk18 = ent->unk30->unk4; ent->unk1C = 3; state++.
 *   8: if (--ent->unk18 == 0) { (*0x0609882C)->unk8->unk3C=0x10; func_0222a708(); state++. }
 *      else if ((*0x0609856C & 1)==0): if (--ent->unk1C == 0 && sound-on) SFX 0xA01205FF;
 *      then for slot k in 1..2 with g_06099590[k]!=0: entity e = g_060995B0[k]->unk8;
 *      e->unk8/unkC/unk10 += delta_record[k] (base 0x06096C04, stride 12); if (ent->unk1C==0
 *      && sound-on) SFX = (g_06053F04[(u8)row[1]] << 8) | 0xA00000FF, row via g_060540E8.
 *   other: no-op.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py/m2c. bsrf resolved (braf_resolve): scene_value_select
 *   0x22a878, process_spawn_list 0x22a5d0 (x4), constructors 0x20ff54/0x20fece/0x20feec,
 *   func_0222a708 (x4), func_02216018, func_0221af30, sound_queue_push 0x215f6c (indirect via
 *   r10 fn-ptr, always 1 arg). mova+add handler/pointer args (ptr_resolve): h207bcc 0x207bcc,
 *   h21c2aa 0x21c2aa, func_02219f26, h237828 0x237828, h238fdc 0x238fdc. States 5->6 fall
 *   through; the r10 fn-pointer is sound_queue_push (m2c mis-rendered a 2nd arg). */
#include "dh_band21.h"

extern s32   scene_select_player_parameter(void);                        /* scene_value_select */
extern void  process_spawn_list(u32 entity);
extern void  actor_mark_slots_for_refresh(void);                        /* reset active actor slots */
extern void  player_update_aim_limits(s32 arg0);                    /* crosshair aim step */
extern void  sound_queue_push(u32 cmd);                     /* sound_queue_push */
extern void  audio_queue_and_latch(s32 id);                      /* sound-if-enabled */
extern void *task_spawn_current_work40(void *parent, void *tmpl);    /* 0x40-record constructor */
extern void *task_spawn_current_work6c(void *parent, void *tmpl);    /* 0x6c-record constructor */
extern void *task_spawn_child_work88(void *parent, void *tmpl);    /* 0x88-record constructor */
extern void  screen_fx_builder_task(void);                        /* helper task handler */
extern void  prompt_tick_rotating_buttons(u32 task);                     /* prompt-HUD handler */
extern void  hud_update_player_meters(void);                        /* helper task handler */
extern void  effect_pulse_scale_task(void);                        /* helper task handler */
extern void  prop_tick_decelerating_descent(void);                        /* projectile task handler */

void setpiece_director(u32 task)
{
    u32 ent = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0:
        MEM32(DH_SCENE_EVENT_FLAGS_ADDR) |= 1;
        SEQUENCE_MEM32(ent, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list(ent);
        SEQUENCE_MEM32(ent, main_ticks) = MEM32(SEQUENCE_MEMU32(ent, spawn_cursor) + 4);
        task_spawn_current_work40((void *)(uintptr_t)task, (void *)screen_fx_builder_task);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 1:
    case 3: {
        s32 t = SEQUENCE_MEM32(ent, main_ticks) - 1;
        SEQUENCE_MEM32(ent, main_ticks) = t;
        if (t == 0) {
            MEM32(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0x3c) = 0x10;
            actor_mark_slots_for_refresh();
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    }

    case 2:
        process_spawn_list(ent);
        SEQUENCE_MEM32(ent, main_ticks) = MEM32(SEQUENCE_MEMU32(ent, spawn_cursor) + 4);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 4:
        process_spawn_list(ent);
        SEQUENCE_MEM32(ent, main_ticks) = MEM32(SEQUENCE_MEMU32(ent, spawn_cursor) + 4);
        audio_queue_and_latch(2);
        SEQUENCE_MEM32(ent, phase_ticks) = MEM32(0x06098570) * 8 + 0x46;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 5:
        if (SEQUENCE_MEM32(ent, main_ticks) < 0x5c) {
            u32 obj = MEMU32((u32)(uintptr_t)task_spawn_child_work88(
                                 (void *)(uintptr_t)MEMU32(0x06099580),
                                 (void *)prop_tick_decelerating_descent) + 8);
            MEM32(obj + 0x14) = 0x8580;
            MEM32(obj + 0x08) = 0x600000;
            MEM32(obj + 0x0c) = 0;
            MEM32(obj + 0x10) = 0x400000;
            MEM16(obj + 0x24) = 0;
            MEM16(obj + 0x26) = (s16)0x8000;
            MEM16(obj + 0x28) = 0;
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        /* fallthrough */
    case 6: {
        s32 c = SEQUENCE_MEM32(ent, phase_ticks) - 1;
        s32 t;
        SEQUENCE_MEM32(ent, phase_ticks) = c;
        if (c + 1 == 0) {
            task_spawn_child_work88((void *)(uintptr_t)task, (void *)prompt_tick_rotating_buttons);
            task_spawn_current_work6c((void *)(uintptr_t)task, (void *)hud_update_player_meters);
        }
        if (SEQUENCE_MEM32(ent, main_ticks) == 0x32 && MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push(DH_SOUND_PROP_LAUNCH_COMMAND);
        if (SEQUENCE_MEM32(ent, main_ticks) == 0x11)
            task_spawn_current_work6c((void *)(uintptr_t)MEMU32(0x06099580), (void *)effect_pulse_scale_task);
        t = SEQUENCE_MEM32(ent, main_ticks) - 1;
        SEQUENCE_MEM32(ent, main_ticks) = t;
        if (t == 0) {
            s32 sel = MEM32(0x0609856c) & 1;
            s32 idx;
            u32 node, tbl, nv;
            MEM32(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0x3c) = 0x10;
            actor_mark_slots_for_refresh();
            if (sel != 0) {
                s32 n = MEM32(0x06098570) + 1;
                MEM32(0x06098570) = n;
                if (n > 4)
                    MEM32(0x06098570) = 4;
            } else {
                player_update_aim_limits(0);
                MEM32(0x06098570) = 0;
            }
            idx = sel | (MEM32(0x06098568) * 2);
            node = SEQUENCE_MEMU32(ent, spawn_cursor);
            tbl = MEMU32(node + 0x2c);
            nv = MEMU32(tbl + idx * 4);
            SEQUENCE_MEM32(ent, branch_cursor) = nv;
            SEQUENCE_MEM32(ent, spawn_cursor) = nv;
            MEM32(ent + 0x08) = 1;
            MEM8(task + 15) = 1;
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    }

    case 7:
        process_spawn_list(ent);
        SEQUENCE_MEM32(ent, main_ticks) = MEM32(SEQUENCE_MEMU32(ent, spawn_cursor) + 4);
        SEQUENCE_MEM32(ent, phase_ticks) = 3;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 8: {
        s32 t = SEQUENCE_MEM32(ent, main_ticks) - 1;
        s32 c;
        s32 k;
        SEQUENCE_MEM32(ent, main_ticks) = t;
        if (t == 0) {
            MEM32(MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8) + 0x3c) = 0x10;
            actor_mark_slots_for_refresh();
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
            break;
        }
        if ((MEM32(0x0609856c) & 1) != 0)
            break;
        c = SEQUENCE_MEM32(ent, phase_ticks) - 1;
        SEQUENCE_MEM32(ent, phase_ticks) = c;
        if (c == 0 && MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push(0xa01205ff);
        for (k = 1; k <= 2; k++) {
            if (MEM32(DH_ACTOR_SLOT_IDS_ADDR + k * 4) != 0) {
                u32 e2 = MEMU32(MEMU32(DH_ACTOR_SLOT_TASKS_ADDR + k * 4) + 8);
                u32 drec = 0x06096c04 + k * 12;
                MEM32(e2 + 0x08) += MEM32(drec + 0);
                MEM32(e2 + 0x0c) += MEM32(drec + 4);
                MEM32(e2 + 0x10) += MEM32(drec + 8);
                if (SEQUENCE_MEM32(ent, phase_ticks) == 0) {
                    u32 sp4 = 0x06054080 + MEM8(0x060540e8 + MEM16(e2 + 0x5e)) * 8;
                    if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) {
                        s32 tv = MEM16(0x06053f04 + (u8)MEM8(sp4 + 1) * 2);
                        sound_queue_push(((u32)tv << 8) | DH_SOUND_CUE_BASE);
                    }
                }
            }
        }
        break;
    }

    default:
        break;
    }
}
