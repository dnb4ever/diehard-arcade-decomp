/* hud_update_player_meters [0x02237828]: Eases and renders the two player meter slots with scene-configured scaling. */
#include "dh.h"
/* 0x02237828 - per-frame update of the two timer/meter slots (array g_06098584,
 * slots 1 and 2). Sibling of func_022371c0 (which drives all 8 slots); this one
 * handles only the fixed pair. arg0->unk10 is the init/run state, arg0->unk8 a
 * control struct whose unk40 counter is pushed into VRAM via func_0221632e.
 * Each slot eases its unk8 toward a target = scale_target(g_06099788[slot-1]),
 * where the scale is an 8-way fixed-point multiplier (1.5x..4x) selected by a
 * global beat/mode byte g_060987ec[g_06098808]. The slot is then drawn through
 * func_02237be4 / func_02237e0e / func_02237e6c, with an optional glyph via the
 * variadic thunk g_060902a0 when g_06099796 != 1 && g_06098670 != 0.
 * branches resolved: bsrf r3 @0x237912 = func_0221632e (0x237912+4+0xfffdea18).
 * fidelity: HIGH. */
extern u8    g_06098584[];   /* slot array, 8 x 0x10-byte entries */
extern s32   g_06098604;     /* active-slot count */
extern s32   g_06098608;     /* active-slot bit mask */
extern s32   g_actor_slot_masks[];   /* per-id bit masks */
extern s32   g_06098568;     /* enable mask (input/credits) */
extern s16   g_06099788[];   /* per-slot target values ([slot-1]) */
extern u8    g_bios_status_buffer_legacy[];   /* beat/mode scale-select table */
extern s32   g_06098808;     /* index into g_060987ec */
extern s16   g_06099796;     /* suppress-glyph flag */
extern s32   g_play_session_mode;     /* glyph-enable flag */
extern s32   g_ui_cursor_column_legacy;     /* glyph dispatch param */
extern s32   g_ui_cursor_row_legacy;     /* glyph dispatch param */
extern s16   g_0609978c[];   /* glyph index table ([slot-1]) */
extern u32  *g_display_transfer_cursor;     /* VRAM command-buffer write pointer */
extern u8    g_0608c504[];   /* glyph format template */
extern void (*g_060902a0)(void *obj, ...);  /* variadic glyph thunk */
extern void  vdp2_queue_attribute(s32 a, u32 src, u32 dst, s32 n);
extern void  hud_draw_segmented_meter(void *slot);
extern void  hud_draw_meter_icon(void *slot, s32 idx);
extern void  hud_draw_meter_label(void *slot);

/* 8-way fixed-point scale of a target magnitude, selected by the global mode
 * byte; all shifts are arithmetic on the sign-extended 16-bit target. */
static s32 scale_target(s32 t) {
    s32 sel = ((u32)(u8)g_bios_status_buffer_legacy[g_06098808] >> 3) & 7;
    switch (sel) {
    case 0:  return (t >> 1) + t;                 /* 1.5x  */
    case 1:  return (t * 2) - (t >> 2);           /* 1.75x */
    case 2:  return t * 2;                        /* 2x    */
    case 3:  return (t * 2) + (t >> 2);           /* 2.25x */
    case 4:  return (t * 2) + (t >> 1);           /* 2.5x  */
    case 5:  return (t * 2) + (t >> 1) + (t >> 2); /* 2.75x */
    case 6:  return (t * 2) + t;                  /* 3x    */
    default: return t * 4;                        /* 4x    */
    }
}

static void slot_init(s32 slot, s32 substate, s16 value) {
    void *s = &g_06098584[slot * 0x10];
    M16(s, 0)   = 2;
    M16(s, 2)   = (s16)substate;
    M8(s, 4)    = 0;
    M8(s, 5)    = 0;
    M16(s, 6)   = 0;
    M32(s, 8)   = value;
    M32(s, 0xc) = 0;
}

void hud_update_player_meters(void *arg0) {
    void *ctrl = MP(arg0, 8);
    s32 i;

    switch (M8(arg0, 0x10)) {
    case 0:
        for (i = 0; i < 8; i++)
            M16(g_06098584, i * 0x10) = 0;
        g_06098604 = 0;
        g_06098608 = 0;
        if (g_actor_slot_masks[0] & g_06098568) {
            slot_init(1, 0, g_06099788[0]);
            g_06098608 |= g_actor_slot_masks[0] | g_actor_slot_masks[1];
        }
        if (g_actor_slot_masks[1] & g_06098568) {
            slot_init(2, 2, g_06099788[1]);
            g_06098608 |= g_actor_slot_masks[2] | g_actor_slot_masks[3];
        }
        M32(ctrl, 0x40) = 0;
        M8(arg0, 0x10)++;
        /* fallthrough */
    case 1:
        M32(ctrl, 0x40) += 3;
        vdp2_queue_attribute(14, (u32)(uintptr_t)g_display_transfer_cursor, 0x25F002D8u, 1);
        *g_display_transfer_cursor = (u32)((M32(ctrl, 0x40) & 0x1F) << 16);
        g_display_transfer_cursor += 1;   /* advance 4 bytes */

        for (i = 1; i <= 2; i++) {
            void *s = &g_06098584[i * 0x10];
            s32 target, prev, acc;

            if (M16(s, 0) == 0)
                continue;

            target = (s16)scale_target((s16)g_06099788[i - 1]);

            /* unk6 is a "stuck at zero" counter; reset while the target moves */
            M16(s, 6) = (target != 0) ? 0 : (s16)(M16(s, 6) + 1);

            prev = M32(s, 8);
            M32(s, 0xc) = prev;
            acc = prev;
            if (prev < target) {
                acc = prev + 8;
                M32(s, 8) = (acc <= target) ? (prev + 8) : target;
            } else if (prev > target) {
                acc = prev - target;
                switch (acc >> 3) {
                case 0:  M32(s, 8) -= 1; break;
                case 1:
                case 2:  M32(s, 8) -= 2; break;
                default: M32(s, 8) -= 8; break;
                }
            }

            if (target != 0) {
                M16(s, 6) = (M32(s, 8) > target) ? (s16)(M16(s, 6) ^ 1) : 0;
                hud_draw_segmented_meter(s);
                hud_draw_meter_icon(s, i);
                if (g_06099796 != 1 && g_play_session_mode != 0 && i <= 2) {
                    g_ui_cursor_column_legacy = ((M16(s, 2) & 3) * 0xA) + 2;
                    g_ui_cursor_row_legacy = ((M16(s, 2) >> 2) * 2) + 1;
                    g_060902a0(s, g_0608c504, g_0609978c[i - 1] - 1);
                }
            } else if (M16(s, 6) & 1) {
                M16(s, 6) ^= 1;
                hud_draw_segmented_meter(s);
                hud_draw_meter_icon(s, i);
                if (g_06099796 != 1 && g_play_session_mode != 0 && i <= 2) {
                    g_ui_cursor_column_legacy = ((M16(s, 2) & 3) * 0xA) + 2;
                    g_ui_cursor_row_legacy = ((M16(s, 2) >> 2) * 2) + 1;
                    g_060902a0(s, g_0608c504, g_0609978c[i - 1] - 1);
                }
            } else {
                M16(s, 6) ^= 1;
                hud_draw_meter_label(s);
            }
        }
        break;
    default:
        break;
    }
}
