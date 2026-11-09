/* hud_tick_eight_meter_slots [0x022371c0]: Eight 0x10-byte meter/popup slots with live count and used-channel mask. */
#include "dh.h"
/* 0x022371c0 - full 8-slot HUD meter/popup manager (the big sibling of
 * func_02237828, which only drives the two fixed slots). Slot array g_06098584
 * holds eight 0x10-byte entries; g_06098604 is the live count and g_06098608 the
 * bit mask of used channels (indices into g_0604ebe4).
 * State 0 clears everything. State 1:
 *  - pushes the HUD digit command through func_0221632e;
 *  - retires one finished top popup (slots 3..7) into VRAM;
 *  - seeds the two fixed slots on demand (g_06099594/98 gates);
 *  - allocation pass (slots 3..7): for each enabled, empty slot, grab a free
 *    channel bit. If g_060995b0[i]->unkF bit 0x10 is clear it takes one bit;
 *    otherwise it reserves a pair (bits t3,t3+1) and, if an existing slot already
 *    owns the partner code t7, re-homes that slot to the next free bit;
 *  - update pass (slots 0..7): retire slots whose enable g_06099590[i] dropped,
 *    else ease unk8 toward a target (= g_060995b0[i]->unk8->unk56, scaled by the
 *    8-way beat multiplier for slots 0..2 only) and draw via func_02237be4 /
 *    func_02237e0e / func_02237e6c, with an optional glyph through g_060902a0.
 * branches resolved: the two jsr sites are func_0221632e (mova 0x237224 +
 * 0xfffdf10a); func_02237e6c confirmed 1-arg (reads only its slot ptr).
 * fidelity: HIGH. */
extern u8    g_06098584[];
extern s32   g_06098604;
extern s32   g_06098608;
extern u32  *g_display_transfer_cursor;
extern u8    g_0609860c[];
extern s32   g_actor_slot_masks[];
extern s32   g_06099594;
extern s32   g_06099598;
extern s16   g_06098594;
extern s16   g_060985a4;
extern u8    g_06079738[];
extern u8    g_bios_status_buffer_legacy[];
extern s32   g_06098808;
extern s16   g_06099796;
extern s32   g_play_session_mode;
extern s32   g_ui_cursor_column_legacy;
extern s32   g_ui_cursor_row_legacy;
extern u8    g_actor_slot_tasks_legacy[];   /* per-slot object pointer table */
extern s32   g_actor_slot_ids[];   /* per-slot enable flags */
extern s16   g_0609978c[];
extern u8    g_0608c504[];
extern void (*g_060902a0)(void *obj, ...);
extern void  vdp2_queue_attribute(s32 a, u32 src, u32 dst, s32 n);
extern void  hud_draw_segmented_meter(void *slot);
extern void  hud_draw_meter_icon(void *slot, s32 idx);
extern void  hud_draw_meter_label(void *slot);

static s32 scale_target(s32 t) {
    s32 sel = ((u32)(u8)g_bios_status_buffer_legacy[g_06098808] >> 3) & 7;
    switch (sel) {
    case 0:  return (t >> 1) + t;
    case 1:  return (t * 2) - (t >> 2);
    case 2:  return t * 2;
    case 3:  return (t * 2) + (t >> 2);
    case 4:  return (t * 2) + (t >> 1);
    case 5:  return (t * 2) + (t >> 1) + (t >> 2);
    case 6:  return (t * 2) + t;
    default: return t * 4;
    }
}

void hud_tick_eight_meter_slots(void *arg0) {
    void *ctrl = MP(arg0, 8);
    s32 i;

    switch (M8(arg0, 0x10)) {
    case 0:
        for (i = 0; i < 8; i++)
            M16(g_06098584, i * 0x10) = 0;
        g_06098604 = 0;
        g_06098608 = 0;
        M32(ctrl, 0x40) = 0;
        M8(arg0, 0x10)++;
        /* fallthrough */
    case 1:
        M32(ctrl, 0x40) -= 3;
        vdp2_queue_attribute(14, (u32)(uintptr_t)g_display_transfer_cursor, 0x25F002D8u, 1);
        *g_display_transfer_cursor = (u32)((M32(ctrl, 0x40) & 0x1F) << 16);
        g_display_transfer_cursor += 1;

        if (M32(g_0609860c, 4) == 0 && M32(g_0609860c, 0xC) == 0) {
            u32 vram = 0x25E66B00u;
            for (i = 3; i < 8; i++) {
                void *s = &g_06098584[i * 0x10];
                if (M16(s, 0) != 0 && M8(s, 4) != 0) {
                    M8(s, 4) = 0;
                    vdp2_queue_attribute(13, (u32)((M8(s, 5) << 8) + 0x224148A0), vram, 0x40);
                    break;
                }
                vram += 0x100;
            }
        }

        if (g_06099594 != 0 && g_06098594 == 0) {
            void *s = &g_06098584[0x10];
            M16(s, 0) = 2; M16(s, 2) = 0;
            M8(s, 4) = 0; M8(s, 5) = 0; M16(s, 6) = 0;
            M32(s, 8) = 0; M32(s, 0xC) = 0;
            g_06098608 |= g_actor_slot_masks[0] | g_actor_slot_masks[1];
        }
        if (g_06099598 != 0 && g_060985a4 == 0) {
            void *s = &g_06098584[0x20];
            M16(s, 0) = 2; M16(s, 2) = 2;
            M8(s, 4) = 0; M8(s, 5) = 0; M16(s, 6) = 0;
            M32(s, 8) = 0; M32(s, 0xC) = 0;
            g_06098608 |= g_actor_slot_masks[2] | g_actor_slot_masks[3];
        }

        /* allocation pass */
        for (i = 3; i < 8; i++) {
            void *s = &g_06098584[i * 0x10];
            void *obj = MP(g_actor_slot_tasks_legacy, i * 4);
            s32 count, mask, b;

            if (g_actor_slot_ids[i] == 0)
                continue;
            if (M16(s, 0) != 0)
                continue;

            count = g_06098604;
            mask = g_06098608;

            if (!(MU8(obj, 0xF) & 0x10)) {
                if (count >= 4)
                    continue;
                b = 4;
                while (mask & g_actor_slot_masks[b]) {
                    b++;
                    if (b >= 8) break;
                }
                M16(s, 0) = 1; M16(s, 2) = (s16)b; M8(s, 4) = 1;
                M8(s, 5) = g_06079738[MU8(obj, 0x12)];
                M16(s, 6) = 0; M32(s, 8) = 0; M32(s, 0xC) = 0;
                g_06098604++;
                g_06098608 |= g_actor_slot_masks[b];
            } else if (count < 3) {
                s32 t3, t7, k;
                b = 4;
                while (mask & g_actor_slot_masks[b]) {
                    b++;
                    if (b >= 8) break;
                }
                t3 = 6 & b;
                t7 = (1 & ~b) | t3;
                k = 3;
                while (!(M16(&g_06098584[k * 0x10], 0) != 0 &&
                         M16(&g_06098584[k * 0x10], 2) == t7)) {
                    k++;
                    if (k >= 8) break;
                }
                if (k < 8) {
                    void *ks = &g_06098584[k * 0x10];
                    s32 b2 = b + 1;
                    if (b2 < 8) {
                        while (mask & g_actor_slot_masks[b2]) {
                            b2++;
                            if (b2 >= 8) break;
                        }
                    }
                    hud_draw_meter_label(ks);
                    M16(ks, 2) = (s16)b2;
                    g_06098608 |= g_actor_slot_masks[b2];
                }
                M16(s, 0) = 2; M16(s, 2) = (s16)t3; M8(s, 4) = 1;
                M8(s, 5) = g_06079738[MU8(obj, 0x12)];
                M16(s, 6) = 0; M32(s, 8) = 0; M32(s, 0xC) = 0;
                g_06098604 += 2;
                g_06098608 |= g_actor_slot_masks[t3 + 1] | g_actor_slot_masks[t3];
            }
        }

        /* update / retire pass */
        for (i = 0; i < 8; i++) {
            void *s = &g_06098584[i * 0x10];

            if (M16(s, 0) == 0)
                continue;

            if (g_actor_slot_ids[i] == 0) {
                s32 u2 = M16(s, 2);
                hud_draw_meter_label(s);
                if (i >= 3)
                    g_06098604 -= M16(s, 0);
                if (M16(s, 0) == 1)
                    g_06098608 &= ~g_actor_slot_masks[u2];
                else
                    g_06098608 &= ~(g_actor_slot_masks[u2] | g_actor_slot_masks[u2 + 1]);
                M16(s, 0) = 0;
                continue;
            }

            {
                s32 base = M16(MP(MP(g_actor_slot_tasks_legacy, i * 4), 8), 0x56);
                s32 target = (i > 2) ? (s16)base : (s16)scale_target(base);
                s32 prev = M32(s, 8);

                M32(s, 0xC) = prev;
                if (prev < target) {
                    M32(s, 8) = (prev + 8 <= target) ? (prev + 8) : target;
                } else if (prev > target) {
                    switch ((prev - target) >> 3) {
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
                        g_060902a0(s, g_0608c504, M16(g_0609978c, (i - 1) * 2) - 1);
                    }
                } else if (M16(s, 6) & 1) {
                    M16(s, 6) ^= 1;
                    hud_draw_segmented_meter(s);
                    hud_draw_meter_icon(s, i);
                    if (g_06099796 != 1 && g_play_session_mode != 0 && i <= 2) {
                        g_ui_cursor_column_legacy = ((M16(s, 2) & 3) * 0xA) + 2;
                        g_ui_cursor_row_legacy = ((M16(s, 2) >> 2) * 2) + 1;
                        g_060902a0(s, g_0608c504, M16(g_0609978c, (i - 1) * 2) - 1);
                    }
                } else {
                    M16(s, 6) ^= 1;
                    hud_draw_meter_label(s);
                }
            }
        }
        break;
    default:
        break;
    }
}
