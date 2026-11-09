/* display_stage_reset_cursors [0x022012e0]: Terminates the stream and resets frame buffer and command cursors. */
#include "dh.h"
/* 0x022012e0 - func_022012e0: per-frame render pipeline, step 5 of 7.
 * Not called directly: entry 5 of the HWRAM step table at cart 0x28c278
 * (runtime 0x0608E278), dispatched by the frame executive (see 0220134c.c).
 * g_06098814 is the "current pipeline stage" marker.
 *   stage 5: terminate the stream with a 0 word at the current cursor (no
 *   increment), then reset the per-frame buffers:
 *     g_060CF0C4 = 0x060C3CC4, g_060DA0C0 = 0x060D6040,
 *     g_060E0DD0 = g_060E0DD4 = 0x060E0BD0 (stream base);
 *   func_02215000 (vdp1_cmdlist_header_init); g_060987C0++; g_060987C4 = 0;
 *   stage marker back to 0.
 * fidelity: HIGH - hand-checked vs asm 0x2012e0..0x201320 (store order kept:
 *   the g_060E0DD4 store is in the bsrf delay slot, before the call). */
extern s32 g_06098814;                   /* pipeline stage marker */
extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
extern s32 g_060E0DD4;                   /* stream base / tail copy */
extern s32 g_060CF0C4;                   /* end pointer of entity array 1 */
extern s32 g_060DA0C0;                   /* buffer cursor (reset per frame) */
extern s32 g_060987C0;                   /* frame counter */
extern s32 g_060987C4;
void vdp1_append_frame_headers(void);                /* vdp1_cmdlist_header_init */

void display_stage_reset_cursors(void)
{
    g_06098814 = 5;
    M32((void *)(uintptr_t)(u32)g_060E0DD0, 0) = 0;
    g_060CF0C4 = 0x060C3CC4;
    g_060DA0C0 = 0x060D6040;
    g_060E0DD0 = 0x060E0BD0;
    g_060E0DD4 = 0x060E0BD0;             /* (bsrf delay slot) */
    vdp1_append_frame_headers();
    g_060987C0 = g_060987C0 + 1;
    g_060987C4 = 0;
    g_06098814 = 0;                      /* (rts delay slot) */
}
