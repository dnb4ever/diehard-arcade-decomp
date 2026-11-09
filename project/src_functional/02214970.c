/* smpc_mask_port_shadow [0x02214970]: Retains only shadow bit four and mirrors the byte to SMPC port two. */
#include "dh_band21.h"
/* 0x02214970 - SMPC SF handshake: mask shadow flag *(u8*)0x0600065B to bit 4 only, then
 * mirror it to SMPC port *(u8*)0x20100077.
 * fidelity: HIGH - leaf; mov.b and #16. */
void smpc_mask_port_shadow(void) {
    MEM8(0x0600065B) &= 0x10;
    MEM8(SMPC_PDR2_ADDR) = MEM8(0x0600065B);
}
