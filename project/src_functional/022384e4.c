/* sound_tick_slot_sequence [0x022384e4]: Three task states use owner/slot records and queued sound IDs. */
#include "dh.h"
/* 0x022384e4 - slot/record state machine (byte state at arg0+0x10 drives a
 * 3-way switch). Reads record arg0->[8], container arg0->[0x14]->[8].
 * Table lookups via 0x06099590 (u32 pair; +0 -> "active" flag, +8 in the
 * second entry -> r10). Sound requests pushed through sound_queue_push
 * (cart 0x215f6c) as (s16 field at rec+0x32 << 8) | 0xA00000FF when
 * g_060e0ddc != 0. func_0223845c(rec, r12) consulted in state 2 / kind 2.
 * Stores 0 (u32) at arg0+0 to deactivate. fidelity: HIGH. */
extern volatile s32 g_sound_effects_enabled;
extern s32 sound_step_command_cursor(u32 a, u32 b);
extern void sound_queue_push(s32 req);

void sound_tick_slot_sequence(u32 arg0) {
    u32 rec, sp8, r12, idx, r4, r10;
    s16 r7;
    s32 sp0, st;

    rec = MU32(arg0, 8);
    sp8 = MU32(arg0, 0x14);
    r12 = MU32(sp8, 8);
    idx = (u32)((s32)M16(rec, 0x30)) << 2;
    r4  = MU32(DH_ACTOR_SLOT_IDS_ADDR, idx);
    r10 = MU32(MU32(DH_ACTOR_SLOT_TASKS_ADDR, idx), 8);
    r7  = M16(rec, 0x62);
    sp0 = M32(rec, 0x40);
    st  = (s32)(s8)M8(arg0, 0x10);

    if (st == 0) {
        u32 a5a = (u32)(u16)M16(r12, 0x5A);
        u32 e   = 0x0607d124 + a5a * 28;
        u32 g   = 0x060541f0 + (u32)(u8)M8(e, 0xF) * 8;
        u32 data = MU32(g, 4);
        s16 r7v  = M16(g, 2);
        s16 kind = M16(g, 0);

        M16(rec, 0x30) = M8(r12, 0x34);
        M32(rec, 0x60) = a5a;
        M32(rec, 0x64) = data;

        if (kind == 0) {
            s32 b;
            M16(rec, 0x32) = r7v;
            b = (s32)(s8)M8(data, 0);
            M32(rec, 0x40) = b;
            if (b == 0 && g_sound_effects_enabled != 0)
                sound_queue_push(((s32)M16(rec, 0x32) << 8) | DH_SOUND_CUE_BASE);
            M8(arg0, 0x10) = 1;
        } else if (kind == 1) {
            u32 g3 = 0x06054080 +
                     (s32)(s8)M8(0x060540e8 + (s32)M16(r12, 0x5E), 0) * 8;
            s32 b;
            M16(rec, 0x32) =
                M16(0x06053f04, (u32)(u8)M8(g3 + (s32)(s16)r7v, 0) * 2);
            b = (s32)(s8)M8(data, 0);
            M32(rec, 0x40) = b;
            if (b == 0 && g_sound_effects_enabled != 0)
                sound_queue_push(((s32)M16(rec, 0x32) << 8) | DH_SOUND_CUE_BASE);
            M8(arg0, 0x10) = 1;
        } else if (kind == 2) {
            s32 b;
            b = (s32)(s8)M8(MU32(rec, 0x64), 0);
            M32(rec, 0x40) = b;
            if (b != 0) {
                M8(arg0, 0x10) = 2;
            } else if (sound_step_command_cursor(rec, r12) == 0) {
                M32(arg0, 0) = 0;
            } else {
                M8(arg0, 0x10) = 2;
            }
        }
    } else if (st == 1) {
        if (r4 != 0 && M16(r10, 0x5A) == r7) {
            if ((u32)(u16)(u16)M16(r10, 0x58) == (u32)sp0) {
                s32 cnt = M32(rec, 0x44) + 1;
                M32(rec, 0x44) = cnt;
                if (g_sound_effects_enabled != 0)
                    sound_queue_push(((s32)M16(rec, 0x32) << 8) | DH_SOUND_CUE_BASE);
                {
                    u32 p = (u32)(M32(rec, 0x44) + M32(rec, 0x64));
                    s8 v = (s8)M8(p, 0);
                    if (v >= 0) {
                        M32(rec, 0x40) = (s32)v;
                        return;
                    }
                }
                M32(arg0, 0) = 0;
            }
            return;
        }
        M32(arg0, 0) = 0;
    } else if (st == 2) {
        if (r4 != 0 && M16(r10, 0x5A) == r7) {
            if ((u32)(u16)(u16)M16(r10, 0x58) == (u32)sp0) {
                if (sound_step_command_cursor(rec, r12) == 0)
                    M32(arg0, 0) = 0;
            }
        } else {
            M32(arg0, 0) = 0;
        }
    }
}
