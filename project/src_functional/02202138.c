#include "dh_work.h"
/* scene_vdp2_bringup_task [0x02202138]: Task states 0..7 program the VDP2 shadow and sequence scene layers, sound and display enable. */
#include "dh.h"
/* 0x02202138 - scene / VDP2-setup STATE MACHINE (task handler). task->unk8 =
 * entity; task->unk10 = state (0..7). Drives the per-frame bring-up of a game
 * scene: state 0 programs the VDP2 register shadow block (g_06098f08) and kicks
 * the scene's loaders/spawns, states 1..7 sequence the layer spawns, timed sound
 * cues (via sound_queue_push), fade/scroll ramps and the final display enable.
 *
 * NOTE ON DISPATCH: wave-1 deferred this as having "TABLE/COMPUTED stc sr / bsrf
 * r0" dispatch. That was a false positive: braf_resolve.py linearly mis-decodes
 * the function's several EMBEDDED literal pools (e.g. 0x2023dc-0x2023eb,
 * 0x202640, 0x202678/7c) as bsrf instructions. This function has NO computed
 * jump table at all - the top dispatch is a plain cmp/eq ladder on task->unk10,
 * and the inner "switches" in states 2/5/6 are cmp/eq ladders on the
 * entity->unk50 down-counter. Every bsrf is a resolved static call. Nothing is
 * stubbed.
 *
 * Held registers across the body: r13=task, r11=entity, r8=&g_06098580,
 * r9=&g_060e0ddc, r10=&g_0609866c, r12=sound_queue_push(0x215f6c),
 * r14=&g_06098f08.  S = *g_06098f08 (VDP2 shadow struct), P = *g_0609866c
 * (scene descriptor).
 *
 * fidelity: HIGH overall; MED on three calls whose r4 argument is inherited
 * (not freshly loaded) across a prior call and is therefore only correct if that
 * prior callee preserves r4 - flagged inline: (*0x06090294), func_02214d26,
 * func_0223c95c. All bsrf resolved with braf_resolve.py; all mova+add PIC handler
 * pointers resolved by hand (spawn targets 0x2191ec/0x20340c/0x2100f8/0x2101ce/
 * 0x204308/0x2041d4). */

#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

/* VDP2 TVMD display register */
#define REG_TVMD (*(volatile u16 *)(uintptr_t)VDP2_TVMD_ADDR)

extern void  video_reset_buffers(s32 a);                     /* 0x216154 */
extern void  hud_setup_large_font(s32 a);                      /* 0x222e44 */
extern void  vdp1_trigger_draw(void);                       /* 0x214d26 (r4 inherited) */
extern void  vdp1_upload_texture_groups(s32 a);                      /* 0x2150d4 */
extern void  vdp2_upload_pattern_groups(s32 a);                      /* 0x215228 */
extern void  model_load_scene_records(s32 a);                      /* 0x223ac0 */
extern void  audio_send_command_buffer(s32 a, s32 b);               /* 0x215e20 */
extern void  actor_reset_slot_registry(void *obj);                  /* 0x227a84 */
extern void  effect_pool_reset_scene(void);                       /* 0x23c95c (r4 inherited) */
extern void  stage_initialize_grids(s32 a, s32 b);               /* 0x216c38 */
extern void  scene_load_background_bank0(void *task, void *desc);     /* 0x207224 stage_vram_loader */
extern void  scene_load_streamed_layer(void *task, void *desc);     /* 0x207772 scene_layer_loader_spawn */
extern void  scene_display_initialize_entry(void *task, void *desc);     /* 0x2082be */
extern void  task_spawn_current_workdc(void *task, void *handler); /* 0x20ff90 */
extern void  scene_spawn_matching_behaviors(void *task);                 /* 0x21ad58 */
extern void  vdp2_queue_attribute(s32 a, void *b, u32 c, s32 d); /* 0x21632e */
extern void  hud_draw_level_record(s32 a, s32 b, s32 c);        /* 0x238364 */
extern void  task_cancel_below_depth(s32 a);                     /* 0x20fdc8 */
extern void *spawn_obj_0x6c(void *parent, void *handler);  /* 0x20fece (returns obj) */
extern void *task_spawn_child_work88(void *parent, void *handler);  /* 0x20feec (returns obj) */
extern void  sound_queue_push(s32 cmd);                   /* 0x215f6c sound_queue_push */

/* scene spawn handlers (referenced by address) */
extern void prop_tick_banked_path(void *task);
extern void hud_install_status_task(void *task);   /* scene_hud_task_init */
extern void display_ramp_to_zero(void *task);
extern void display_ramp_to_limit(void *task);
extern void scene_sprite_ease_task(void *task);
extern void scene_sprite_acceleration_task(void *task);

void scene_vdp2_bringup_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *S = GP(DH_VDP2_REGISTER_SHADOW_ADDR);   /* VDP2 register shadow */
    void *P = GP(DH_SCENE_DESCRIPTOR_ADDR);   /* scene descriptor */
    void *obj;

    switch (TASK_U8(task, state)) {
    case 0:
        video_reset_buffers(-1);
        MU16(S, 0xe) = (u16)((MU16(S, 0xe) & ~0x300) | 0x300);
        MU16(S, 0xe) = (u16)((MU16(S, 0xe) & ~0xff) | 0x23);
        M32(S, 0x10) = -1;
        M32(S, 0x14) = 0x454fffff;
        M32(S, 0x18) = -1;
        M32(S, 0x1c) = 0x0137ffff;
        hud_setup_large_font(7);
        (*(void (**)(s32))(uintptr_t)0x06090294)(7);  /* r4 inherited (=7) */
        G32(0x060986b8) = 0;
        vdp1_trigger_draw();                                /* r4 inherited */
        MU16(S, 0) = (u16)((MU16(S, 0) & ~0x30) | 0x10);
        MU16(S, 0) = (u16)((MU16(S, 0) & ~0x7) | 0x3);
        MU16(S, 0xe0) = (u16)((MU16(S, 0xe0) & ~0xf) | 0xf);
        MU16(S, 0x110) = (u16)((MU16(S, 0x110) & ~0x40) | 0x40);
        MU16(S, 0x112) = (u16)(MU16(S, 0x112) & ~0x40);
        MU16(S, 0xf8) = 0x604;
        MU16(S, 0xfa) = 0x700;
        MU16(S, 0xfc) = 3;
        MU16(S, 0xf0) = 5;
        MU16(S, 0xf2) = 0;
        MU16(S, 0xf4) = 0;
        MU16(S, 0xf6) = 0;
        MU16(S, 0) = (u16)((MU16(S, 0) & 0x7fff) | 0x8000);
        vdp1_upload_texture_groups(SCENE_M32(P, texture_groups));
        vdp2_upload_pattern_groups(SCENE_M32(P, pattern_groups));
        model_load_scene_records(SCENE_M32(P, model_list));
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);   /* -> 1 */
        break;

    case 1:
        if (SCENE_M32(P, sound_resource) != 0) {
            audio_send_command_buffer(SCENE_M32(P, sound_resource), 0x6c000);
        }
        {
            void *q = GP(DH_VIEW_TASK_ADDR);
            M32(q, 0) = SCENE_M32(P, camera_definition);
            actor_reset_slot_registry(q);
            M8(q, 0x10) = 0;
        }
        effect_pool_reset_scene();                               /* r4 inherited */
        stage_initialize_grids(SCENE_M32(P, stage_index), SCENE_M32(P, view_radius));
        scene_load_background_bank0(task, P);
        scene_load_streamed_layer(task, P);
        scene_display_initialize_entry(task, P);
        task_spawn_current_workdc(task, (void *)prop_tick_banked_path);
        spawn_obj_0x6c(task, (void *)hud_install_status_task);
        scene_spawn_matching_behaviors(task);
        M32(entity, 0x50) = 0x5dc;
        *(s8 *)(uintptr_t)0x0609861c = 1;
        MU16(S, 0x114) = (u16)-255;
        MU16(S, 0x116) = (u16)-255;
        MU16(S, 0x118) = (u16)-255;
        obj = spawn_obj_0x6c(task, (void *)display_ramp_to_zero);
        M32(MP(obj, 8), 0x50) = 0x40;
        if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(DH_SOUND_LATCH_BASE);
        if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(DH_SOUND_PROP_LAUNCH_COMMAND);
        G32(0x06099578) = 1;
        TASK_S8(task, state) = 2;
        break;

    case 2: {
        s32 c = M32(entity, 0x50) - 1;
        M32(entity, 0x50) = c;
        if (c == 0) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
            M32(MP(obj, 8), 0x50) = 0x18;
            TASK_S8(task, state) = 3;
            M32(entity, 0x50) = 0x12c;
        } else if (c == 0xf0) {
            G32(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x20000;
        } else if (c == 0x1e0 || c == 0x3c0) {
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(0xa01401ff);
            G32(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x10000;
        } else if (c == 0x1a4) {
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(DH_SOUND_PROP_LAUNCH_COMMAND);
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(0xa01204ff);
        } else if (c == 0x2a3) {
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(0xa01203ff);
        } else if (c == 0x384) {
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(DH_SOUND_PROP_LAUNCH_COMMAND);
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(0xa01202ff);
        } else if (c == 0x483) {
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(0xa01201ff);
        } else if (c == 0x528) {
            if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) sound_queue_push(0xa01200ff);
        }
        /* c == 120 and all others: fall to tail */
        break;
    }

    case 3:
        M32(S, 0x84) += 0x34ccc;
        if (!(M32(entity, 0x3c) & 2)) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_zero);
            M32(MP(obj, 8), 0x50) = -16;
            TASK_S8(task, state) = 5;
            vdp2_queue_attribute(14, GP(0x22700ad0), 0x25f00000, 0x80);
            obj = task_spawn_child_work88(task, (void *)scene_sprite_ease_task);
            {
                void *e = MP(obj, 8);
                M32(e, 8) = (s32)0xffff4ccd;
                M32(e, 0xc) = -0x3333;
                M32(e, 0x10) = 0x11999;
                M16(e, 0x30) = 11;
            }
            obj = task_spawn_child_work88(task, (void *)scene_sprite_ease_task);
            {
                void *e = MP(obj, 8);
                M32(e, 8) = 0xb333;
                M32(e, 0xc) = -0x1eb8;
                M32(e, 0x10) = 0x11999;
                M16(e, 0x30) = 12;
            }
            G32(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x100000;
        }
        break;

    case 4:
        M32(S, 0x84) += 0x14ccc;
        if (!(M32(entity, 0x3c) & 2)) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_zero);
            M32(MP(obj, 8), 0x50) = -16;
            vdp2_queue_attribute(14, GP(0x22700ad0), 0x25f00000, 0x80);
            obj = task_spawn_child_work88(task, (void *)scene_sprite_acceleration_task);
            (void)obj;
            M32(entity, 0x50) = 0x12c;
            TASK_S8(task, state) = 6;
            G32(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x100000;
        }
        break;

    case 5: {
        s32 c = M32(entity, 0x50) - 1;
        M32(entity, 0x50) = c;
        if (c == 0) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
            M32(MP(obj, 8), 0x50) = -16;
            TASK_S8(task, state) = 7;
        } else if (c == 0xf0) {
            hud_draw_level_record(0, 0, 5);
            hud_draw_level_record(0, 1, 12);
            hud_draw_level_record(0, 2, 13);
        }
        break;
    }

    case 6: {
        s32 c = M32(entity, 0x50) - 1;
        M32(entity, 0x50) = c;
        if (c == 0) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
            M32(MP(obj, 8), 0x50) = -16;
            TASK_S8(task, state) = 7;
        } else if (c == 0xf0) {
            hud_draw_level_record(0, 0, 5);
        }
        break;
    }

    case 7:
        if (!(M32(entity, 0x3c) & 2)) {
            void *e2;
            G32(0x06099578) = 0;
            REG_TVMD = (u16)(MU16(S, 0) & 0x7fff);
            e2 = MP(TASK_PTR(task, parent), 8);
            M32(e2, 0x3c) |= 1;
            sound_queue_push(DH_SOUND_TRANSITION_DONE_COMMAND);
            task_cancel_below_depth(0);
        }
        break;

    default:
        break;
    }

    /* common tail (.L202768) */
    (*(void (**)(void))(uintptr_t)0x06090298)();
    G32(0x0609869c) = 1;
}
