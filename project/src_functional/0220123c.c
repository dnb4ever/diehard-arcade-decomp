/* display_stage_clear_sprite_tables [0x0220123c]: Clears sprite character tables and terminates this display command stage. */
#include "dh.h"
/* 0x0220123c - func_0220123c: per-frame render pipeline, step 2 of 7.
 * Not called directly: entry 2 of the HWRAM step table at cart 0x28c278
 * (runtime 0x0608E278), dispatched by the frame executive (see 0220134c.c).
 * g_06098814 is the "current pipeline stage" marker.
 *   calls func_022151d8 (clear_sprite_tables; no stage number is set)
 *   then push a 0 word onto the stream at *g_060E0DD0 (post-increment) and
 *   reset the stage marker to 0.
 * fidelity: HIGH - hand-checked vs asm 0x20123c..0x20125a;
 *   bsrf target resolved from pool. */
extern s32 g_06098814;                   /* pipeline stage marker */
extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
void sprite_clear_character_tables(void);                /* clear_sprite_tables */

void display_stage_clear_sprite_tables(void)
{
    s32 p;

    sprite_clear_character_tables();
    p = g_060E0DD0;                      /* push a 0 word onto the stream */
    g_060E0DD0 = p + 4;
    M32((void *)(uintptr_t)(u32)p, 0) = 0;
    g_06098814 = 0;                      /* (rts delay slot) */
}
