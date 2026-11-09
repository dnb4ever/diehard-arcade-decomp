/* 0x224492c  point/box overlap test against arg0 box offset by arg0->unk130/134,
   half-extents arg2/arg3. Returns 1 if inside, else 0.
   NONMATCHING ~33%: SHC r32 coalesces the four early returns and allocates a
   callee-saved reg where the ROM build reused an argument register; equivalent. */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
typedef struct {
    s32 unk0; s32 unk4; s32 unk8; s32 unkC; s32 unk10; s32 unk14;
    s32 unk18; s32 unk1C; s32 unk20; s8 pad24[0x10]; s8 unk34;
    s8 pad35[0x130-0x35]; s32 unk130; s32 unk134;
} Obj;

s32 f_24492c(Obj *arg0, Obj *arg1, s32 arg2, s32 arg3) {
    s32 temp_r1;
    s32 temp_r5;
    s32 temp_r0;
    s32 temp_r6;

    temp_r1 = arg1->unk8 + arg0->unk130;
    temp_r5 = arg1->unk10 + arg0->unk134;
    temp_r0 = arg0->unk8;
    if (temp_r0 < (temp_r1 - arg2)) {
        return 0;
    }
    if ((arg2 + temp_r1) < temp_r0) {
        return 0;
    }
    temp_r6 = arg0->unk10;
    if (temp_r6 < (temp_r5 - arg3)) {
        return 0;
    }
    if ((arg3 + temp_r5) < temp_r6) {
        return 0;
    }
    return 1;
}
