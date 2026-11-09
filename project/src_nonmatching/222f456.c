/* countdown timer tick: decrement arg0->unk58; return -1 when it expired or
   arg1->unk84 has any of the given flag bits. Clean leaf. NONMATCHING: member
   of unit 0x222f3ae whose first member is a misseg (fn.units starts 0x12 bytes
   into the previous unit s literal pool); the shared trailing pool prevents a
   byte-exact grind. SHC r32 also picks r6 where the ROM used r3. Equivalent. */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
typedef struct { s8 pad0[0x58]; s16 unk58; s8 pad5A[0x84-0x5A]; s32 unk84; } Obj;
s32 f_22f456(Obj *arg0) {
    s16 temp_r3;
    temp_r3 = arg0->unk58 - 1;
    arg0->unk58 = temp_r3;
    if ((s16) (temp_r3 + 1) <= 0) {
        return -1;
    }
    return 0;
}
