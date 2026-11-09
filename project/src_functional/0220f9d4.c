#include "dh_work.h"
/* input_poll_records [0x0220f9d4]: Computes held/pressed/released buttons, hold timers and BIOS start requests. */
#include "dh.h"
/* 0x0220f9d4 - per-frame controller state machine. Picks the active player set from
 * g_06098670 (==0 -> set 1 and process all 5 button records; else set 0 and process only
 * record 4): record array base g_06098734 + which*0x60 (stride 0x18), raw-input array
 * 0x06000708 + which*0x20 (stride 8).
 *
 * For each record: cur = rec->unk2 | raw; rec->unk0 = cur; rec->unk4 = ~prev & cur (press
 * edges); rec->unk6 = ~cur & prev (release edges); increment the hold timers rec->unkC and
 * rec->unkE. The 0x3C0 direction bits gate rec->unk16/unkC/unkE (reset timers on a direction
 * change or when a new direction is latched); bit0 gates rec->unk10 and bit1 gates rec->unk12
 * (each counts held frames, reset on the press edge).
 *
 * Then, if the system pad g_06098798 has bits 0x30 set and g_060987D0 == 0, it calls the
 * kernel vector *(0x0600066C) on the pad buffer 0x060987D4 and, per 0x10/0x20 bit and the
 * per-player ready bytes g_06098674/5 (==0 or ==3), latches the start flags g_0609880C /
 * g_06098810 via the kernel probe *(0x06000664)(player).
 *
 * Takes no arguments (m2c's phantom arg is dead). fidelity: HIGH - transcribed from asm;
 * the two indirect calls go through RAM kernel-vector slots (1 ptr arg / returns s32). */
#define FN(a) (*(void **)(uintptr_t)(a))

void input_poll_records(void) {
    s32 which = (M32((void *)DH_PLAY_SESSION_MODE_ADDR, 0) == 0) ? 1 : 0;
    u8 *rec = (u8 *)DH_PLAYER_INPUT_RECORDS_ADDR + which * 0x60;
    s16 *raw = (s16 *)(uintptr_t)(0x06000708 + which * 0x20);
    s16 pad;
    s32 i;

    for (i = which * 4; i < 5; i++, rec += 0x18, raw += 4) {
        s16 prev = PLAYER_INPUT(rec)->buttons;
        s16 cur  = (s16)(PLAYER_INPUT(rec)->injected_buttons | *raw);
        PLAYER_INPUT(rec)->buttons = cur;
        PLAYER_INPUT(rec)->pressed = (s16)(~prev & cur);
        PLAYER_INPUT(rec)->released = (s16)(~cur & prev);
        PLAYER_INPUT(rec)->direction_hold_frames += 1;
        PLAYER_INPUT(rec)->direction_change_frames += 1;
        if ((u16)cur & DH_PAD_DIRECTIONS) {
            if ((cur ^ prev) & DH_PAD_DIRECTIONS)
                PLAYER_INPUT(rec)->direction_change_frames = 0;
            if (!((cur & PLAYER_INPUT(rec)->latched_direction) & DH_PAD_DIRECTIONS)) {
                PLAYER_INPUT(rec)->latched_direction = cur;
                PLAYER_INPUT(rec)->direction_hold_frames = 0;
            }
        } else {
            PLAYER_INPUT(rec)->direction_change_frames = 0;
            PLAYER_INPUT(rec)->direction_hold_frames = 0;
        }
        if ((u16)cur & DH_PAD_ACTION1) {
            PLAYER_INPUT(rec)->button1_hold_frames += 1;
            if ((cur ^ prev) & DH_PAD_ACTION1)
                PLAYER_INPUT(rec)->button1_hold_frames = 0;
        } else {
            PLAYER_INPUT(rec)->button1_hold_frames = 0;
        }
        if ((u16)cur & DH_PAD_ACTION2) {
            PLAYER_INPUT(rec)->button2_hold_frames += 1;
            if ((cur ^ prev) & DH_PAD_ACTION2)
                PLAYER_INPUT(rec)->button2_hold_frames = 0;
        } else {
            PLAYER_INPUT(rec)->button2_hold_frames = 0;
        }
    }

    pad = M16((void *)0x06098794, 4);
    if ((pad & DH_SYSTEM_START_MASK) && M32((void *)0x060987D0, 0) == 0) {
        ((void (*)(void *))FN(0x0600066C))((void *)0x060987D4);
        if ((pad & DH_SYSTEM_START_P1) && M16((void *)0x0609880C, 0) == 0
            && (M8((void *)DH_PLAYER_JOIN_STATES_ADDR, 0) == 0 || MU8((void *)DH_PLAYER_JOIN_STATES_ADDR, 0) == 3)
            && ((s32 (*)(s32))FN(0x06000664))(0) != 0) {
            M16((void *)0x0609880C, 0) = 1;
        }
        if ((pad & DH_SYSTEM_START_P2) && M16((void *)0x06098810, 0) == 0
            && (M8((void *)0x06098675, 0) == 0 || MU8((void *)0x06098675, 0) == 3)
            && ((s32 (*)(s32))FN(0x06000664))(1) != 0) {
            M16((void *)0x06098810, 0) = 1;
        }
    }
}
