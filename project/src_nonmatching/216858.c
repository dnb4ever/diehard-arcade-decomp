/* 0x2216858  [rank 7-12 unit]  f_216858: tilemap blit with 0x40-stride wrap. Functionally-correct; not byte-exact (reg alloc/scheduling).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_216858(s32 arg0, s32 arg1, void *arg2, s32 arg3, s32 arg4, s32 arg5) {
    T *t = (T *)arg2;
    s32 temp_r4 = arg0 + t->unk2;
    s16 *var_r12 = t->unk4;
    s32 var_r9 = (arg5 & 0x200) ? 0x40 : 0x20;
    s32 temp_r7 = arg3 & (var_r9 - 1);
    s32 sp4 = (arg5 & 0x400) ? 0x40 : 0x20;
    s32 temp_r10 = arg4 & (sp4 - 1);
    s16 *var_r14 = (s16 *)(arg1 + (temp_r7 * 2) + (temp_r10 << 6));
    s32 var_r8 = 0;
    s32 sp8 = var_r9 * 2;
    s16 *spC = (s16 *)((temp_r7 * 2) + arg1);
    s32 var_r11 = temp_r10;
    while (var_r8 < t->unk1) {
        s16 *sp0 = var_r14;
        s32 var_r10 = 0;
        s32 var_r5 = temp_r7;
        while (var_r10 < t->unk0) {
            if (var_r5 < var_r9) {
                if (var_r5 < 0x20) {
                    *var_r14 = *var_r12 + temp_r4;
                    var_r12++;
                    var_r14++;
                } else {
                    s16 tv = *var_r12;
                    s16 *temp_r1 = (s16 *)((char *)var_r14 + 0x7C0);
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
        if (var_r11 < sp4) {
            if (var_r11 != 0x20) {
                var_r14 = (s16 *)((char *)sp0 + 0x40);
            } else {
                var_r14 = (s16 *)((char *)sp0 + 0x840);
            }
        } else {
            var_r11 = 0;
            var_r14 = spC;
        }
        var_r8++;
    }
}
