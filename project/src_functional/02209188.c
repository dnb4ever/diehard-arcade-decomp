/* results_task_dispatch [0x02209188]: Results/HUD dispatcher selects sibling states and resident-stat draw callbacks. */
#include "dh_work.h"
#include "dh.h"

/* 0x02209188 - func_02209188: results / HUD "task" dispatcher (spawned by
 * boot_display_mode_task, symbols.txt line 544).  This is the largest remaining
 * undone unit (fn.units member 0x20917a->real 0x209188 .. code-end 0x20a35a,
 * ~2017 words incl. literal pools; the 0x20917a..0x209186 preamble is trailing
 * pool of the previous unit - the real prologue is at 0x209188).
 *
 * STRUCTURE.  The unit is NOT one function: it is a dispatcher plus eight sibling
 * SHC functions laid down adjacently (each has its own prologue/epilogue).  They
 * were originally reproduced together in this file; each now lives in its own
 * <addr>.c (one function per file); this file keeps only the dispatcher.
 * Boundaries (hand-traced from prologues + rts/tails):
 *
 *   dispatcher  0x209188 .. 0x2093f6   rts @0x2093f4
 *   helper      0x2093f8 .. 0x20947e   rts @0x20947c  func_022093f8 (HUD header)
 *   state 1     0x2094c4 .. 0x2095ac   jmp @0x2095a8  func_022094c4
 *   state 2     0x2095cc .. 0x20980c   rts @0x20980a  func_022095cc
 *   state 3     0x209814 .. 0x209a54   rts @0x209a50  func_02209814
 *   state 4     0x209a8c .. 0x209d54   rts @0x209d50  func_02209a8c
 *   state 5     0x209d70 .. 0x20a0b0   rts @0x20a0ac  func_02209d70
 *   state 8     0x20a0b0 .. 0x20a200   rts @0x20a1fc  func_0220a0b0
 *   state 9     0x20a21c .. 0x20a35a   rts @0x20a356  func_0220a21c
 *
 * DISPATCH (0x209188).  arg0 = task; ent = task->unk8 (r10).  state = (s8)task->unk10.
 * If (u32)state >= 13 -> return (default).  Else braf jump table @0x2091b8 (13 s16
 * entries, target = 0x2091b6 + entry; decoded directly from ROM, verified):
 *   s0 ->0x2091d2 (inline init; sets state=1 and FALLS THROUGH into s1)
 *   s1 ->0x2094c4  s2 ->0x2095cc  s3 ->0x209814  s4 ->0x209a8c  s5 ->0x209d70
 *   s6 ->0x20a7dc (continue_prompt_handler)  s7 ->0x2093bc (inline, then tail
 *        kill_objects_above_id)  s8 ->0x20a0b0  s9 ->0x20a21c
 *   s10->0x20a390 (hud_stats_screen_build)  s11->0x20a4f0 (hud_tally_screen_build)
 *   s12->0x20a648 (hud_grid_screen_build)
 * States 1-9 are epilogue-then-branch TAIL CALLS (bra/jmp/braf) -> modelled as
 * `handler(task); return;`.  s6/s10/s11/s12 tail into other (already-named) units.
 *
 * The HUD helpers share a cursor + RAM draw-thunk convention (see sibling file
 * 0x0220a648.c and notes/symbols.txt):
 *   g_060997A4 = cursor color/x    g_060997A8 = cursor mode/y    g_060997AC = font
 *   (*g_0609029C)(desc)            glyph/descriptor draw thunk ("ui_glyph")
 *   (*g_060902A0)(fmt, ...)        formatted-text thunk          ("ui_printf")
 *   (*g_06000644)(op, addr, 0)     resident get-field fp; returns a u16 field
 *
 * CORRECTIONS vs m2c (tools/decomp/decomp_bounded.py):
 *  - m2c could not fold the braf dispatch (M2C_ERROR) and emitted the jump-table
 *    words as garbage code; the table is decoded by hand here.
 *  - `tst #0x80` zero-extends on hardware; m2c renders it as `& ~0x7F` (0xFF80).
 *    Corrected to `& 0x80` in func_022095cc (two player button-pressed sites).
 *  - all ui_printf sites are modelled (fmt, value) per the sibling convention;
 *    m2c's per-site arg lists/orders for *g_060902A0 are unreliable and were not
 *    used verbatim.
 *
 * fidelity: see the per-function "fidelity:" tags.  Dispatcher + helper +
 * states 1,2,3,5 are hand-traced HIGH.  States 4,8,9 are faithful reconstructions
 * (MEDIUM).  The state-3 per-frame voice selector and the state-5 stats-readout
 * body (previously deferred) are now fully decoded from the fn.py disassembly.
 */

/* ---- HUD cursor + RAM draw/field thunks ---- */
extern void (*g_06090294)(void);                         /* ui/display init thunk (state 0) */
extern void (*g_06000630)(void *cfg);                    /* BIOS slot (state 7) */
extern void (*g_06000634)(void *cfg);                    /* BIOS config probe (state 0) */
extern s32  (*g_0600063C)(void);                         /* BIOS work-buffer base (state 0) */

/* ---- misc HWRAM globals ---- */
extern void *g_vdp2Shadow;    /* 0x06098F08: VDP2 shadow-register block ptr */
extern void *g_curActorObj;   /* 0x0609882C: current scene/actor object ptr */
extern s32   g_06098804;      /* work-buffer base pointer */
extern s32   g_06098808;      /* config read cursor */
extern s32   g_play_session_mode;      /* scene/credit-armed flag */
extern s32   g_060986B8;      /* cleared in state 0 */
extern s32   g_sound_effects_enabled_legacy;      /* sound-enable flag */

/* ---- called / tail-called functions ---- */
extern void video_reset_buffers(s32 mode);            /* 0x216154 video_state_init */
extern void hud_setup_small_font(s32 mode);            /* 0x222d3c VDP2 text-mode setup */
extern void continue_prompt_handler(void *task);   /* 0x20a7dc (state 6)  */
extern void kill_objects_above_id(s32 id);         /* 0x20fdc8 (state 7)  */
extern void hud_stats_screen_build(void *task);    /* 0x20a390 (state 10) */
extern void hud_tally_screen_build(void *task);    /* 0x20a4f0 (state 11) */
extern void hud_grid_screen_build(void *task);     /* 0x20a648 (state 12) */

/* ---- state handlers (own files, split out of this TU) ---- */
void results_draw_header(s32 code, void *ent);
void results_draw_state1_labels(void *task);
void results_draw_state2_markers(void *task);
void results_tick_continue_countdown(void *task);
void results_edit_options(void *task);
void results_draw_final_stats(void *task);
void results_draw_state8_stats(void *task);
void results_draw_state9_stats(void *task);

/* raw-RAM access helpers */
#define RS32(a) (*(s32  *)(uintptr_t)(a))

/* ============================ dispatcher ============================ */
void results_task_dispatch(void *task)
{
    void *ent = TASK_PTR(task, work);               /* r10 */
    s32  state = TASK_S8(task, state);           /* mov.b, sign-extended */

    if ((u32)state >= 13u)
        return;                            /* default */

    switch (state) {
    case 0: {
        /* ---- state 0: one-shot init of the results/VDP2 scene ---- */
        void *P = g_vdp2Shadow;
        void *Q = g_curActorObj;

        video_reset_buffers(-1);                 /* video_state_init */

        M16(P, 0x0E) = (s16)((MU16(P, 0x0E) & ~0x300) | 0x300);
        M32(P, 0x10) = -1;
        M32(P, 0x14) = 0x454FFFFF;
        M32(P, 0x18) = -1;
        M32(P, 0x1C) = 0x0137FFFF;
        g_060986B8 = 0;
        hud_setup_small_font(7);                  /* VDP2 text-mode setup */
        g_06090294();                      /* ui/display init thunk */
        M16(P, 0xF8) = 0;
        M16(P, 0xFA) = 0x700;
        M16(P, 0xFC) = 0;
        M16(P, 0x00) = (s16)((MU16(P, 0x00) & 0x7FFF) | 0x8000);

        M32(Q, 0x00) = 0x0220B098;         /* &func_0220b098 (obj_rebuild_mtx; mova 0x209244+0x1E54) */
        M8 (Q, 0x10) = 0;

        g_play_session_mode = 1;
        g_sound_effects_enabled_legacy = 1;
        g_06000634((void *)(uintptr_t)DH_BIOS_STATUS_BUFFER_ADDR);
        g_06098804 = g_0600063C();
        g_06098808 = RS32(0x060511DC + ((((u32)RS32(0x06000248) >> 12) & 0xF) * 4)) * 2;

        M32((void *)(uintptr_t)0x060987D4, 0) = 0;   /* 4-byte clears (byte-wise in ROM) */
        M32((void *)(uintptr_t)DH_PLAYER_JOIN_STATES_ADDR, 0) = 0;

        M32(ent, 0x40) = 7;
        M32(ent, 0x44) = 1;
        TASK_S8(task, state) = 1;
    }
    /* state 0 continues straight into state 1 */
    __attribute__((fallthrough));
    case 1:  results_draw_state1_labels(task);         return;
    case 2:  results_draw_state2_markers(task);         return;
    case 3:  results_tick_continue_countdown(task);         return;
    case 4:  results_edit_options(task);         return;
    case 5:  results_draw_final_stats(task);         return;
    case 6:  continue_prompt_handler(task); return;   /* 0x20a7dc */
    case 7: {
        /* ---- state 7: tear down the results scene, free high-id objects ---- */
        void *obj = MP(TASK_PTR(task, parent), 8);
        g_play_session_mode = 0;
        g_sound_effects_enabled_legacy = 0;
        g_06000630((void *)(uintptr_t)DH_BIOS_STATUS_BUFFER_ADDR);
        M32(obj, 0x3C) |= 1;
        kill_objects_above_id(0);          /* tail call 0x20fdc8, r4=0 */
        return;
    }
    case 8:  results_draw_state8_stats(task);         return;
    case 9:  results_draw_state9_stats(task);         return;
    case 10: hud_stats_screen_build(task); return;    /* 0x20a390 */
    case 11: hud_tally_screen_build(task); return;    /* 0x20a4f0 */
    case 12: hud_grid_screen_build(task);  return;    /* 0x20a648 */
    default: return;
    }
}
/* fidelity: HIGH - prologue, jump table, state-0 init, state-7 block, and all 13
 * dispatch targets hand-traced vs fn.py disassembly (0x209188..0x2093f6). */
