#include "dh_work.h"
/* input_update_direction_tap [0x0220fb98]: Four-phase direction release/press detector using six-byte per-player state. */
#include "dh.h"
/* 0x0220fb98 - func_0220fb98(in, st): per-frame direction-input double-tap
 * detector (leaf, void). `in` points at a 16-bit pad word; only bits 0x300
 * (left/right) are looked at. `st` is a 6-byte per-player record (0x060987AC +
 * slot*6): +2 u16 phase, +4 u8 hold counter, +5 u8 "double-tap fired" flag.
 *   st->5 = 0;  dir = *in & 0x300;
 *   phase 0: released        -> phase 1
 *   phase 1: released        -> phase 2, counter = 0
 *   phase 2: pressed         -> phase 3, counter = 0
 *   phase 3: released        -> fired (st->5 = 1), phase 0
 *            pressed         -> counter++; counter >= 2 -> phase 0
 *   other phases: no change.
 * Called by func_02223ee0 (bsrf @0x223fc2) with (pad record, g_060987AC record).
 * fidelity: HIGH - hand-traced vs asm 0x20fb98..0x20fc12; phase compared as u16,
 *   counter as u8, store order kept (case 3 fires: +5 then +2). */
void input_update_direction_tap(void *in, void *st)
{
    s32 dir;

    DIRECTION_TAP(st)->fired = 0;
    dir = MU16(in, 0) & DH_PAD_HORIZONTAL;
    switch (DIRECTION_TAP(st)->phase) {
    case DH_TAP_WAIT_IDLE:
        if (dir == 0)
            M16(st, 2) = (s16)(M16(st, 2) + 1);
        break;
    case DH_TAP_CONFIRM_IDLE:
        if (dir == 0) {
            M16(st, 2) = (s16)(M16(st, 2) + 1);
            DIRECTION_TAP(st)->hold_frames = 0;
        }
        break;
    case DH_TAP_WAIT_PRESS:
        if (dir != 0) {
            M16(st, 2) = (s16)(M16(st, 2) + 1);
            DIRECTION_TAP(st)->hold_frames = 0;
        }
        break;
    case DH_TAP_WAIT_RELEASE:
        if (dir == 0) {
            DIRECTION_TAP(st)->fired = 1;
            M16(st, 2) = DH_TAP_WAIT_IDLE;
        } else {
            DIRECTION_TAP(st)->hold_frames = (s8)(DIRECTION_TAP(st)->hold_frames + 1);
            if (MU8(st, 4) >= 2)
                M16(st, 2) = DH_TAP_WAIT_IDLE;
        }
        break;
    default:
        break;
    }
}
