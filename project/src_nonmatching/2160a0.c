/* 0x22160a0  [rank 7-12 unit]  f_2160a0: sound-cue push (calls sound_queue_push @0x2215f6c). Not byte-exact: cross-unit bsr cannot resolve under the objcopy-only oracle, and SHC tail-calls where the ROM does bsr+return.
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

extern void func_215f6c(int);

void func_2160a0(void *arg0) {
    int sp0, sp4;
    int t;
    t = *(int *)0x060E0DDC;
    if ((*(s16 *)((char *)arg0 + 0xBA) & 0x3F) < 0xB) {
        sp4 = *(u8 *)((char *)arg0 + 0x49);
        sp0 = (*(s16 *)((char *)arg0 + 0x64) > 0x10) ? 1 : 0;
        if (t != 0) {
            func_215f6c((*(s16 *)(0x060542B8 + (((sp4 * 2) + sp0) * 2)) << 8) | 0xA00000FF);
        }
    } else if (t != 0) {
        func_215f6c((*(s16 *)(0x060542E4 + ((*(s8 *)((char *)arg0 + 0x34) - 1) * 2)) << 8) | 0xA00000FF);
    }
}
