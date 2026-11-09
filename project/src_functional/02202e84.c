/* scene_run_task [0x02202e84]: Drives the descriptor-selected scene resource, display and completion state machine. */
#include "dh_work.h"
#include "dh.h"
/* 0x02202e84 - func_02202e84: scene-run FSM task handler (sibling after the
 * 0x02202d14 / 0x202d7e / 0x202dda / 0x202dfa trampoline chain). Not called
 * directly: its runtime address 0x06006E84 sits in the scene descriptor table
 * (cart 0x270154 and 0x270294). One function, 0x202e84..0x20340a, with literal
 * pools embedded at 0x202ec8, 0x202fee..0x20303f, 0x203156..0x2031e7,
 * 0x20324a..0x20327b, 0x20336a..0x203397 (the "bsrf 0x629c7xx" and similar
 * capstone lines are pool words, not code).
 *
 * task->unk10 (s8) = state; ent = task->unk8; scene = *0x0609866c.
 *   state 0: VDP2 shadow (*0x06098F08) set-up for the scene, VDP2 mode 7 text
 *            setup, display-init thunk, actor-obj handler := 0x0220F6F6,
 *            scene palette/load calls; state -> 1.
 *   state 1: load the scene's three resource lists, set bit0 of the three
 *            g_060BACAC.. enables, optional voice clip, spawn the scene's HUD /
 *            background / layer tasks, reset player/HUD globals, spawn the
 *            actor-list task, arm ent->unk40 = 600 and pick the colour-ramp
 *            source pair (ent->unk64/68) by g_06098668; state -> 2 and FALLS
 *            THROUGH into state 2 (bra 0x203288).
 *   state 2: per-frame: unpack five words from each colour-ramp stream into
 *            the two player HUD records (0x06098734 / 0x0609874C), advance the
 *            streams, run the frame helpers; count ent->unk40 down - at 0 spawn
 *            the 0x022101ce task (its ent->unk50 = -16) and go to state 3; at
 *            300 set bit 18 of g_06098580.
 *   state 3: once ent->unk3C bit1 is clear: write the TVMD register
 *            (0x25F80000) with display off, clear the enables, sound command
 *            0xA00003FF, flag the parent's object (unk3C |= 1), and kill
 *            objects above id (s8)task->unkE - 1.
 *   other  : return.
 *
 * fidelity: HIGH - hand-traced vs capstone (work/opus_tail/shdis.py) over the
 * whole extent; every bsrf/braf target and mova+pool handler resolved:
 *   0x216154 0x222e44 0x23820c 0x214d26 0x216b14 0x227a84 0x23c95c 0x216c38
 *   0x2150d4 0x215228 0x223ac0 0x215e20 0x20fece 0x206e2e 0x207360 0x2089d0
 *   0x20feec 0x233388 0x2332d4 0x23dcc2 0x246052 0x216d58 0x243628 0x215f6c
 *   0x20fdc8; handlers 0x20f6f6 0x20340c 0x204258 0x2371c0 0x237ea6 0x246308
 *   0x22a120 0x2101ce.  Shadow/scene pointers are re-read at every access, as
 * in the ROM. The state-2 unpack uses two 5-entry u16 stack arrays (private
 * spills in the ROM). */

#define G8(a)   (*(s8  *)(uintptr_t)(a))
#define G16(a)  (*(s16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))
#define VS      GP(DH_VDP2_REGISTER_SHADOW_ADDR)                  /* VDP2 shadow-register block */
#define SCENE   GP(DH_SCENE_DESCRIPTOR_ADDR)                  /* current scene descriptor */
#define TVMD    (*(volatile u16 *)(uintptr_t)VDP2_TVMD_ADDR)

extern void (*g_06090294)(void);                /* ui/display init thunk */

extern void  video_reset_buffers(s32 mode);           /* video_state_init */
extern void  hud_setup_large_font(s32 mode);           /* VDP2 text-mode setup */
extern void  hud_upload_shared_assets(void);
extern void  vdp1_trigger_draw(void);
extern void  vdp2_apply_sprite_commands(u32 stream);
extern void  actor_reset_slot_registry(void);
extern void  effect_pool_reset_scene(void);
extern void  stage_initialize_grids(s32 arg0, s32 arg1);
extern void  vdp1_upload_texture_groups(s16 *list);
extern void  vdp2_upload_pattern_groups(u32 list);
extern void  model_load_scene_records(s16 *idlist);
extern void  audio_send_command_buffer(void *buf, s32 len); /* sound_send_command */
extern void  scene81_stage_setup_entry(void *ctx, void *scenedesc);   /* scene81_stage_setup */
extern void  scene_bg_layer_build_entry(void *task, void *entity);     /* scene_bg_layer_build */
extern void  scene_load_shutter_layer(void *task);
extern void *spawn_obj_0x6c(void *parent, void *handler); /* 0x0220fece */
extern void *task_spawn_child_work88(void *parent, void *handler);  /* spawn_obj_variant */
extern void  scene_spawn_record_list(void *parent, void *record);
extern void  scene_spawn_effect_list(void *node);
extern void  player_clear_saved_action(s32 idx);
extern void  combat_reset_stage_bounds(void);
extern void  stage_draw_visible_cells(void);
extern void  enemy_resolve_team_pairs(void);
extern void  sound_queue_push_entry(s32 req);            /* sound_queue_push */
extern void  task_cancel_below_depth(u8 depth);           /* kill_objects_above_id */

/* task handlers installed / spawned here */
extern void  camera_step_scene_definition(void *scene);
extern void  hud_install_status_task(void *task);         /* scene_hud_task_init */
extern void  scene_sprite_spin_task(void *task);         /* no C yet (sibling of 0x022041d4) */
extern void  hud_tick_eight_meter_slots(void *arg0);
extern void  hud_tick_player_popups(void *arg0);
extern void  enemy_wave_initialize_task(void *task);
extern void  actor_spawn_list_tick_raw_entry(void *task);         /* actor_spawn_list_tick */
extern void  display_ramp_to_limit(u32 task);

void scene_run_task(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32 state = TASK_S8(task, state);

    if (state == 0) {
        G32(0x06099578) = 0;
        video_reset_buffers(-1);
        M16(VS, 0xF8) = 0x703;
        M16(VS, 0xFA) = 0x700;
        M16(VS, 0xFC) = 4;
        M16(VS, 0xF0) = 5;
        M16(VS, 0xF2) = 0;
        M16(VS, 0xF4) = 0;
        M16(VS, 0xF6) = 0;
        M16(VS, 0xE0) = (s16)(M16(VS, 0xE0) & ~0x10);
        M16(VS, 0x0E) = (s16)((M16(VS, 0x0E) & 0xFCFF) | 0x300);
        M16(VS, 0x0E) = (s16)((M16(VS, 0x0E) & 0xFF00) | 0x23);
        M32(VS, 0x10) = -1;
        M32(VS, 0x14) = 0x454FFFFF;
        M32(VS, 0x18) = -1;
        M32(VS, 0x1C) = 0x0137FFFF;
        G32(0x060986B8) = 1;
        hud_setup_large_font(7);
        g_06090294();
        hud_upload_shared_assets();
        M16(VS, 0x114) = 0;
        M16(VS, 0x116) = 0;
        M16(VS, 0x118) = 0;
        vdp1_trigger_draw();
        M16(VS, 0x00) = (s16)((M16(VS, 0x00) & ~0x30) | 0x10);
        M16(VS, 0x00) = (s16)((M16(VS, 0x00) & ~0x07) | 0x03);
        vdp2_apply_sprite_commands(0x0604EF20);
        M16(VS, 0xE0) = (s16)((M16(VS, 0xE0) & ~0x0F) | 0x0F);
        {
            void *obj = GP(DH_VIEW_TASK_ADDR);         /* current actor object */
            M8(obj, 0x10) = 0;
            MP(obj, 0) = (void *)0x0220F6F6;    /* mova 0x202fc8 + 0xC72E */
        }
        actor_reset_slot_registry();
        effect_pool_reset_scene();
        stage_initialize_grids(M32(SCENE, 0x20), M32(SCENE, 0x24));
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        return;
    }

    if (state == 1) {
        u8 *rec;

        vdp1_upload_texture_groups((s16 *)MP(SCENE, 0x0));
        vdp2_upload_pattern_groups(MU32(SCENE, 0x4));
        model_load_scene_records((s16 *)MP(SCENE, 0x8));
        G32(0x060BACAC) = G32(0x060BACAC) | 1;
        G32(0x060BACB0) = G32(0x060BACB0) | 1;
        G32(0x060BACB4) = G32(0x060BACB4) | 1;
        if (M32(SCENE, 0xC) != 0)
            audio_send_command_buffer(MP(SCENE, 0xC), 0x6C000);
        M16(VS, 0x00) = (s16)((M16(VS, 0x00) & 0x7FFF) | 0x8000);
        spawn_obj_0x6c(task, (void *)hud_install_status_task);
        scene81_stage_setup_entry(task, SCENE);
        scene_bg_layer_build_entry(task, SCENE);
        scene_load_shutter_layer(task);
        task_spawn_child_work88(task, (void *)scene_sprite_spin_task);
        spawn_obj_0x6c(task, (void *)hud_tick_eight_meter_slots);
        spawn_obj_0x6c(task, (void *)hud_tick_player_popups);

        /* per-scene record: 12-byte entries, byte offset truncated to s8 */
        rec = (u8 *)(uintptr_t)(0x0604EDA8 + (s8)(M8(SCENE, 0x29) * 12));
        if (M32(rec, 4) != 0)
            scene_spawn_record_list(task, MP(rec, 4));
        if (M32(rec, 0) != 0)
            scene_spawn_effect_list(MP(rec, 0));
        player_clear_saved_action(1);
        player_clear_saved_action(2);

        G16(0x0609978E) = 5;
        G16(0x0609978C) = 5;
        {
            s16 v = G16(0x06099794);
            G16(0x0609978A) = v;
            G16(0x06099788) = v;
        }
        G32(0x06099760) = 30;
        G32(0x0609975C) = 30;
        G8(0x06098689) = 0;
        G8(0x06098688) = 0;
        G8(0x0609868B) = 0;
        G8(0x0609868A) = 0;
        G16(0x06099792) = 0;
        G16(0x06099790) = 0;
        G8(0x0609868D) = 0;
        G8(0x0609868C) = 0;
        G32(DH_SCENE_EVENT_FLAGS_ADDR) = 0;
        G32(DH_RANDOM_SEED_ADDR) = 0;

        spawn_obj_0x6c(task, (void *)enemy_wave_initialize_task);
        combat_reset_stage_bounds();
        G32(0x0609856C) = 0;
        {
            void *o = spawn_obj_0x6c(task, (void *)actor_spawn_list_tick_raw_entry);
            M32(MP(o, 8), 0x48) = 1;
            G32(0x06099578) = 1;
        }
        G8(0x0609861C) = 1;
        G32(0x06098568) = 3;
        M32(ent, 0x40) = 600;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        if (G32(DH_SCENE_INDEX_ADDR) == 4) {
            M32(ent, 0x64) = 0x22180014;
            M32(ent, 0x68) = 0x22183014;
        } else {
            M32(ent, 0x64) = 0x22186014;
            M32(ent, 0x68) = 0x22189014;
        }
        state = 2;                              /* bra 0x203288: fall into state 2 */
    }

    if (state == 2) {
        u8 *s0 = MP(ent, 0x64);
        u8 *s1 = MP(ent, 0x68);
        u16 a[5], b[5];
        s32 i, v;

        for (i = 0; i < 5; i++) {
            u32 w = MU32(s0, 0);
            s0 += 4;
            a[i] = (u16)(((w & 0xFF0000) >> 8) | (w & 0xFF));
            w = MU32(s1, 0);
            s1 += 4;
            b[i] = (u16)(((w & 0xFF0000) >> 8) | (w & 0xFF));
        }
        G16(DH_PLAYER_INPUT_RECORDS_ADDR) = (s16)a[0];
        G16(0x06098738) = (s16)a[1];
        G16(0x06098740) = (s16)a[2];
        G16(0x06098744) = (s16)a[3];
        G16(0x06098746) = (s16)a[4];
        G16(0x0609874C) = (s16)b[0];
        G16(0x06098750) = (s16)b[1];
        G16(0x06098758) = (s16)b[2];
        G16(0x0609875C) = (s16)b[3];
        G16(0x0609875E) = (s16)b[4];
        MP(ent, 0x64) = s0;
        MP(ent, 0x68) = s1;
        G32(DH_READY_PLAYER_MASK_ADDR) = 3;
        stage_draw_visible_cells();
        enemy_resolve_team_pairs();

        v = M32(ent, 0x40) - 1;
        M32(ent, 0x40) = v;
        if (v == 0) {
            void *o = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
            M32(MP(o, 8), 0x50) = -16;
            TASK_S8(task, state) = 3;
        }
        if (M32(ent, 0x40) == 300)
            G32(DH_SCENE_EVENT_FLAGS_ADDR) = G32(DH_SCENE_EVENT_FLAGS_ADDR) | 0x40000;
        return;
    }

    if (state == 3) {
        stage_draw_visible_cells();
        if (M32(ent, 0x3C) & 2)
            return;
        TVMD = (u16)(MU16(VS, 0x00) & 0x7FFF);  /* display off, straight to hardware */
        G32(0x06099578) = 0;
        G32(0x060BACAC) = G32(0x060BACAC) & ~1;
        G32(0x060BACB0) = G32(0x060BACB0) & ~1;
        G32(0x060BACB4) = G32(0x060BACB4) & ~1;
        sound_queue_push_entry((s32)DH_SOUND_TRANSITION_DONE_COMMAND);
        G32(0x060986B8) = 0;
        {
            void *o = MP(TASK_PTR(task, parent), 8);
            M32(o, 0x3C) = M32(o, 0x3C) | 1;
        }
        G32(DH_RANDOM_SEED_ADDR) = G32(DH_FRAME_TICK_ADDR);
        task_cancel_below_depth((u8)(TASK_S8(task, depth) + 0xFF));
    }
}
