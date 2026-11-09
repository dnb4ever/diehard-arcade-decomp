/* 0x2215fac  pad/port status dispatch -> func_215f6c (SMPC/comms). Record
   array @0x06053DBC indexed by *0x06098668. NONMATCHING: calls an extern
   (func_215f6c, previous TU) so SHC r32 emits mov.l+jsr where the ROM uses
   a near bsr; functionally equivalent. */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
extern void func_215f6c(s32);
typedef struct { s8 unk0; s8 unk1; s8 unk2; s8 unk3; } Rec;
void f_215fac(void) {
    s32 temp_r14;
    Rec *temp_r14_2;
    s32 temp_r4;

    temp_r14 = *(s32 *)0x06098668 * 4;
    temp_r14_2 = (Rec *)(temp_r14 + 0x06053DBC);
    temp_r4 = *(s32 *)0x060E0DDC;
    if ((temp_r14_2->unk0 == 0) && ((u8) temp_r14_2->unk3 != 0xFF)) {
        if (temp_r4 != 0) {
            func_215f6c(0xA01000FF | ((u8) temp_r14_2->unk3 << 8));
        }
        *(s32 *)0x060E0DE0 = (s32) (u8) temp_r14_2->unk3;
        return;
    }
    if (((u8) temp_r14_2->unk2 == 2) && (*(s8 *)0x0609868F < 0x4C)) {
        if (temp_r4 != 0) {
            func_215f6c(0xA01000FF | ((u8) temp_r14_2->unk3 << 8));
        }
        *(s32 *)0x060E0DE0 = (s32) (u8) temp_r14_2->unk3;
    }
}
