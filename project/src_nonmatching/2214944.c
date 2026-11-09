/* 0x2214944  smpc_cmd_sshon: SMPC COMREG=0x02 (SSHON). 63.6% - same exact-build trait. */
void smpc_cmd_sshon(void) {
    do {
    } while (((u8) *(s8 *)0x20100063 & 1) == 1);
    *(s8 *)0x20100063 = 1;
    do {
    } while (!((u8) *(s8 *)0x20100063 & 1));
    *(s8 *)0x2010001F = 2;
    do {
    } while (((u8) *(s8 *)0x20100063 & 1) == 1);
}
