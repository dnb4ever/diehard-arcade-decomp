/* obj_anim_step [0x02238e94]: Existing map corroborated by source: void(void *actor). obj=*(actor+8). phase byte @+0x10: 0=load start size, 1=load target + seed deltas (ashl_var(30,g_06096C90), ashr_var(4,g_06096C90)), 2=shrink per-frame & clear on expiry. All paths tail-call func_022386fc(obj+0x34). 1 ashl + 1 ashr. Hand-verified. */
#include "dh.h"
/* 0x00238e94  obj_anim_step  (per-object size/shrink animation state machine).
 *
 * Previously deferred: calls the 0x24a360 shift-left helper (ashl_var) and the
 * 0x24a420 shift-right helper (ashr_var) via the non-standard r0/r1 bsrf ABI,
 * and ends in a PIC tail-call (bra func_022386fc) that blocked m2c outright.
 * braf_resolve.py: 238ed4 -> 0x24a360, 238ee2 -> 0x24a420.
 *
 * obj = *(void**)(arg0+8).  A phase byte at arg0+0x10 selects:
 *   phase 0: load fixed start size (0x8C,0x8C,0x78) into obj->{0x34,0x36,0x38};
 *            advance phase.
 *   phase 1: load target size (0x78,0x78,0x64); seed the per-frame deltas
 *            obj->0x40 = ashl_var(30, g_06096C90) (frame count),
 *            obj->0x44 = ashr_var(4,  g_06096C90) (shrink step); advance phase.
 *   phase 2: shrink obj->{0x34,0x36,0x38} by obj->0x44 each frame; when the
 *            frame counter obj->0x40 hits 0, zero the sizes and clear
 *            *(s32*)arg0.
 *   other:  no state change.
 * All paths then re-sync the live size via func_022386fc(obj + 0x34).
 *
 * The shift counts are the runtime global g_06096C90, so the helper edge cases
 * (count<0 / count>=32) are reachable; the helpers are called, not inlined.
 *
 * fidelity: HIGH - switch, field offsets/widths and the two shift calls read
 * straight from ROM asm; tail-call target func_022386fc confirmed (void, m2c). */
extern s32 g_06096C90;                            /* global scale/level driver */
extern s32 shc_shift_left(s32 value, s32 count);   /* ashl_var: value << count */
extern s32 shc_shift_right_signed(s32 value, s32 count);   /* ashr_var: (signed)value >> count */
extern void display_accumulate_channel(void *p);                 /* copy/advance live size triple */

void obj_anim_step(void *arg0) {
    void *obj = MP(arg0, 8);

    switch (M8(arg0, 0x10)) {
    case 0:
        M16(obj, 0x34) = 0x8C;
        M16(obj, 0x36) = 0x8C;
        M16(obj, 0x38) = 0x78;
        M8(arg0, 0x10)++;
        break;
    case 1:
        M16(obj, 0x34) = 0x78;
        M16(obj, 0x36) = 0x78;
        M16(obj, 0x38) = 0x64;
        M32(obj, 0x40) = shc_shift_left(30, g_06096C90);
        M32(obj, 0x44) = shc_shift_right_signed(4, g_06096C90);
        M8(arg0, 0x10)++;
        break;
    case 2:
        M16(obj, 0x34) = (s16)(M16(obj, 0x34) - M32(obj, 0x44));
        M16(obj, 0x36) = (s16)(M16(obj, 0x36) - M32(obj, 0x44));
        M16(obj, 0x38) = (s16)(M16(obj, 0x38) - M32(obj, 0x44));
        if (--M32(obj, 0x40) == 0) {
            M16(obj, 0x34) = 0;
            M16(obj, 0x36) = 0;
            M16(obj, 0x38) = 0;
            M32(arg0, 0) = 0;
        }
        break;
    default:
        break;
    }

    display_accumulate_channel((void *)((u8 *)obj + 0x34));
}
