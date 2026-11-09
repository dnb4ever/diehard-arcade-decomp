/* obj_retract_step [0x0221d176]: Existing map corroborated by source: void(void *actor). obj=*(actor+8). phase @+0x10: 0 latches 2 child ptrs (gated by g_06098568 bits 0/1) + counter=ashl_var(19,g_06096C90), FALLS THROUGH to 1; 1/2 pull each child's +0x10 coord in by ashr_var(const,g_06096C90). 1 ashl + 4 ashr. Hand-verified. */
#include "dh.h"
/* 0x0021d176  obj_retract_step  (per-object two-child retract state machine).
 *
 * Previously deferred: one call to the 0x24a360 shift-left helper (ashl_var) and
 * four to the 0x24a420 shift-right helper (ashr_var), all via the non-standard
 * r0/r1 bsrf ABI (value in r0 delay slot, count in r1).  braf_resolve.py:
 * 21d21c -> 0x24a360; 21d232/21d24c/21d27e/21d298 -> 0x24a420.
 *
 * obj = *(void**)(arg0+8).  Phase byte at arg0+0x10 selects:
 *   phase 0: optionally latch two child pointers from globals (gated by bits 0/1
 *            of g_06098568), seed the frame counter obj->0x20 =
 *            ashl_var(19, g_06096C90), then FALL THROUGH into phase 1.
 *   phase 1: for each present child, pull its coordinate (child->0x10) in by
 *            ashr_var(0x1000, g_06096C90) / ashr_var(0x0C00, g_06096C90); when
 *            the counter obj->0x20 reaches 0, reset it to 2 and advance phase.
 *   phase 2: pull both children in by ashr_var(0x0800, g_06096C90); when the
 *            counter reaches 0, clear *(s32*)arg0.
 *   other:  nothing.
 *
 * All shift counts are the runtime global g_06096C90, so the helper edge cases
 * are reachable; the helpers are called rather than inlined.
 *
 * fidelity: HIGH - switch (with the 0->1 fallthrough), flag gates, child-pointer
 * indirection and all five shift calls read from ROM asm; shift targets
 * confirmed by braf_resolve.py. */
extern s32   g_06096C90;        /* scale/level driver (shift count) */
extern s32   g_06098568;        /* child-present flag bits */
extern void *g_060995B4;        /* child-0 source record */
extern void *g_060995B8;        /* child-1 source record */
extern s32 shc_shift_left(s32 value, s32 count);   /* ashl_var: value << count */
extern s32 shc_shift_right_signed(s32 value, s32 count);   /* ashr_var: (signed)value >> count */

void obj_retract_step(void *arg0) {
    void *obj = MP(arg0, 8);

    switch (M8(arg0, 0x10)) {
    case 0:
        if (g_06098568 & 1)
            MP(obj, 0x30) = MP(g_060995B4, 8);
        if (g_06098568 & 2)
            MP(obj, 0x34) = MP(g_060995B8, 8);
        M8(arg0, 0x10)++;
        M32(obj, 0x20) = shc_shift_left(19, g_06096C90);
        /* fallthrough */
    case 1:
        if (MP(obj, 0x30) != (void *)0)
            M32(MP(obj, 0x30), 0x10) -= shc_shift_right_signed(0x1000, g_06096C90);
        if (MP(obj, 0x34) != (void *)0)
            M32(MP(obj, 0x34), 0x10) -= shc_shift_right_signed(0x0C00, g_06096C90);
        if (--M32(obj, 0x20) == 0) {
            M32(obj, 0x20) = 2;
            M8(arg0, 0x10)++;
        }
        break;
    case 2:
        if (MP(obj, 0x30) != (void *)0)
            M32(MP(obj, 0x30), 0x10) -= shc_shift_right_signed(0x0800, g_06096C90);
        if (MP(obj, 0x34) != (void *)0)
            M32(MP(obj, 0x34), 0x10) -= shc_shift_right_signed(0x0800, g_06096C90);
        if (--M32(obj, 0x20) == 0)
            M32(arg0, 0) = 0;
        break;
    default:
        break;
    }
}
