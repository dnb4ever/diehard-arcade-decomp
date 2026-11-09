/* 0x2214980  smpc_pdr2_clr: SMPC PDR2 @0x20100077 = (*0x600065b &= 0xEF) */
void smpc_pdr2_clr(void) {
    *(s8 *)0x0600065B &= 0xEF;
    *(s8 *)0x20100077 = *(s8 *)0x0600065B;
}
