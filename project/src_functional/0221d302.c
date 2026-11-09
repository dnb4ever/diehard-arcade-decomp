/* scene_tick_teardown_timer [0x0221d302]: Four states load resources and use scene-selected 06074d94 teardown timer record. */
/* 0x0221d302 - 4-state round/scene teardown timer task (tail-called by scene_timer_hud_task).
 * Picks a 12-byte record `rec` from the table at 0x06074D94 indexed by the scene director's
 * byte at (*g_0609866C)->unk29. entity = task->unk8. State at task->unk10:
 *   0: func_02217d6c(task); advance.
 *   1: func_02217f04(task); g_06098560 = func_0220feb0(g_bootSpawnArg, rec.u0); if rec.u4!=0
 *      func_02233388(task, rec.u4); set the VDP2 shadow (*g_06098F08) fields +0x114/+0x116/
 *      +0x118 to -0xFF; spawn an object (func_0220fece(task, func_022100f8)) and seed its
 *      entity->unk50 from (s16)rec.u8; if g_06098670==1 func_02216d58(); clear g_06098580,
 *      func_0222a8d8(); entity->unk54 = 0x270F; g_06098568 = g_06098564; -> state 2
 *      (falls through).
 *   2: func_02216d58(); when g_06098664==1: spawn func_0220fece(task, func_022101ce), seed
 *      its entity->unk50 from (s16)rec.uA, emit sound_emit_gated((0x80 - |rec.uA|) >> 1),
 *      advance to state 3.
 *   3: func_02216d58(); unless entity->unk3C bit1 is set: mark task->unk14->unk8->unk3C |= 1,
 *      write VDP2 TVMD (0x25F80000) from (*g_06098F08)->unk0 & 0x7FFF, clear g_06099578 and
 *      the low bit of g_060BACAC/B0/B4, sound_queue_push(0xA00003FF), zero g_060986B8/
 *      g_0609856C/g_0609861C/g_06098664, then tail-call screen_fade_sequencer
 *      (func_0220fdc8) with (s8)task->unkE + 0xFF.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. rec stride 12, director byte sign-extended
 *   then (u8)(idx*12). func_0220fece here is the 2-arg (parent, handler) form returning the
 *   object (r0); handlers 0x2100f8/0x2101ce via mova+add (ptr_resolve). braf_resolve for all
 *   bsrf/braf targets. |rec.uA| via cmp/pz+neg; final shar is arithmetic >>1. */
#include "dh_band21.h"

extern void  scene_reload_graphics(void *task);
extern void  scene_reset_stage(void *task);
extern void *task_spawn_child_work6c(void *a, s32 b);       /* returns obj */
extern void  scene_spawn_record_list(void *task, s32 arg);
extern void *task_spawn_current_work6c(void *task, void *handler); /* spawn_obj_0x6c, returns obj */
extern void  display_ramp_to_zero(void);                 /* spawned handler (template ptr) */
extern void  display_ramp_to_limit(void);                 /* spawned handler (template ptr) */
extern void  stage_draw_visible_cells(void);
extern s32   player_refresh_ready_mask(void);                 /* player-ready status (result unused) */
extern void  audio_queue_scene_gated_id(s16 id);              /* sound_emit_gated */
extern void  sound_queue_push_entry(s32 cmd);             /* sound_queue_push */
extern void  task_cancel_below_depth(s32 arg);             /* screen_fade_sequencer */

void scene_tick_teardown_timer(void *task)
{
    u32 dir = MEMU32(DH_SCENE_DESCRIPTOR_ADDR);
    u32 rec = 0x06074d94 + (u8)((s8)MEM8(dir + 0x29) * 12);
    u32 entity = MEMU32((u32)(uintptr_t)task + 8);
    u32 t = (u32)(uintptr_t)task;

    switch (MEM8(t + 0x10)) {
    case 0:
        scene_reload_graphics(task);
        MEM8(t + 0x10) = (s8)(MEM8(t + 0x10) + 1);
        break;
    case 1: {
        u32 p = MEMU32(DH_VDP2_REGISTER_SHADOW_ADDR);
        u32 obj;
        scene_reset_stage(task);
        MEM32(0x06098560) = (s32)(uintptr_t)task_spawn_child_work6c(
            (void *)(uintptr_t)MEMU32(0x060996d0), MEM32(rec + 0));
        if (MEM32(rec + 4) != 0)
            scene_spawn_record_list(task, MEM32(rec + 4));
        MEM16(p + 0x114) = -0xff;
        MEM16(p + 0x116) = -0xff;
        MEM16(p + 0x118) = -0xff;
        obj = (u32)(uintptr_t)task_spawn_current_work6c(task, (void *)display_ramp_to_zero);
        MEM32(MEMU32(obj + 8) + 0x50) = MEM16(rec + 8);
        if (MEM32(DH_PLAY_SESSION_MODE_ADDR) == 1)
            stage_draw_visible_cells();
        MEM32(DH_SCENE_EVENT_FLAGS_ADDR) = 0;
        player_refresh_ready_mask();
        MEM32(entity + 0x54) = 0x270f;
        MEM32(0x06098568) = MEM32(DH_READY_PLAYER_MASK_ADDR);
        MEM8(t + 0x10) = 2;
    }
        /* fallthrough */
    case 2:
        stage_draw_visible_cells();
        if (MEM32(0x06098664) == 1) {
            u32 obj = (u32)(uintptr_t)task_spawn_current_work6c(task, (void *)display_ramp_to_limit);
            s32 a = MEM16(rec + 0x0a);
            MEM32(MEMU32(obj + 8) + 0x50) = a;
            audio_queue_scene_gated_id((s16)((0x80 - (a < 0 ? -a : a)) >> 1));
            MEM8(t + 0x10) = 3;
        }
        break;
    case 3:
        stage_draw_visible_cells();
        if ((MEM32(entity + 0x3c) & 2) == 0) {
            u32 p = MEMU32(DH_VDP2_REGISTER_SHADOW_ADDR);
            u32 owner = MEMU32(MEMU32(t + 0x14) + 8);
            MEM32(owner + 0x3c) |= 1;
            MEM16(VDP2_TVMD_ADDR) = (s16)(MEM16(p + 0) & 0x7fff);
            MEM32(0x06099578) = 0;
            MEM32(0x060bacac) &= ~1;
            MEM32(0x060bacb0) &= ~1;
            MEM32(0x060bacb4) &= ~1;
            sound_queue_push_entry((s32)DH_SOUND_TRANSITION_DONE_COMMAND);
            MEM32(0x060986b8) = 0;
            MEM32(0x0609856c) = 0;
            MEM8(0x0609861c) = 0;
            MEM32(0x06098664) = 0;
            task_cancel_below_depth((s8)MEM8(t + 0x0e) + 0xff);
        }
        break;
    default:
        break;
    }
}
