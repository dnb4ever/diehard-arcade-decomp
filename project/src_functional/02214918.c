/* smpc_stop_slave_cpu [0x02214918]: Handshakes SMPC command three to stop the slave SH-2. */
#include "dh.h"
/* 0x02214918 (smpc_cmd_sshoff) - issue the SMPC SSHOFF command (slave SH-2 off). Handshake on the
 * SMPC status flag reg SF (0x20100063 bit0): wait until SF clears, set SF, wait until it is observed
 * set, write COMREG (0x2010001F) = 3 (SSHOFF), then wait for SF to clear (command accepted).
 * fidelity: HIGH - byte register accesses; sibling of smpc_cmd_sshon (COMREG = 2). */
extern volatile u8 reg_smpc_sf;   /* SMPC SF (status flag) */
extern volatile u8 reg_smpc_comreg;   /* SMPC COMREG */

void smpc_stop_slave_cpu(void) {
    while ((reg_smpc_sf & 1) == 1) {}
    reg_smpc_sf = 1;
    while (!(reg_smpc_sf & 1)) {}
    reg_smpc_comreg = 3;
    while ((reg_smpc_sf & 1) == 1) {}
}
