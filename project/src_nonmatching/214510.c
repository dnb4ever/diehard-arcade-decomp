/* 0x2214510  [rank 7-12 unit]  f_214510: SCU-DMA variant with fn-ptr call @0x060902A0. Functionally-correct; not byte-exact (call reloc + reg alloc).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

s32 func_214510(s32 arg0, s32 arg1, s32 arg2) {
    s32 sp0;
    s32 temp_r8;
    s32 var_r9;
    temp_r8 = arg0 & 1;
    sp0 = arg2;
    var_r9 = 0;
    if ((temp_r8 != 0) || (arg1 & 1)) {
        *(s32 *)0x060997A4 = 0xA;
        *(s32 *)0x060997A8 = 0xA;
        ((void (*)(s32, s32, s32, s32))(*(void **)0x060902A0))(0xA, 0x06053D70, arg0, arg1);
    }
loop_4:
    if (!(*(s32 *)-0x50 & 4) && !(*(s32 *)-0x74 & 2)) {
        var_r9 += 1;
        if (var_r9 < 0x10000) {
            goto loop_4;
        }
    }
    *(s32 *)-0x74 = 0;
    *(s32 *)-0x50 = 1;
    *(s32 *)-0x80 = arg0 & ~1;
    *(s32 *)-0x7C = -2 & arg1;
    *(s32 *)-0x78 = sp0;
    *(s32 *)-0x74 = 0;
    *(s32 *)-0x74 = 0x5611;
    if ((temp_r8 != 0) || (arg1 & 1)) {
        return 0;
    }
    return 1;
}
