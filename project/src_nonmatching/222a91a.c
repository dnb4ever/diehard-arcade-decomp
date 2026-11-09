/* 0x222a91a  sets arg0->unk4F (0/1/2) from a global counter @0x06098668, forced
   to 2 when 0x060987C8==1. Clean leaf. NONMATCHING: same misseg/shared-pool unit
   as 0x222a8d8; ~90% insn-identical, functionally equivalent. */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
typedef struct { s8 pad[0x4F]; s8 unk4F; } Obj;
void f_22a91a(Obj *arg0) {
    s32 temp_r5;

    temp_r5 = *(s32 *)0x06098668;
    if (temp_r5 < 0xE) {
        arg0->unk4F = 0;
    } else if (temp_r5 < 0x27) {
        arg0->unk4F = 1;
    } else {
        arg0->unk4F = 2;
    }
    if (*(s32 *)0x060987C8 == 1) {
        arg0->unk4F = 2;
    }
}
