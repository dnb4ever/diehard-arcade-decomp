/* smpc_start_slave_cpu [0x02214944]: Handshakes SMPC command two to start the slave SH-2. */
#include "dh.h"
/* 0x02214944 (smpc_cmd_sshon) - issue the SMPC SSHON command (slave SH-2 on). Same SF handshake as
 * smpc_cmd_sshoff: wait SF clear, set SF, wait SF set, write COMREG (0x2010001F) = 2 (SSHON), wait
 * SF clear.
 * fidelity: HIGH - byte register accesses; sibling of smpc_cmd_sshoff (COMREG = 3). */
extern volatile u8 reg_smpc_sf;   /* SMPC SF (status flag) */
extern volatile u8 reg_smpc_comreg;   /* SMPC COMREG */

void smpc_start_slave_cpu(void) {
    while ((reg_smpc_sf & 1) == 1) {}
    reg_smpc_sf = 1;
    while (!(reg_smpc_sf & 1)) {}
    reg_smpc_comreg = 2;
    while ((reg_smpc_sf & 1) == 1) {}
}
