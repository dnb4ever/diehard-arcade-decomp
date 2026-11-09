/* 0x221603e  NONMATCHING: extern bsr vs jsr (see 0x2215fac). */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
extern void func_215f6c(s32);
typedef struct { s8 unk0; s8 unk1; s8 unk2; s8 unk3; } Rec;
void f_21603e(s16 arg0) {
    s16 sp0;

    sp0 = arg0;
    if (((Rec *)((*(s32 *)0x06098668 * 4) + 0x06053DBC))->unk1 == 0) {
        func_215f6c((sp0 << 8) | 0xA00300FF);
    }
}
