/* smpc_read_intback [0x0220acf6]: Handshakes an INTBACK request and returns the high nibble of OREG3. */
#include "dh.h"
/* 0x0220acf6 (smpc_intback_read) - run an SMPC INTBACK (peripheral/status fetch).
 * Handshake on SF (0x20100063 bit0): wait until clear, set it, write IREG0=1,
 * IREG1=0xF0, IREG2=0xF0, COMREG=0x10 (INTBACK), wait for SF to clear (command
 * accepted), then return OREG>>4 (0x20100027).
 * fidelity: HIGH - byte register accesses; unsigned result shift. Leaf. */
extern volatile u8 reg_smpc_sf;   /* SF (status flag)  */
extern volatile u8 reg_smpc_ireg0;   /* IREG0             */
extern volatile u8 reg_smpc_ireg1;   /* IREG1             */
extern volatile u8 reg_smpc_ireg2;   /* IREG2             */
extern volatile u8 reg_smpc_comreg;   /* COMREG            */
extern volatile u8 reg_smpc_oreg3;   /* OREG              */

u32 smpc_read_intback(void) {
    while (reg_smpc_sf & 1) {}
    reg_smpc_sf = 1;
    reg_smpc_ireg0 = 1;
    reg_smpc_ireg1 = 0xF0;
    reg_smpc_ireg2 = 0xF0;
    reg_smpc_comreg = 0x10;
    while (reg_smpc_sf & 1) {}
    return (u32)reg_smpc_oreg3 >> 4;
}
