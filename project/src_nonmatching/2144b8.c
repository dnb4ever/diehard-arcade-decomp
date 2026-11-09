/* 0x22144b8  [rank 7-12 unit]  f_2144b8: SCU-DMA trigger + on-chip reg writes (neg-addr @0xFFFFFFxx). Functionally-correct; not byte-exact (reg alloc).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

s32 func_2144b8(s32 arg0, s32 arg1, s32 arg2) {
    s32 var_r10;
    var_r10 = 0;
loop_1:
    if (!(*(s32 *)-0x50 & 4) && !(*(s32 *)-0x74 & 2)) {
        var_r10 += 1;
        if (var_r10 < 0x10000) {
            goto loop_1;
        }
    }
    *(s32 *)-0x74 = 0;
    *(s32 *)-0x50 = 1;
    *(s32 *)-0x80 = arg0;
    *(s32 *)-0x7C = arg1;
    *(s32 *)-0x78 = arg2;
    *(s32 *)-0x74 = 0;
    *(s32 *)-0x74 = 0x5211;
    return 1;
}
