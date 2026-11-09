/* video_menu_bootstrap_sm [0x0223358c]: Existing map corroborated by source: 3-state(0,1,2). VDP2 shadow(*0x06098f08) setup; (src symbols_band234.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223358c - video/menu bootstrap + debug-text state machine (dispatch on task->unk10).
 *
 * entity = task->unk8.  frame[] flags live at g_06098734 (bits 1/2/4) and
 * g_06098794 (bits 0x10/0x20).  VDP2 = the VDP2 shadow-register block *0x06098f08.
 *
 *   state 0 (init): program the VDP2 shadow block (mode/colour/scroll fields),
 *       run the register/DMA helpers, install g_curActorObj's handler func_0220e2d8,
 *       spawn two child tasks (handler func_022348cc, then handler func_02234984 the
 *       VDP2 stage loader), register a glyph sheet (entity->unk64 = func_02227bc4(...)),
 *       emit 6 debug-text glyph strings through the variadic thunk *0x060902a0 (each
 *       site latches g_060997a4/a8 then pushes ONE descriptor pointer; the 6 pushes
 *       are reclaimed together by add #24,r15), advance task->unk10 to 1, enable the
 *       display plane, and write the grey/fade colour to VDP2 colour RAM 0x25e5fd00.
 *   state 1: func_0220ad58(70,2); if the cached frame id (entity->unk64->unk8->unk5E)
 *       changed, re-latch entity->unk30 and re-run the 3 resource helpers; when the
 *       g_06098734 flags 1&2&4 are all set, spawn child task func_022101ce, poke its
 *       sub-object (->unk8->unk50 = -24) and advance to state 2; when the g_06098794
 *       flags 0x10&0x20 are set, step the 0..15 fade ramp (entity->unk60) and rewrite
 *       colour RAM.
 *   state 2: once entity->unk3C bit1 is clear, clear the ready flags and set the
 *       downstream task (task->unk14->unk8)->unk3C bit0, then tail-call func_0220fdc8(0).
 *   other:  no-op.
 *
 * fidelity: HIGH - all 19 real computed branches resolved (braf_resolve); the one
 * flagged "bsrf r6 @0x233740" is a FALSE positive: 0x233738-0x2337af is this function's
 * literal pool (skipped by bra 0x2337b0), and 0x233740 is the constant 0x0603/0x0700
 * stored into VDP2->unkF8/unkFA - not a branch.  m2c corrections: the func_0220fece /
 * func_02227bc4 results (returned in r0) are the VALUES stored into entity->unk64 and
 * used as the spawned-task handle; m2c mis-rendered those as the pre-call mova pool
 * addresses.  Each glyph-thunk site passes exactly ONE stack argument (the descriptor);
 * all its register args are dead (clobbered by the preceding spawn calls), so the
 * thunk takes its two parameters from the latched globals plus the pushed descriptor. */

#define VDP2        (*(void *volatile *)DH_VDP2_REGISTER_SHADOW_ADDR)             /* VDP2 shadow register block */
#define GLYPH_THUNK (*(void (*volatile *)(void *))0x060902a0)   /* variadic debug-glyph output thunk */

extern void *g_curActorObj;                 /* 0x0609882c */
extern s32   g_glyph_param0;                /* 0x060997a4 - glyph thunk latch A */
extern s32   g_glyph_param1;                /* 0x060997a8 - glyph thunk latch B */
extern u8    g_0609861c;                    /* 0x0609861c - display-ready flag */
extern s32   g_06096c90;                    /* 0x06096c90 */
extern s32   g_060e1320;                    /* 0x060e1320 */
extern void *g_06091384[];                  /* 0x06091384 - per-unk30 resource table A */
extern void *g_06091454[];                  /* 0x06091454 - per-unk30 resource table B */
extern void *g_06091524[];                  /* 0x06091524 - per-unk30 resource table C */

extern void  video_reset_buffers(s32);
extern void  hud_setup_small_font(s32);
extern void  vdp2_apply_sprite_commands(void *);
extern void  vdp1_upload_texture_groups(void *);
extern void  vdp2_upload_pattern_groups(void *);
extern void  model_load_scene_records(void *);
extern void  actor_reset_slot_registry(void *);
extern void  effect_pool_reset_scene(void);
extern void *actor_spawn_fixed_slot(s32, void *);
extern void  stage_initialize_grids(s32, s32);
extern void *task_spawn_current_work6c(void *parent, void *handler);  /* spawn task; returns new task */
extern void  debug_draw_camera_pose(s32, s32);
extern void  task_cancel_below_depth(s32);
extern void  debug_orbit_camera_task(void *task);     /* handler installed into g_curActorObj */
extern void  vdp2_init_menu_graphics(void *task);     /* spawned task handler */
extern void  vdp2_stage_loader(void *task);     /* spawned task handler - VDP2 stage loader */
extern void  display_ramp_to_limit(void *task);     /* spawned task handler (state 1) */

void video_menu_bootstrap_sm(void *task) {
    void *entity = TASK_PTR(task, work);
    volatile s16 *flags124 = (volatile s16 *)DH_PLAYER_INPUT_RECORDS_ADDR;   /* frame flags, bits 1/2/4 */
    volatile s16 *flags30  = (volatile s16 *)0x06098794;   /* frame flags, bits 0x10/0x20 */

    switch (TASK_U8(task, state)) {
    case 0: {
        void *v;
        video_reset_buffers(-1);
        v = VDP2;
        MU16(v, 0xE)  = (MU16(v, 0xE) & ~0x300) | 0x300;
        MU16(v, 0xE)  = (MU16(v, 0xE) & ~0xFF) | 0x23;
        M32(v, 0x10)  = -1;
        M32(v, 0x14)  = 0x454FFFFF;
        M32(v, 0x18)  = -1;
        M32(v, 0x1C)  = 0x0137FFFF;
        hud_setup_small_font(7);
        (*(void (*volatile *)(void))0x06090294)();
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & ~0x30) | 0x10;
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & ~0x7) | 0x3;
        vdp2_apply_sprite_commands((void *)0x0608bcc8);
        v = VDP2; MU16(v, 0xE0) = (MU16(v, 0xE0) & ~0xF) | 0xF;
        v = VDP2; MU16(v, 0xE0) = MU16(v, 0xE0) & ~0x10;
        v = VDP2;
        MU16(v, 0xF8) = 0x603;
        MU16(v, 0xFA) = 0x700;
        MU16(v, 0xFC) = 4;
        MU16(v, 0xF0) = 5;
        MU16(v, 0xF2) = 0;
        MU16(v, 0xF4) = 0;
        MU16(v, 0xF6) = 0;
        MU16(v, 0) = (MU16(v, 0) & 0x7FFF) | 0x8000;
        M16(entity, 0x30) = 2;
        vdp1_upload_texture_groups(g_06091384[2]);                       /* unk30 == 2 here */
        vdp2_upload_pattern_groups(g_06091454[M16(entity, 0x30)]);
        model_load_scene_records(g_06091524[M16(entity, 0x30)]);
        MP(g_curActorObj, 0)  = (void *)debug_orbit_camera_task;      /* install handler */
        MU8(g_curActorObj, 0x10) = 0;
        actor_reset_slot_registry(g_curActorObj);
        effect_pool_reset_scene();
        MP(entity, 0x64) = actor_spawn_fixed_slot(0, (void *)0x022339f0);
        stage_initialize_grids(0, 1);
        task_spawn_current_work6c(task, (void *)vdp2_init_menu_graphics);
        task_spawn_current_work6c(task, (void *)vdp2_stage_loader);
        g_glyph_param0 = 35; g_glyph_param1 =  1; GLYPH_THUNK((void *)0x0608bef0);
        g_glyph_param0 =  5; g_glyph_param1 = 22; GLYPH_THUNK((void *)0x0608bf04);
        g_glyph_param0 =  5; g_glyph_param1 = 23; GLYPH_THUNK((void *)0x0608bf10);
        g_glyph_param0 =  5; g_glyph_param1 = 24; GLYPH_THUNK((void *)0x0608bf1c);
        g_glyph_param0 =  5; g_glyph_param1 = 25; GLYPH_THUNK((void *)0x0608bf28);
        g_glyph_param0 =  5; g_glyph_param1 = 26; GLYPH_THUNK((void *)0x0608bf34);
        g_0609861c      = 1;
        TASK_U8(task, state) = 1;
        g_060e1320      = 0;
        M32(entity, 0x60) = 0;
        v = VDP2; MU16(v, 0x110) |= 0x20;
        v = VDP2; MU16(v, 0x112) &= (u16)~0x20;
        v = VDP2; M32(v, 0xAC) = 0x2FE80;
        {
            s32 c = M32(entity, 0x60);
            *(volatile s16 *)0x25e5fd00 =
                (s16)(((c << 10) | (c << 5) | c) & 0x7FFF);
        }
        break;
    }
    case 1: {
        debug_draw_camera_pose(70, 2);
        if (M16(MP(MP(entity, 0x64), 8), 0x5E) != M16(entity, 0x30)) {
            M16(entity, 0x30) = M16(MP(MP(entity, 0x64), 8), 0x5E);
            vdp1_upload_texture_groups(g_06091384[M16(entity, 0x30)]);
            vdp2_upload_pattern_groups(g_06091454[M16(entity, 0x30)]);
            model_load_scene_records(g_06091524[M16(entity, 0x30)]);
        }
        {
            u16 f = (u16)flags124[0];
            if ((f & 1) && (f & 2) && (f & 4)) {
                void *child = task_spawn_current_work6c(task, (void *)display_ramp_to_limit);
                M32(MP(child, 8), 0x50) = -24;
                TASK_U8(task, state)++;
            }
        }
        {
            u16 f = (u16)flags30[0];
            if ((f & 0x10) && (f & 0x20)) {
                s32 c = M32(entity, 0x60);
                c = (c >= 15) ? 0 : c + 1;
                M32(entity, 0x60) = c;
                *(volatile s16 *)0x25e5fd00 =
                    (s16)(((c << 10) | (c << 5) | c) & 0x7FFF);
            }
        }
        break;
    }
    case 2:
        if (!(M32(entity, 0x3C) & 2)) {
            void *v = VDP2;
            MU16(v, 0) = MU16(v, 0) & 0x7FFF;
            g_0609861c = 0;
            g_06096c90 = 0;
            M32(MP(TASK_PTR(task, parent), 8), 0x3C) |= 1;
            task_cancel_below_depth(0);                               /* tail call */
        }
        break;
    default:
        break;
    }
}
