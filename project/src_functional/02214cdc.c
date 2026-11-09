/* vdp1_begin_frame [0x02214cdc]: Writes plot trigger and framebuffer erase-window registers. */
#include "dh_band21.h"
/* 0x02214cdc (vdp1_frame_begin) - start a VDP1 frame. Sets the frame-active flag
 * g_060E0E30 = 1 and writes the end code 0x8000 to the command buffer word at
 * 0x060C3CC0. Programs the VDP1 erase/draw registers: PTMR (0x25D00004) = 2 (plot
 * on next field), EWDR (0x25D00006) = 0, EWLR (0x25D00008) = 0, EWRR (0x25D0000A) =
 * 0x58F0 (erase window to full screen). Then fills four halfwords of 0x0505 at byte
 * offsets 0xF0..0xF6 of the command table *0x06098F08. Calls func_02214dae to
 * terminate the current command list and tail-calls func_0223964e to kick the draw.
 * NOTE: fn.extent over-runs this entry (braf not treated as a terminator); the real
 * function ends at the tail branch, so this models only 0x214cdc`s own body.
 * fidelity: HIGH - register pokes, widths (mov.w) and the four 0x0505 fills verified. */
extern void vdp1_reset_sort_arenas(void);   /* terminate VDP1 command list */
extern void debug_reset_model_preview(void);   /* VDP1 draw start (tail call) */

void vdp1_begin_frame(void) {
    u16 *cmd;

    MEMU16(0x060E0E30) = 1;
    MEMU16(0x060C3CC0) = 0x8000;
    MEMU16(VDP1_PTMR_ADDR) = 2;
    MEMU16(VDP1_EWDR_ADDR) = 0;
    MEMU16(VDP1_EWLR_ADDR) = 0;
    MEMU16(VDP1_EWRR_ADDR) = 0x58F0;

    cmd = *(u16 **)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR;
    cmd[0x78] = 0x0505;
    cmd[0x79] = 0x0505;
    cmd[0x7A] = 0x0505;
    cmd[0x7B] = 0x0505;

    vdp1_reset_sort_arenas();
    debug_reset_model_preview();
}
