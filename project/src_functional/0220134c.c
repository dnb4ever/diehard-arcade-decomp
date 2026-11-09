/* display_stage_submit_frame [0x0220134c]: Runs display-list construction, sorting and VDP uploads for the final frame stages. */
#include "dh.h"
/* 0x0220134c - func_0220134c: per-frame render pipeline, final step group.
 * Not called directly: it is entry 7 of the HWRAM step table at cart 0x28c278
 * (runtime 0x0608E278: 0x0600521e, 0x0600523c, 0x0600525a, 0x06005278,
 * 0x060052e0, 0x06005320, 0x0600534c = this function), dispatched by the
 * frame executive. g_06098814 is the "current pipeline stage" marker; each
 * stage number is stored in the delay slot of the bsrf, i.e. before the call.
 *   stage 8 : displaylist_radix_sort   (0x02214e2c)
 *   stage 9 : slot distance tables     (0x02227ca8)
 *   stage 10: vdp1_build_cmdlist       (0x02214f90)
 *   stage 11: push a 0 word onto the stream at *g_060E0DD0 (post-increment)
 *   stage 0 : idle
 * fidelity: HIGH - 26 insns, hand-checked vs asm 0x20134c..0x201382; bsrf
 *   targets resolved from pool (0x13ad2, 0x26946, 0x13c26). */
extern s32 g_06098814;                   /* pipeline stage marker */
extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
void display_radix_sort_records(void);                /* displaylist_radix_sort */
void actor_build_proximity_tables(void);                /* slot distance tables */
void vdp1_upload_command_lists(void);                /* vdp1_build_cmdlist */

void display_stage_submit_frame(void)
{
    s32 p;

    g_06098814 = 8;
    display_radix_sort_records();
    g_06098814 = 9;
    actor_build_proximity_tables();
    g_06098814 = 10;
    vdp1_upload_command_lists();
    g_06098814 = 11;
    p = g_060E0DD0;
    g_060E0DD0 = p + 4;
    M32((void *)(uintptr_t)(u32)p, 0) = 0;
    g_06098814 = 0;
}
