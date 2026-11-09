/* display_stage_build_quads [0x02201320]: Builds the pending model quad list, advances its descriptor and terminates the stage. */
#include "dh.h"
/* 0x02201320 - func_02201320: per-frame render pipeline, step 6 of 7.
 * Not called directly: entry 6 of the HWRAM step table at cart 0x28c278
 * (runtime 0x0608E278), dispatched by the frame executive (see 0220134c.c).
 * g_06098814 is the "current pipeline stage" marker.
 *   stage 6: func_02215bf4 (no C yet; void(void), saves r8-r14, walks the
 *   buffer at g_060DA0C0); then g_060DA0C0 += 0x80; push a 0 word onto the
 *   stream at *g_060E0DD0 (post-increment); stage marker back to 0.
 * fidelity: HIGH - hand-checked vs asm 0x201320..0x20134c; bsrf target
 *   0x215bf4 resolved from pool (0x148c8). */
extern s32 g_06098814;                   /* pipeline stage marker */
extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
extern s32 g_060DA0C0;                   /* buffer cursor */
void model_build_pending_quad_list(void);                /* 0x02215bf4 (undecompiled) */

void display_stage_build_quads(void)
{
    s32 p;

    g_06098814 = 6;                      /* (bsrf delay slot) */
    model_build_pending_quad_list();
    g_060DA0C0 = g_060DA0C0 + 0x80;
    p = g_060E0DD0;                      /* push a 0 word onto the stream */
    g_060E0DD0 = p + 4;
    M32((void *)(uintptr_t)(u32)p, 0) = 0;
    g_06098814 = 0;                      /* (rts delay slot) */
}
