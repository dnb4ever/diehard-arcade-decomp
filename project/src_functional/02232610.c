/* scene_tick_round_intro [0x02232610]: Six-state stage bootstrap with resource, fade and spawn-list progression. */
#include "dh_work.h"
#include "dh.h"
/* 0x02232610 - stage/round intro bootstrap task (6-state machine).
 *
 * task = r4; entity = task->unk8; state byte = task->unk10 (offset 0x10).
 * Despite the "math dispatcher" label in the triage brief this is a scene/stage
 * bootstrap handler: the apparent div1/mac opcodes in the raw listing are literal
 * pools (base ptr 0x6098f08, VDP2 reg data, bsrf deltas) misdecoded as code. One
 * phantom `bsrf r6` at 0x2327c4 is the literal word 0x0603 (loaded by the mov.w at
 * 0x232676); all 35 real computed branches resolve via braf_resolve.py.
 *
 * CFG (state at task->unk10):
 *   0 -> full VDP2/VDP1 bring-up, texture/entity/HUD reset, advance to 1.
 *   1 -> sound + fade/layer spawn, player-ready latch, advance to 2; FALLS THROUGH
 *        into state 2 the same frame.
 *   2 -> ready poll; if entity armed & handler != 0x2205232 set timer & advance to 3;
 *        converge: if handler == 0x2205812 mark task->unk14 and jump to state 5.
 *   3 -> wait handler==0x2205232 (restart) else count entity->unk50 down; on 0 spawn
 *        fade obj, refresh players, emit gated sound, advance to 4.
 *   4 -> per-frame: gate on entity->unk3C&2, VDP2 TVMD write, refresh/publish,
 *        clear enable bits, tail-call func_0220fdc8(task->unkE) (braf).
 *   5 -> like 4 but leaner; func_0220fdc8(task->unkE+0xFF) as a normal call.
 *
 * fidelity: HIGH. The one ABI caveat is the no-arg indirect thunk (*g_06090294)()
 * in state 0 (a VDP commit callback) - no stack args are marshalled at the call
 * site, modelled as a plain no-arg call.
 */

/* ---- callees (names from notes/symbols*.txt where known) ---- */
extern void  video_reset_buffers(s32 mode);            /* video_state_init */
extern void  hud_setup_large_font(s32 mode);            /* vdp2_setup_largefont */
extern void  hud_upload_shared_assets(void);                /* gfx_vram_dma_setup */
extern void  vdp1_trigger_draw(void);                /* vdp1_draw_kick */
extern void  vdp2_apply_sprite_commands(void *p);
extern void  vdp1_upload_texture_groups(void *groups);        /* upload_texture_groups */
extern void  vdp2_upload_pattern_groups(void *p);
extern void  model_load_scene_records(void *p);
extern void  actor_reset_slot_registry(void);                /* entity_system_reset */
extern void  effect_pool_reset_scene(void);                /* func_0223c95c */
extern void  stage_initialize_grids(void *a, void *b);
extern void  audio_send_command_buffer(s32 a, s32 b);        /* sound_send_command */
extern void  scene_spawn_display_and_effects(void *task);          /* scene_stage_fade_spawn */
extern void *task_spawn_current_work6c(void *parent, void *tmpl); /* spawn (ctor fwds parent,tmpl) */
extern void  combat_reset_stage_bounds(void);
extern void  audio_queue_scene_sound(void);                /* sound_cue_dispatch (ignores r4) */
extern void  player_refresh_ready_mask(void);                /* player-ready status */
extern void  stage_draw_visible_cells(void);                /* reloads r4 from 0x609882c -> no args */
extern void  enemy_resolve_team_pairs(void);
extern s32   players_entities_ready(void);                /* players_entities_ready */
extern void  input_reset_records(void);                /* reset controller button records */
extern void  effect_retire_finished_slots(void);                /* active-effect slot sweep */
extern void  both_players_special_trigger(void);                /* players_special_refresh */
extern void  player_save_action(s32 which);           /* actor_status_publish */
extern void  audio_queue_scene_gated_id(s32 id);              /* sound_emit_gated */
extern void  sound_queue_push(s32 cmd);             /* sound_queue_push */
extern void  task_cancel_below_depth(s32 depth);           /* prune active-list by depth */

/* ---- globals / hardware ---- */
extern void (*g_06090294)(void);                 /* VDP commit callback slot */
extern void *g_vdp2_register_shadow_legacy;                         /* VDP2 shadow-register block base */
extern void *g_scene_descriptor_legacy;                         /* active scene descriptor */
extern void *g_view_task;                         /* current source-actor holder */
extern void *g_06098678;                         /* active handler-node holder */
extern u8    g_06072660[];                       /* per-stage resource signature table, stride 0x50 */
extern u8    g_06088700[];                       /* fade-descriptor table, stride 0xC */
extern u8    g_06088808[];                       /* arg block for func_02216b14 */
extern s8    g_0609868f;                         /* stage index */
extern s32   g_060986b8;
extern s32   g_060bacac, g_060bacb0, g_060bacb4; /* enable-bit words */
extern s32   g_play_session_mode;
extern s32   g_ready_player_mask, g_06098568;
extern s32   g_06099578;
extern s8    g_0609861c;
extern s32   g_scene_event_flags;
extern s32   g_06096c90;
extern s32   g_0609856c;
extern u16   g_06098812;
extern u16   g_0609880e;
extern u8    g_player_join_states[];                        /* two phase bytes at +0,+1 */
extern volatile u16 g_25f80000;                   /* VDP2 TVMD register */

void scene_tick_round_intro(void *task) {
    void *entity = TASK_PTR(task, work);
    void *desc;
    void *vb;
    void *src;

    switch (TASK_U8(task, state)) {
    case 0:
        video_reset_buffers(5);
        vb = g_vdp2_register_shadow_legacy;
        M16(vb, 0xF8) = 0x603;
        M16(vb, 0xFA) = 0x700;
        M16(vb, 0xFC) = 4;
        M16(vb, 0xF0) = 5;
        M16(vb, 0xF2) = 0;
        M16(vb, 0xF4) = 0;
        M16(vb, 0xF6) = 0;
        M16(vb, 0xE0) = (s16)(M16(vb, 0xE0) & ~0x10);
        M16(vb, 0xE)  = (s16)((M16(vb, 0xE) & ~0x300) | 0x300);
        M16(vb, 0xE)  = (s16)((M16(vb, 0xE) & ~0xFF) | 0x23);
        M32(vb, 0x10) = -1;
        M32(vb, 0x14) = 0x454FFFFF;
        M32(vb, 0x18) = -1;
        M32(vb, 0x1C) = 0x0137FFFF;
        g_060986b8 = 1;
        hud_setup_large_font(7);
        g_06090294();
        hud_upload_shared_assets();
        vdp1_trigger_draw();
        M16(vb, 0) = (s16)((M16(vb, 0) & ~0x30) | 0x10);
        M16(vb, 0) = (s16)((M16(vb, 0) & ~7) | 3);
        vdp2_apply_sprite_commands(g_06088808);
        M16(vb, 0xE0) = (s16)((M16(vb, 0xE0) & ~0xF) | 0xF);
        M16(vb, 0) = (s16)((M16(vb, 0) & 0x7FFF) | 0x8000);
        src = g_view_task;
        M8(src, 0x10) = 0;
        desc = g_scene_descriptor_legacy;
        M32(src, 0) = SCENE_M32(desc, camera_definition);
        {
            s8 idx = g_0609868f;
            if (MP(desc, 0) != MP(g_06072660, idx * 0x50))
                vdp1_upload_texture_groups(MP(desc, 0));
            if (MP(desc, 4) != MP(g_06072660, idx * 0x50 + 4))
                vdp2_upload_pattern_groups(MP(desc, 4));
            if (MP(desc, 8) != MP(g_06072660, idx * 0x50 + 8))
                model_load_scene_records(MP(desc, 8));
        }
        actor_reset_slot_registry();
        effect_pool_reset_scene();
        stage_initialize_grids(MP(desc, 0x20), MP(desc, 0x24));
        TASK_S8(task, state) += 1;
        break;

    case 1:
        desc = g_scene_descriptor_legacy;
        if (SCENE_M32(desc, sound_resource) != 0)
            audio_send_command_buffer(SCENE_M32(desc, sound_resource), 0x6C000);
        g_060bacac |= 1;
        g_060bacb0 |= 1;
        g_060bacb4 |= 1;
        scene_spawn_display_and_effects(task);
        task_spawn_current_work6c(task, (void *)0x0604A308);
        combat_reset_stage_bounds();
        if (g_player_join_states[0] == 1) g_player_join_states[0] = 2;
        if (g_player_join_states[1] == 1) g_player_join_states[1] = 2;
        task_spawn_current_work6c(task, (void *)0x0602E120);
        g_06099578 = 1;
        g_0609861c = 1;
        g_scene_event_flags = 0;
        g_06096c90 = 0;
        audio_queue_scene_sound();
        player_refresh_ready_mask();
        g_06098568 = g_ready_player_mask;
        TASK_S8(task, state) = 2;
        /* FALLTHROUGH into state 2 */
        /* fall through */
    case 2:
        player_refresh_ready_mask();
        stage_draw_visible_cells();
        enemy_resolve_team_pairs();
        if ((M32(entity, 0x3C) & 8) && MP(g_06098678, 0) != (void *)0x06009232) {
            M32(entity, 0x50) = players_entities_ready() ? 70 : 30;
            g_play_session_mode = 0;
            input_reset_records();
            effect_retire_finished_slots();
            TASK_S8(task, state) = 3;
        }
        if (MP(g_06098678, 0) == (void *)0x06009812) {
            M8(TASK_PTR(task, parent), 0xD) = -1;
            TASK_S8(task, state) = 5;
        }
        break;

    case 3:
        stage_draw_visible_cells();
        if (MP(g_06098678, 0) == (void *)0x06009232) {
            g_play_session_mode = 1;
            M32(entity, 0x50) = 30;
            break;
        }
        M32(entity, 0x50) -= 1;
        if (M32(entity, 0x50) != 0)
            break;
        desc = g_scene_descriptor_legacy;
        {
            s16   sidx = SCENE_M16(desc, pose_index);
            void *fade = g_06088700 + (s16)(sidx * 0xC);
            void *obj  = task_spawn_current_work6c(task, (void *)0x060141CE);
            s32   v, a;
            M32(MP(obj, 8), 0x50) = (s16)M16(fade, 0xA);
            both_players_special_trigger();
            v = (s16)M16(fade, 0xA);
            a = (v < 0) ? -v : v;
            audio_queue_scene_gated_id((0x80 - a) >> 1);
            TASK_S8(task, state) = 4;
        }
        break;

    case 4:
        stage_draw_visible_cells();
        if (M32(entity, 0x3C) & 2)
            break;
        vb = g_vdp2_register_shadow_legacy;
        g_25f80000 = (u16)(MU16(vb, 0) & 0x7FFF);
        g_06099578 = 0;
        both_players_special_trigger();
        player_save_action(1);
        player_save_action(2);
        g_060bacac &= ~1;
        g_060bacb0 &= ~1;
        g_060bacb4 &= ~1;
        sound_queue_push((s32)DH_SOUND_TRANSITION_DONE_COMMAND);
        g_060986b8 = 0;
        M32(MP(TASK_PTR(task, parent), 8), 0x3C) |= 1;
        g_0609856c = 0;
        g_play_session_mode = 1;
        g_06098812 = 0;
        g_0609880e = 0;
        task_cancel_below_depth(TASK_S8(task, depth));   /* tail (braf) */
        break;

    case 5:
        vb = g_vdp2_register_shadow_legacy;
        g_25f80000 = (u16)(MU16(vb, 0) & 0x7FFF);
        g_06099578 = 0;
        g_060bacac &= ~1;
        g_060bacb0 &= ~1;
        g_060bacb4 &= ~1;
        sound_queue_push((s32)DH_SOUND_TRANSITION_DONE_COMMAND);
        g_060986b8 = 0;
        g_0609856c = 0;
        g_play_session_mode = 0;
        g_06098812 = 0;
        g_0609880e = 0;
        M32(MP(TASK_PTR(task, parent), 8), 0x3C) |= 1;
        task_cancel_below_depth(TASK_S8(task, depth) + 0xFF);
        g_0609861c = 0;
        break;

    default:
        break;
    }
}
