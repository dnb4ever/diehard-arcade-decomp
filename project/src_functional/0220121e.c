/* display_stage_clear_work [0x0220121e]: Clears render work buffers and terminates this display command stage. */
#include "dh.h"
/* 0x0220121e - func_0220121e: per-frame render pipeline, step 1 of 7.
 * Not called directly: entry 1 of the HWRAM step table at cart 0x28c278
 * (runtime 0x0608E278), dispatched by the frame executive (see 0220134c.c).
 * g_06098814 is the "current pipeline stage" marker.
 *   calls func_02223c20 (no stage number is set)
 *   then push a 0 word onto the stream at *g_060E0DD0 (post-increment) and
 *   reset the stage marker to 0.
 * fidelity: HIGH - hand-checked vs asm 0x20121e..0x20123c;
 *   bsrf target resolved from pool. */
extern s32 g_06098814;                   /* pipeline stage marker */
extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
void display_clear_work_heads(void);                /* 0x02223c20 */

void display_stage_clear_work(void)
{
    s32 p;

    display_clear_work_heads();
    p = g_060E0DD0;                      /* push a 0 word onto the stream */
    g_060E0DD0 = p + 4;
    M32((void *)(uintptr_t)(u32)p, 0) = 0;
    g_06098814 = 0;                      /* (rts delay slot) */
}
