/* 0x2214918  smpc_cmd_sshoff: SMPC COMREG=0x03 (SSHOFF). 63.6% - SHC r32 emits
   cmp/eq #1 where ROM reuses the mask register (cmp/eq r4,r2); exact-build trait. */
void smpc_cmd_sshoff(void) {
    do {
    } while (((u8) *(s8 *)0x20100063 & 1) == 1);
    *(s8 *)0x20100063 = 1;
    do {
    } while (!((u8) *(s8 *)0x20100063 & 1));
    *(s8 *)0x2010001F = 3;
    do {
    } while (((u8) *(s8 *)0x20100063 & 1) == 1);
}
