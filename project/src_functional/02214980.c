/* smpc_clear_port_shadow_bit [0x02214980]: Clears shadow bit four and mirrors the byte to SMPC port two. */
#include "dh_band21.h"
/* 0x02214980 - SMPC SF handshake: clear bit 4 of shadow flag *(u8*)0x0600065B, then mirror
 * it to SMPC port *(u8*)0x20100077 (counterpart of func_02214970).
 * fidelity: HIGH - leaf; mov.b and #239 (~0x10). */
void smpc_clear_port_shadow_bit(void) {
    MEM8(0x0600065B) &= 0xEF;
    MEM8(SMPC_PDR2_ADDR) = MEM8(0x0600065B);
}
