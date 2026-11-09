/* event_gate_branch_draft [0x0221edec]: Event branch draft with unresolved scalar-pair helper return; v1.09 limitation preserved. */
#include "dh.h"
/* 0x0221edec evt_actor_gate_branch - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index (0..13; >=14 exits).
 * Gate entity re-read each invocation: gate = (*(u32*)0x0609882C)->unk8,
 * cond = (s16)gate->unkAE. States dispatch via the 14-entry table at 0x0221ee30.
 * func_0221fdca returns a register pair (r0=base, r1=idx); a byte offset from the
 * live-in r10 table selects a word where 0/1/0 flag values are stored.
 * Common tail: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 * task->unk10++.
 * fidelity: MEDIUM - dispatch table contents not present in the reachable
 * listing; case bodies reconstructed from the register trace. The >=14 guard,
 * gate chain, signed cond load (0xAE) and post-increment of task->unk10 exact. */
extern u64  event_select_branch_pair_interior(s16 cond, u32 gate);   /* r0=base, r1=idx */
extern s32  scene_select_player_parameter(void);                 /* scene_value_select */
extern void process_spawn_list(void *entity);
extern u8    g_tbl_idx[];                        /* live-in r10 byte table */
extern s32   g_scene_event_flags;

void event_gate_branch_draft(void *arg0) {
    void *entity = MP(arg0, 0x8);
    void *gate = MP(MP((void *)DH_VIEW_TASK_ADDR, 0x0), 0x8);
    s16 cond = (s16)M16(gate, 0xae);
    s8 state = M8(arg0, 0x10);

    if ((u32)(u8)state >= 0xe) {
        return;
    }

    switch (state) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    case 1:
        if (cond == 1) {
            M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        }
        break;
    case 2:
        process_spawn_list(entity);
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    case 3: {
        u64 ret = event_select_branch_pair_interior(cond, (u32)gate);
        u8 off = g_tbl_idx[(u32)(ret >> 32)];
        M32((u32)ret, off) = 0;
        M32((u32)ret, off) = 1;
        M32((u32)ret, off) = 0;
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    }
    case 4:
        if (cond == 1) {
            M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        }
        break;
    case 5:
        if (cond == 1) {
            u64 ret = event_select_branch_pair_interior(cond, (u32)gate);
            u8 off = g_tbl_idx[(u32)(ret >> 32)];
            M32((u32)ret, off) = 0;
            M32((u32)ret, off) = 1;
            M32((u32)ret, off) = 0;
            M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        }
        break;
    case 6:
        process_spawn_list(entity);
        g_scene_event_flags |= 4;
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    case 7:
        if (cond == 1) {
            M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        }
        break;
    case 8: {
        u64 ret = event_select_branch_pair_interior(cond, (u32)gate);
        u8 off = g_tbl_idx[(u32)(ret >> 32)];
        M32((u32)ret, off) = 0;
        M32((u32)ret, off) = 1;
        M32((u32)ret, off) = 0;
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    }
    case 9:
        if (cond == 2) {
            M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        }
        break;
    case 10:
        process_spawn_list(entity);
        g_scene_event_flags |= 2;
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    case 11: {
        u64 ret = event_select_branch_pair_interior(cond, (u32)gate);
        u8 off = g_tbl_idx[(u32)(ret >> 32)];
        M32((u32)ret, off) = 0;
        M32((u32)ret, off) = 1;
        M32((u32)ret, off) = 0;
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    }
    case 12:
        if (cond != 0) {
            M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        }
        break;
    case 13:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        M8(arg0, 0x10) = (s8)(M8(arg0, 0x10) + 1);
        break;
    default:
        break;
    }
}
