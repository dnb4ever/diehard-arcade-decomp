/* 0x0220acf6  smpc_intback_read: SMPC INTBACK sequence, return OREG>>4.
   NONMATCHING: correct logic/addressing but SHC r32 instruction scheduling differs
   from the 1996 ROM build (store ordering + first-two-insn order); a leaf with
   enough scheduling freedom to expose the version trait. */
typedef unsigned char u8;
typedef unsigned int u32;
u32 f_20acf6(void) {
    while (*(u8 *)0x20100063 & 1) {}
    *(u8 *)0x20100063 = 1;
    *(u8 *)0x20100001 = 1;
    *(u8 *)0x20100003 = 0xf0;
    *(u8 *)0x20100005 = 0xf0;
    *(u8 *)0x2010001f = 0x10;
    while (*(u8 *)0x20100063 & 1) {}
    return *(u8 *)0x20100027 >> 4;
}
