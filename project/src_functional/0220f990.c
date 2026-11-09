#include "dh_work.h"
/* input_reset_records [0x0220f990]: Clears five 0x18-byte input records and two six-byte direction-tap records. */
#include "dh.h"
/* 0x0220f990 - reset the controller button-state tables. Zeroes all 5 button records of the
 * array g_06098734 (stride 0x18, 12 shorts each, written in DESCENDING offset order 0x16..0x0
 * as the ROM does: the @(22,r4) store is issued first, before dt), then clears the two 6-byte
 * sub-records at g_060987ac and g_060987b2 (bytes +5,+4 then shorts +2,+0 of each).
 * Counterpart to the per-frame poller func_0220f9d4.
 * Real entry 0x20f990 (merged into the fn.units member 0x20f880 by the preceding braf's pool).
 * fidelity: HIGH - transcribed from the fn.py disassembly; leaf (no calls). */
void input_reset_records(void) {
    u8 *rec = (u8 *)DH_PLAYER_INPUT_RECORDS_ADDR;
    u8 *p;
    s32 n;

    for (n = 5; n != 0; n--) {
        PLAYER_INPUT(rec)->latched_direction = 0;
        M16(rec, 20) = 0;
        PLAYER_INPUT(rec)->button2_hold_frames = 0;
        PLAYER_INPUT(rec)->button1_hold_frames = 0;
        PLAYER_INPUT(rec)->direction_change_frames = 0;
        PLAYER_INPUT(rec)->direction_hold_frames = 0;
        M16(rec, 10) = 0;
        M16(rec, 8) = 0;
        PLAYER_INPUT(rec)->released = 0;
        PLAYER_INPUT(rec)->pressed = 0;
        PLAYER_INPUT(rec)->injected_buttons = 0;
        PLAYER_INPUT(rec)->buttons = 0;
        rec += 0x18;                      /* advance one record (delay-slot add) */
    }

    p = (u8 *)DH_PLAYER_DIRECTION_TAPS_ADDR;
    M8(p, 5) = 0;
    M8(p, 4) = 0;
    M16(p, 2) = 0;
    M16(p, 0) = 0;
    p += 6;                              /* 0x060987b2 */
    M8(p, 5) = 0;
    M8(p, 4) = 0;
    M16(p, 2) = 0;
    M16(p, 0) = 0;                       /* rts delay slot */
}
