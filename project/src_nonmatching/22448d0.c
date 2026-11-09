/* 0x22448d0  variant of 0x2244874 returning s32 (1/0) and using arg1->unk0.
   NONMATCHING: same saved-register allocation difference as 0x2244874. */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
typedef struct {
    s32 unk0; s32 unk4; s32 unk8; s32 unkC; s32 unk10; s32 unk14;
    s32 unk18; s32 unk1C; s32 unk20; s8 pad24[0x10]; s8 unk34;
    s8 pad35[0x130-0x35]; s32 unk130; s32 unk134;
} Obj;

s32 f_2448d0(Obj *arg0, Obj *arg1, Obj *arg2) {
    s32 temp_r4;
    s32 temp_r7;
    s32 temp_r0;
    Obj *temp_r13;

    temp_r4 = arg1->unk0;
    temp_r7 = arg2->unk0;
    temp_r13 = (Obj *)((s16) (arg0->unk34 * 0x48) + 0x060B6D9C);
    temp_r0 = temp_r13->unk18;
    if (((temp_r4 + 0x30000) > (temp_r7 - temp_r0)) && ((temp_r4 + 0xFFFD0000) < (temp_r7 + temp_r0))) {
        s32 temp_r7_2;
        s32 temp_r0_2;
        s32 temp_r4_2;
        temp_r7_2 = arg1->unk8;
        temp_r0_2 = arg2->unk8;
        temp_r4_2 = temp_r13->unk20;
        if (((temp_r7_2 + 0x10000) > (temp_r0_2 - temp_r4_2)) && ((temp_r7_2 + 0xFFFF0000) < (temp_r0_2 + temp_r4_2))) {
            return 1;
        }
    }
    return 0;
}
