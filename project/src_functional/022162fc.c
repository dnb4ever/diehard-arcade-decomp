/* 0x022162fc  reset_displist_state: zero a set of display-list globals and set the write pointer.
 *   *0x0609950c = 0;  *0x06098f0c = 0;  *0x06098f10 = 0;  *0x06099510 = 0x060f0000
 * fidelity: faithful - r5=0 stores, final store in rts delay slot. Leaf. */
#include "dh_band21.h"

void reset_displist_state(void) {
    MEM32(0x0609950c) = 0;
    MEM32(0x06098f0c) = 0;
    MEM32(0x06098f10) = 0;
    MEM32(DH_DISPLAY_TRANSFER_CURSOR_ADDR) = 0x060f0000;
}
