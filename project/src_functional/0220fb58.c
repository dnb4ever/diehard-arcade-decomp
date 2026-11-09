/* input_settle_injected_buttons [0x0220fb58]: Repeats the resident injected-button halfword copy into the first player input record five times. */
#include "dh.h"
/* 0x0220fb58 - copy the 16-bit value at 0x06000708 into g_06098734->unk2 (0x06098736)
 * five times in a row (unrolled/settle loop over the same source and destination).
 * fidelity: HIGH - transcribed from m2c; leaf, no calls/branches beyond the count loop. */
void input_settle_injected_buttons(void) {
    s32 i = 5;
    do {
        i--;
        M16((void *)DH_PLAYER_INPUT_RECORDS_ADDR, 2) = M16((void *)0x06000708, 0);
    } while (i != 0);
}
