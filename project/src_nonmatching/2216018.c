/* 0x2216018  NONMATCHING: extern bsr vs jsr (see 0x2215fac). */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
extern void func_215f6c(s32);
typedef struct { s8 unk0; s8 unk1; s8 unk2; s8 unk3; } Rec;
void f_216018(s32 arg0) {
    s32 sp0;

    sp0 = arg0;
    if (*(s32 *)0x060E0DDC != 0) {
        func_215f6c((sp0 << 8) | 0xA01000FF);
    }
    *(s32 *)0x060E0DE0 = sp0;
}
