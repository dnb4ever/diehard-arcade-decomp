/* video_menu_bootstrap_sm3 [0x02236e66]: Existing map corroborated by source: sibling. s0: install func_0220e29e, spawn func_02236af0, (src symbols_band234.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02236e66 - video/menu bootstrap + debug-text state machine (dispatch on task->unk10).
 * Third sibling of func_0223358c / func_0223673a: a VDP2 shadow-register bootstrap that
 * spawns the debug editor and shows a labelled read-out.
 *
 * entity = task->unk8.  VDP2 = *0x06098f08.  Frame flags at 0x06098734 (bits 1/2/4,
 * advance) and 0x06098794 (bits 0x10/0x20, fade).
 *
 *   state 0 (init): clear g_06099578, program the VDP2 shadow block, install
 *       g_curActorObj's handler func_0220e29e, spawn a child task (handler func_02236af0
 *       the editor), emit NINE debug-text labels through the variadic glyph thunk
 *       *0x060902a0 (each site latches g_060997a4/a8 and pushes ONE descriptor; 9 pushes
 *       reclaimed by add #36,r15), set g_06099578, advance task->unk10 to 1, enable the
 *       display plane, write colour RAM, then fall through to state 1.
 *   state 1: when the 0x06098734 flags 1&2&4 are set advance to state 2; when the
 *       0x06098794 flags 0x10&0x20 are set step the 0..15 fade ramp (entity->unk60) and
 *       rewrite colour RAM 0x25e5fd00.
 *   state 2: spawn child task func_022101ce, poke its sub-object (->unk8->unk50 = -0x20),
 *       set entity->unk3C bit1, advance to state 3, then fall through.
 *   state 3: once entity->unk3C bit1 is clear, set (task->unk14->unk8)->unk3C bit0 and
 *       tail-call func_0220fdc8(0).
 *   other:  no-op.
 *
 * fidelity: HIGH - all 6 computed branches resolved (braf_resolve); 0x236eac-0x236ee2
 * is the literal pool (skipped by the dispatch bra).  m2c corrections: every glyph site
 * takes 1 stack arg (m2c reported 3 at the first by counting dead leftover registers);
 * the state-2 spawn result (r0->r4) is the task whose sub-object is poked (m2c rendered
 * it as the raw mova pool offset); handler/spawn addresses are mova+delta results. */

#define VDP2   (*(void *volatile *)DH_VDP2_REGISTER_SHADOW_ADDR)
#define GLYPH1 (*(void (*volatile *)(void *))0x060902a0)        /* 1 stack arg / site */

extern void *g_curActorObj;                 /* 0x0609882c */
extern s32   g_glyph_param0;                /* 0x060997a4 */
extern s32   g_glyph_param1;                /* 0x060997a8 */
extern s32   g_06099578;                    /* 0x06099578 */

extern void  video_reset_buffers(s32);
extern void  hud_setup_small_font(s32);
extern void  vdp1_trigger_draw(void);
extern void  task_spawn_child_work88(void *parent, void *handler);  /* spawn task */
extern void *task_spawn_current_work6c(void *parent, void *handler);  /* spawn task; returns new task */
extern void  task_cancel_below_depth(s32);
extern void  debug_initialize_world_pose_task(void *task);     /* handler installed into g_curActorObj */
extern void  debug_cursor_editor(void *task);     /* spawned task handler - debug editor */
extern void  display_ramp_to_limit(void *task);     /* spawned task handler (state 2) */

void video_menu_bootstrap_sm3(void *task) {
    void *entity = TASK_PTR(task, work);
    volatile s16 *flags124 = (volatile s16 *)DH_PLAYER_INPUT_RECORDS_ADDR;   /* @r15 (sp0) */
    volatile s16 *flags30  = (volatile s16 *)0x06098794;   /* @(4,r15) (sp4) */

    switch (TASK_U8(task, state)) {
    case 0: {
        void *v;
        g_06099578 = 0;
        video_reset_buffers(-1);
        v = VDP2; M32(v, 0x10) = 0x454FFFFF;
        v = VDP2; M32(v, 0x18) = 0x0137FFFF;
        v = VDP2;
        MU16(v, 0xF8) = 0x707;
        MU16(v, 0xFA) = 0x707;
        MU16(v, 0xFC) = 0;
        hud_setup_small_font(7);
        (*(void (*volatile *)(void))0x06090294)();
        vdp1_trigger_draw();
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & ~0x30) | 0x10;
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & ~0x7) | 0x3;
        v = VDP2; MU16(v, 0xE0) = (MU16(v, 0xE0) & ~0xF) | 0xF;
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & 0x7FFF) | 0x8000;
        MP(g_curActorObj, 0) = (void *)debug_initialize_world_pose_task;
        MU8(g_curActorObj, 0x10) = 0;
        task_spawn_child_work88(task, (void *)debug_cursor_editor);
        g_glyph_param0 = 35; g_glyph_param1 =  1; GLYPH1((void *)0x0608c240);
        g_glyph_param0 = 70; g_glyph_param1 = 10; GLYPH1((void *)0x0608c254);
        g_glyph_param0 = 70; g_glyph_param1 = 11; GLYPH1((void *)0x0608c25c);
        g_glyph_param0 = 70; g_glyph_param1 = 12; GLYPH1((void *)0x0608c264);
        g_glyph_param0 = 70; g_glyph_param1 = 14; GLYPH1((void *)0x0608c26c);
        g_glyph_param0 = 71; g_glyph_param1 = 15; GLYPH1((void *)0x0608c274);
        g_glyph_param0 = 71; g_glyph_param1 = 16; GLYPH1((void *)0x0608c27c);
        g_glyph_param0 = 71; g_glyph_param1 = 17; GLYPH1((void *)0x0608c284);
        g_glyph_param0 = 70; g_glyph_param1 = 18; GLYPH1((void *)0x0608c28c);
        g_06099578 = 1;
        TASK_U8(task, state)++;
        M32(entity, 0x60) = 0;
        v = VDP2; MU16(v, 0x110) |= 0x20;
        v = VDP2; MU16(v, 0x112) &= (u16)~0x20;
        v = VDP2; M32(v, 0xAC) = 0x2FE80;
        {
            s32 c = M32(entity, 0x60);
            *(volatile s16 *)0x25e5fd00 =
                (s16)(((c << 10) | (c << 5) | c) & 0x7FFF);
        }
    }
    /* fallthrough */
    case 1: {
        u16 f = (u16)flags124[0];
        if ((f & 1) && (f & 2) && (f & 4)) TASK_U8(task, state)++;
        {
            u16 g = (u16)flags30[0];
            if ((g & 0x10) && (g & 0x20)) {
                s32 c = M32(entity, 0x60);
                c = (c >= 15) ? 0 : c + 1;
                M32(entity, 0x60) = c;
                *(volatile s16 *)0x25e5fd00 =
                    (s16)(((c << 10) | (c << 5) | c) & 0x7FFF);
            }
        }
        break;
    }
    case 2: {
        void *child = task_spawn_current_work6c(task, (void *)display_ramp_to_limit);
        M32(MP(child, 8), 0x50) = -0x20;
        M32(entity, 0x3C) |= 2;
        TASK_U8(task, state)++;
    }
    /* fallthrough */
    case 3:
        if (!(M32(entity, 0x3C) & 2)) {
            M32(MP(TASK_PTR(task, parent), 8), 0x3C) |= 1;
            task_cancel_below_depth(0);                   /* tail call */
        }
        break;
    default:
        break;
    }
}
