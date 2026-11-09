/* 0x221675e  [rank 7-12 unit]  f_21675e: tilemap blit with 0x80-stride wrap. Functionally-correct; not byte-exact (reg alloc/scheduling).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_21675e(s32 arg0, s32 arg1, void *arg2, s32 arg3, s32 arg4, s32 arg5) {
    T *t = (T *)arg2;
    s32 temp_r4 = arg0 + t->unk2;
    s16 *var_r12 = t->unk4;
    s32 var_r9 = (arg5 & 0x200) ? 0x80 : 0x40;
    s32 temp_r7 = arg3 & (var_r9 - 1);
    s32 var_r8 = (arg5 & 0x400) ? 0x80 : 0x40;
    s32 temp_r10 = arg4 & (var_r8 - 1);
    s16 *var_r14 = (s16 *)(arg1 + (temp_r7 * 2) + (temp_r10 << 7));
    s32 sp4 = 0;
    s32 sp8 = var_r9 * 2;
    s16 *spC = (s16 *)((temp_r7 * 2) + arg1);
    s32 var_r11 = temp_r10;
    while (sp4 < t->unk1) {
        s16 *sp0 = var_r14;
        s32 var_r10 = 0;
        s32 var_r5 = temp_r7;
        while (var_r10 < t->unk0) {
            if (var_r5 < var_r9) {
                if (var_r5 < 0x40) {
                    *var_r14 = *var_r12 + temp_r4;
                    var_r12++;
                    var_r14++;
                } else {
                    s16 tv = *var_r12;
                    s16 *temp_r1 = (s16 *)((char *)var_r14 + 0x1F80);
                    var_r12++;
                    var_r14++;
                    *temp_r1 = tv + temp_r4;
                }
            } else {
                s16 *temp_r14;
                var_r5 = 0;
                temp_r14 = (s16 *)((char *)var_r14 - sp8);
                *temp_r14 = *var_r12 + temp_r4;
                var_r12++;
                var_r14 = temp_r14 + 1;
            }
            var_r10++;
            var_r5++;
        }
        var_r11++;
        if (var_r11 < var_r8) {
            s32 var_r3 = (var_r11 != 0x40) ? 0x80 : 0x2080;
            var_r14 = (s16 *)((char *)sp0 + var_r3);
        } else {
            var_r14 = spC;
            var_r11 = 0;
        }
        sp4++;
    }
}
