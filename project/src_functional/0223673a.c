/* video_menu_bootstrap_sm2 [0x0223673a]: Existing map corroborated by source: sibling of 0223358c. s0: install func_0220e1e2, (src symbols_band234.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223673a - video/menu bootstrap + debug-text state machine (dispatch on task->unk10).
 * Sibling of func_0223358c: a second VDP2 shadow-register bootstrap that drives a block
 * of debug glyph strings and a g_curActorObj position read-out.
 *
 * entity = task->unk8.  VDP2 = *0x06098f08 (shadow-register block).  Frame flags live
 * at 0x06098734 (bits 1/2/4, advance) and 0x06098794 (bits 0x10/0x20, fade).
 *
 *   state 0 (init): program the VDP2 shadow block, install g_curActorObj's handler
 *       func_0220e1e2, spawn a child task (handler func_022363e4 the inspector), emit
 *       TWELVE debug-text labels through the variadic glyph thunk *0x060902a0 (each site
 *       latches g_060997a4/a8 and pushes ONE descriptor; 12 pushes reclaimed by
 *       add #48,r15), advance task->unk10 to 1, enable the display plane, write colour
 *       RAM, then fall through into state 1.
 *   state 1: emit THREE value labels showing g_curActorObj->unk8->{unk8,unkC,unk10}
 *       (each site pushes TWO stack args: descriptor + value; 6 pushes = add #24,r15);
 *       when the 0x06098734 flags 1&2&4 are set advance to state 2; when the 0x06098794
 *       flags 0x10&0x20 are set step the 0..15 fade ramp (entity->unk60) and rewrite
 *       colour RAM 0x25e5fd00.
 *   state 2: VDP2->unk0 &= 0x7fff; set (task->unk14->unk8)->unk3C bit0; tail-call
 *       func_0220fdc8(0).
 *   other:  no-op.
 *
 * fidelity: HIGH - all 5 computed branches resolved (braf_resolve); 0x23677e-0x2367c6
 * is the literal pool (skipped by the dispatch bra).  m2c corrections: the state-0 and
 * state-1 glyph arg counts (m2c reported 3/1 by counting dead leftover registers) -
 * state-0 sites take 1 stack arg each, state-1 sites take 2; handler/spawn addresses
 * are the mova+delta results m2c rendered as raw pool offsets. */

#define VDP2   (*(void *volatile *)DH_VDP2_REGISTER_SHADOW_ADDR)                  /* VDP2 shadow register block */
#define GLYPH1 (*(void (*volatile *)(void *))0x060902a0)        /* 1 stack arg / site */
#define GLYPH2 (*(void (*volatile *)(void *, s32))0x060902a0)   /* 2 stack args / site */

extern void *g_curActorObj;                 /* 0x0609882c */
extern s32   g_glyph_param0;                /* 0x060997a4 - glyph thunk latch A */
extern s32   g_glyph_param1;                /* 0x060997a8 - glyph thunk latch B */
extern s32   g_06099578;                    /* 0x06099578 */

extern void  video_reset_buffers(s32);
extern void  hud_setup_small_font(s32);
extern void  vdp1_trigger_draw(void);
extern void  task_spawn_child_work88(void *parent, void *handler);  /* spawn task */
extern void  task_cancel_below_depth(s32);
extern void  debug_move_world_pose_task(void *task);     /* handler installed into g_curActorObj */
extern void  debug_obj_inspector(void *task);     /* spawned task handler - debug inspector */

void video_menu_bootstrap_sm2(void *task) {
    void *entity = TASK_PTR(task, work);

    switch (TASK_U8(task, state)) {
    case 0: {
        void *v;
        video_reset_buffers(-1);
        v = VDP2; MU16(v, 0xE0) = MU16(v, 0xE0) & (u16)~0x10;
        v = VDP2; M32(v, 0x10) = 0x454FFFFF;
        v = VDP2; M32(v, 0x18) = 0x0137FFFF;
        v = VDP2;
        MU16(v, 0xF8) = 0x707;
        MU16(v, 0xFA) = 0x707;
        MU16(v, 0xFC) = 0;
        hud_setup_small_font(7);
        (*(void (*volatile *)(void))0x06090294)();
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & 0x7FFF) | 0x8000;
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & ~0x30) | 0x10;
        v = VDP2; MU16(v, 0) = (MU16(v, 0) & ~0x7) | 0x3;
        v = VDP2; MU16(v, 0xE0) = (MU16(v, 0xE0) & ~0xF) | 0xF;
        vdp1_trigger_draw();
        task_spawn_child_work88(task, (void *)debug_obj_inspector);
        MP(g_curActorObj, 0) = (void *)debug_move_world_pose_task;
        MU8(g_curActorObj, 0x10) = 0;
        g_glyph_param0 = 35; g_glyph_param1 =  1; GLYPH1((void *)0x0608c17c);
        g_glyph_param0 = 70; g_glyph_param1 = 10; GLYPH1((void *)0x0608c190);
        g_glyph_param0 = 70; g_glyph_param1 = 12; GLYPH1((void *)0x0608c19c);
        g_glyph_param0 = 71; g_glyph_param1 = 13; GLYPH1((void *)0x0608c1a4);
        g_glyph_param0 = 71; g_glyph_param1 = 14; GLYPH1((void *)0x0608c1ac);
        g_glyph_param0 = 71; g_glyph_param1 = 15; GLYPH1((void *)0x0608c1b4);
        g_glyph_param0 = 70; g_glyph_param1 = 17; GLYPH1((void *)0x0608c1bc);
        g_glyph_param0 = 71; g_glyph_param1 = 18; GLYPH1((void *)0x0608c1c4);
        g_glyph_param0 = 71; g_glyph_param1 = 19; GLYPH1((void *)0x0608c1cc);
        g_glyph_param0 = 71; g_glyph_param1 = 20; GLYPH1((void *)0x0608c1d4);
        g_glyph_param0 = 70; g_glyph_param1 = 22; GLYPH1((void *)0x0608c1dc);
        g_glyph_param0 = 70; g_glyph_param1 = 23; GLYPH1((void *)0x0608c1e4);
        TASK_U8(task, state)++;
        /* common block (shared with the state-0 -> state-1 fall-through) */
        g_06099578 = 1;
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
        volatile s16 *flags124 = (volatile s16 *)DH_PLAYER_INPUT_RECORDS_ADDR;
        volatile s16 *flags30  = (volatile s16 *)0x06098794;
        void *actor = MP(g_curActorObj, 8);
        g_glyph_param0 = 78; g_glyph_param1 = 18; GLYPH2((void *)0x0608c1ec, M32(actor, 8));
        g_glyph_param0 = 78; g_glyph_param1 = 19; GLYPH2((void *)0x0608c1ec, M32(actor, 0xC));
        g_glyph_param0 = 78; g_glyph_param1 = 20; GLYPH2((void *)0x0608c1ec, M32(actor, 0x10));
        {
            u16 f = (u16)flags124[0];
            if ((f & 1) && (f & 2) && (f & 4)) TASK_U8(task, state)++;
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
    case 2: {
        void *v = VDP2;
        MU16(v, 0) = MU16(v, 0) & 0x7FFF;
        M32(MP(TASK_PTR(task, parent), 8), 0x3C) |= 1;
        task_cancel_below_depth(0);                       /* tail call */
        break;
    }
    default:
        break;
    }
}
