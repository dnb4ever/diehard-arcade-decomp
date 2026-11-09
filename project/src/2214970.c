/* 0x2214970  smpc_pdr2_set: SMPC PDR2 @0x20100077 = (*0x600065b &= 0x10) */
void smpc_pdr2_set(void) {
    *(s8 *)0x0600065B &= 0x10;
    *(s8 *)0x20100077 = *(s8 *)0x0600065B;
}
