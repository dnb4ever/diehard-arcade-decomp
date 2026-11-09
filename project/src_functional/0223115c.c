/* obj_expand_draw4 [0x0223115c]: Existing map corroborated by source: void(void *actor). obj=*(actor+8). phase @+0x10: 0 seed (counter=ashl_var(30,g_06096C90)) FALLTHROUGH to 1; 1 grow/ease half-extents via 4 ashr_var; 2 clear actor. Draw tail: 4-quadrant sprite submit via func_022153f6(obj, obj->0x60, 1) (PIC jsr @r13 = mova(0x23129c)+0xFFFE415A -> 0x022153f6, 3-arg). 1 ashl + 4 ashr. Hand-verified. */
#include "dh.h"
/* 0x0023115c  obj_expand_draw4  (object pop-in animation, then submit a 4-quadrant
 * sprite via func_022153f6).
 *
 * Previously deferred: one call to the 0x24a360 shift-left helper (ashl_var) and
 * four to the 0x24a420 shift-right helper (ashr_var) via the non-standard r0/r1
 * bsrf ABI, plus a PIC computed call (jsr @r13) that m2c could not lower.
 * braf_resolve.py: 231188 -> 0x24a360; 2311c2/2311de/2311ee/2311fe -> 0x24a420.
 * The jsr target resolves to func_022153f6 (r13 = mova(0x23129c)+0xFFFE415A =
 * 0x2153F6, a 3-arg routine - it does not read r7, so the register left in r7 at
 * the call is irrelevant).
 *
 * obj = *(void**)(arg0+8).  Phase byte at arg0+0x10:
 *   phase 0: seed half-extents (0x14/0x1C = 0x2000, 0x18 = 0), a frame counter
 *            0x58 = ashl_var(30, g_06096C90), zoom 0x60 = 0xA0000, step 0x68 =
 *            0x1800, latch the base position (0x40/0x44/0x48 <- 0x8/0xC/0x10);
 *            advance phase; FALL THROUGH into phase 1.
 *   phase 1: grow zoom (0x60 += ashr_var(0x3800,n)); ease the step
 *            (0x68 -= ashr_var(0x180,n) while >0x400); expand the half-extents
 *            (0x14/0x1C += ashr_var(0x68,n), 0x18 += ashr_var(0x180,n)); when the
 *            frame counter 0x58 hits 0 (or global g_06098580 bit 0x20000 is set),
 *            advance phase.
 *   phase 2: clear *(s32*)arg0 and return without drawing.
 *   other:  draw only.
 * Draw tail (phases 0,1,>2): set tile 0x30 = ((frame>>1)&7)+0x188, then submit
 * four corner sprites (0x8/0xC/0x10 = base +/- half-extent, flip bits in 0x22)
 * via func_022153f6(obj, obj->0x60, 1); bump the frame counter 0x64.
 *
 * All shift counts are the runtime global g_06096C90, so the helper edge cases are
 * reachable; the helpers are called rather than inlined.
 *
 * fidelity: HIGH - switch/fallthrough, every field offset, the four draw
 * quadrants and all five shift calls read from ROM asm; shift targets confirmed
 * by braf_resolve.py, draw target by mova-delta resolution (func_022153f6, 3-arg). */
extern s32 g_06096C90;        /* scale/level driver (shift count) */
extern s32 g_scene_event_flags;        /* global state flags */
extern s32 shc_shift_left(s32 value, s32 count);   /* ashl_var: value << count */
extern s32 shc_shift_right_signed(s32 value, s32 count);   /* ashr_var: (signed)value >> count */
extern void sprite_draw_child(void *obj, s32 zoom, s32 mode);  /* submit one sprite */

void obj_expand_draw4(void *arg0) {
    void *obj = MP(arg0, 8);
    s32 ext;

    switch (M8(arg0, 0x10)) {
    case 0:
        M32(obj, 0x14) = 0x2000;
        M32(obj, 0x18) = 0;
        M32(obj, 0x1C) = 0x2000;
        M32(obj, 0x58) = shc_shift_left(30, g_06096C90);
        M32(obj, 0x60) = 0xA0000;
        M32(obj, 0x64) = 0;
        M32(obj, 0x68) = 0x1800;
        M32(obj, 0x40) = M32(obj, 8);
        M32(obj, 0x44) = M32(obj, 0xC);
        M32(obj, 0x48) = M32(obj, 0x10);
        M8(arg0, 0x10)++;
        /* fallthrough */
    case 1:
        M32(obj, 0x60) += shc_shift_right_signed(0x3800, g_06096C90);
        if (M32(obj, 0x68) > 0x400)
            M32(obj, 0x68) -= shc_shift_right_signed(0x0180, g_06096C90);
        ext = shc_shift_right_signed(M32(obj, 0x68), g_06096C90);
        M32(obj, 0x14) += ext;
        M32(obj, 0x18) += shc_shift_right_signed(0x0180, g_06096C90);
        M32(obj, 0x1C) += ext;
        if (--M32(obj, 0x58) == 0 || (g_scene_event_flags & 0x20000))
            M8(arg0, 0x10)++;
        break;
    case 2:
        M32(arg0, 0) = 0;
        return;
    default:
        break;
    }

    M16(obj, 0x30) = (s16)((((s32)M32(obj, 0x64) >> 1) & 7) + 0x188);
    M16(obj, 0x22) = 0;

    M32(obj, 0x8)  = M32(obj, 0x40) - M32(obj, 0x14);
    M32(obj, 0xC)  = M32(obj, 0x44) + M32(obj, 0x18);
    M32(obj, 0x10) = M32(obj, 0x48) - M32(obj, 0x1C);
    sprite_draw_child(obj, M32(obj, 0x60), 1);

    M32(obj, 0x8)  = M32(obj, 0x40) - M32(obj, 0x14);
    M32(obj, 0xC)  = M32(obj, 0x44) + M32(obj, 0x18);
    M32(obj, 0x10) = M32(obj, 0x48) + M32(obj, 0x1C);
    sprite_draw_child(obj, M32(obj, 0x60), 1);

    M16(obj, 0x22) = 0x10;

    M32(obj, 0x8)  = M32(obj, 0x40) + M32(obj, 0x14);
    M32(obj, 0xC)  = M32(obj, 0x44) + M32(obj, 0x18);
    M32(obj, 0x10) = M32(obj, 0x48) - M32(obj, 0x1C);
    sprite_draw_child(obj, M32(obj, 0x60), 1);

    M32(obj, 0x8)  = M32(obj, 0x40) + M32(obj, 0x14);
    M32(obj, 0xC)  = M32(obj, 0x44) + M32(obj, 0x18);
    M32(obj, 0x10) = M32(obj, 0x48) + M32(obj, 0x1C);
    sprite_draw_child(obj, M32(obj, 0x60), 1);

    M32(obj, 0x64) += 1;
}
