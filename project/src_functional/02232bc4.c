/* stage_intro_task_b [0x02232bc4]: Existing map corroborated by source: 7-state sibling of 0x02232610. Resets are unconditional; (src symbols_bandmd.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02232bc4 - sibling stage/round intro bootstrap task (7-state machine).
 *
 * task = r4 (held in r13); entity = task->unk8 (r10); state byte = task->unk10.
 * Same family as 0x02232610: a scene/stage bootstrap handler, not a math routine
 * (the div1/mac opcodes in the raw listing are literal pools). One phantom
 * `bsrf r6` at 0x232d58 is the literal word 0x0603 (loaded by mov.w at 0x232c36);
 * all 29 real computed branches resolve via braf_resolve.py.
 *
 * CFG (state at task->unk10):
 *   0 -> VDP2/VDP1 bring-up + texture/entity/HUD reset (unconditional, unlike the
 *        0x232610 variant which gates these on a signature table), advance to 1.
 *   1 -> sound + HUD/layer spawn, player-ready latch, advance to 2; FALLS THROUGH
 *        into state 2 the same frame (seeds entity->unk50 = 0xC7F).
 *   2 -> handler dispatch: ==0x2205812 -> state 3; ==0x2204AFA/0x2205232 -> hold;
 *        else draw glyph countdown via variadic thunk (*g_060902a0), spawn on
 *        expiry -> state 4; ready-gate -> state 6.
 *   3 -> VDP2 TVMD write, clear enables, tail-call func_0220fdc8(task->unkE+0xFF).
 *   4 -> both-players progress gate (unk5A>34, unk54 countdown, unk56 flags) ->
 *        state 5 or 6 via func_0222a04e.
 *   5 -> restart poll (handler==0x2204AFA) + entity->unk50 timeout -> state 2/6.
 *   6 -> cleanup (func_0222a0ec), back to state 3.
 *
 * fidelity: HIGH. ABI caveats: (*g_06090294)() no-arg VDP commit thunk (state 0);
 * the variadic glyph thunk (*g_060902a0)(...) in state 2 marshals exactly two
 * stack args - control word 0x608881c and (entity->unk50 >> 5) - pushed then
 * reclaimed with `add #8,r15`; modelled as a 2-arg call in that push order.
 */

/* ---- callees ---- */
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
extern void  player_clear_saved_action(s32 which);
extern void  scene_spawn_display_and_effects(void *task);          /* scene_stage_fade_spawn */
extern void  scene_load_line_scroll_layer(void *task);          /* scene_layer_loader_spawn_a (r5 clobbered) */
extern void *task_spawn_current_work6c(void *parent, void *tmpl); /* spawn (ctor fwds parent,tmpl) */
extern void  audio_queue_scene_sound(void);                /* sound_cue_dispatch (ignores r4) */
extern void  player_refresh_ready_mask(void);                /* player-ready status */
extern void  stage_draw_visible_cells(void);                /* reloads r4 from 0x609882c -> no args */
extern void  sound_queue_push(s32 cmd);             /* sound_queue_push */
extern void  task_cancel_below_depth(s32 depth);           /* prune active-list by depth */
extern s32   players_select_health_leader(void);
extern void  players_publish_health_order(void);
extern void  input_reset_records(void);                /* reset controller button records */

/* ---- globals / hardware ---- */
extern void (*g_06090294)(void);                 /* VDP commit callback slot */
extern void (*g_060902a0)(void *ctrl, s32 val);  /* variadic glyph/text thunk */
extern void *g_vdp2_register_shadow_legacy;                         /* VDP2 shadow-register block base */
extern void *g_scene_descriptor_legacy;                         /* active scene descriptor */
extern void *g_view_task;                         /* current source-actor holder */
extern void *g_06098678;                         /* active handler-node holder */
extern void *g_060995b4, *g_060995b8;            /* player A / player B */
extern u8    g_06088808[];                        /* arg block for func_02216b14 */
extern u8    g_0608881c[];                        /* glyph thunk control block */
extern s32   g_060986b8;
extern s32   g_060bacac, g_060bacb0, g_060bacb4;  /* enable-bit words */
extern s32   g_play_session_mode;
extern s32   g_06099578;
extern s8    g_0609861c;
extern s32   g_scene_event_flags;
extern s32   g_0609856c;
extern u16   g_06098812;
extern u16   g_0609880e;
extern u16   g_actor_pose_set_legacy;
extern u16   g_0609978c[];                         /* two words at +0,+2 */
extern s32   g_ui_cursor_column_legacy, g_ui_cursor_row_legacy;
extern u8    g_player_join_states[];                          /* two phase bytes at +0,+1 */
extern volatile u16 g_25f80000;                     /* VDP2 TVMD register */

void stage_intro_task_b(void *task) {
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
        desc = g_scene_descriptor_legacy;
        src = g_view_task;
        M32(src, 0) = SCENE_M32(desc, camera_definition);
        M8(src, 0x10) = 0;
        vdp1_upload_texture_groups(MP(desc, 0));
        vdp2_upload_pattern_groups(MP(desc, 4));
        model_load_scene_records(MP(desc, 8));
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
        player_clear_saved_action(1);
        player_clear_saved_action(2);
        g_actor_pose_set_legacy = 1;
        scene_spawn_display_and_effects(task);
        scene_load_line_scroll_layer(task);
        if (g_player_join_states[0] == 1) g_player_join_states[0] = 2;
        if (g_player_join_states[1] == 1) g_player_join_states[1] = 2;
        task_spawn_current_work6c(task, (void *)0x0602E120);
        g_0609978c[1] = 1;
        g_0609978c[0] = 1;
        g_06099578 = 1;
        g_0609861c = 1;
        g_scene_event_flags = 0;
        g_0609856c = 0;
        audio_queue_scene_sound();
        TASK_S8(task, state) = 2;
        M32(entity, 0x50) = 0xC7F;
        /* FALLTHROUGH into state 2 */
        /* fall through */
    case 2:
        player_refresh_ready_mask();
        stage_draw_visible_cells();
        {
            void *h = MP(g_06098678, 0);     /* dereferenced handler node */
            if (h == (void *)0x06009812) {
                g_0609856c = 2;
                TASK_S8(task, state) = 3;
                break;
            }
            if (h == (void *)0x06008AFA || h == (void *)0x06009232) {
                M32(entity, 0x50) = 0xC7F;
                goto s2_check_ready;
            }
            g_ui_cursor_column_legacy = 21;
            g_ui_cursor_row_legacy = 2;
            g_060902a0(g_0608881c, M32(entity, 0x50) >> 5);
            M32(entity, 0x50) -= 1;
            if (M32(entity, 0x50) > 0)
                goto s2_check_ready;
            task_spawn_current_work6c(task, (void *)0x06037122);
            g_play_session_mode = 0;
            input_reset_records();
            M32(entity, 0x54) = 92;
            TASK_S8(task, state) = 4;
            break;
        s2_check_ready:
            if (g_player_join_states[0] != 0 && g_player_join_states[1] != 0)
                break;
            g_play_session_mode = 0;
            input_reset_records();
            TASK_S8(task, state) = 6;
        }
        break;

    case 3:
        vb = g_vdp2_register_shadow_legacy;
        g_25f80000 = (u16)(MU16(vb, 0) & 0x7FFF);
        g_06099578 = 0;
        g_060bacac &= ~1;
        g_060bacb0 &= ~1;
        g_060bacb4 &= ~1;
        sound_queue_push((s32)DH_SOUND_SCENE_END_COMMAND);
        g_060986b8 = 0;
        M32(MP(TASK_PTR(task, parent), 8), 0x3C) |= 1;
        g_play_session_mode = 1;
        g_06098812 = 0;
        g_0609880e = 0;
        g_0609861c = 0;
        g_actor_pose_set_legacy = 0;
        task_cancel_below_depth(TASK_S8(task, depth) + 0xFF);   /* tail (braf) */
        break;

    case 4:
        stage_draw_visible_cells();
        {
            void *eA = MP(g_060995b4, 8);
            void *eB = MP(g_060995b8, 8);
            if (MU16(eA, 0x5A) > 34 || MU16(eB, 0x5A) > 34) {
                s32 old = M32(entity, 0x54);
                M32(entity, 0x54) = old - 1;
                if (old > 0)
                    break;
            }
            if (M16(eA, 0x56) != 0 && M16(eB, 0x56) != 0) {
                if (players_select_health_leader()) {
                    TASK_S8(task, state) = 6;
                } else {
                    g_play_session_mode = 1;
                    TASK_S8(task, state) = 5;
                }
                break;
            }
            if (M16(eA, 0x56) != 0 || M16(eB, 0x56) != 0) {
                g_play_session_mode = 1;
                TASK_S8(task, state) = 5;
            } else {
                TASK_S8(task, state) = 6;
            }
        }
        break;

    case 5:
        stage_draw_visible_cells();
        if (MP(g_06098678, 0) == (void *)0x06008AFA) {
            if (g_player_join_states[0] == 0 || g_player_join_states[1] == 0)
                TASK_S8(task, state) = 6;
        }
        M32(entity, 0x50) += 1;
        if (M32(entity, 0x50) > 110) {
            if (g_player_join_states[0] == 2 && g_player_join_states[1] == 2) {
                M32(entity, 0x50) = 0xC7F;
                TASK_S8(task, state) = 2;
            }
        }
        break;

    case 6:
        stage_draw_visible_cells();
        players_publish_health_order();
        TASK_S8(task, state) = 3;
        break;

    default:
        break;
    }
}
