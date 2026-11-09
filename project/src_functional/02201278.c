/* display_stage_reset_sort [0x02201278]: Resets sort arenas and VDP1 command state before terminating the stage. */
#include "dh.h"
/* 0x02201278 - func_02201278: per-frame render pipeline, step 4 of 7.
 * Not called directly: entry 4 of the HWRAM step table at cart 0x28c278
 * (runtime 0x0608E278), dispatched by the frame executive (see 0220134c.c).
 * g_06098814 is the "current pipeline stage" marker.
 *   stage 4: func_02214dae (vdp1_set_erase)
 *   then push a 0 word onto the stream at *g_060E0DD0 (post-increment) and
 *   reset the stage marker to 0.
 * fidelity: HIGH - hand-checked vs asm 0x201278..0x20129a;
 *   bsrf target resolved from pool. */
extern s32 g_06098814;                   /* pipeline stage marker */
extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
void vdp1_reset_sort_arenas(void);                /* vdp1_set_erase */

void display_stage_reset_sort(void)
{
    s32 p;

    g_06098814 = 4;                      /* (bsrf delay slot) */
    vdp1_reset_sort_arenas();
    p = g_060E0DD0;                      /* push a 0 word onto the stream */
    g_060E0DD0 = p + 4;
    M32((void *)(uintptr_t)(u32)p, 0) = 0;
    g_06098814 = 0;                      /* (rts delay slot) */
}
