/* 0x2216110  [rank 7-12 unit]  f_216110: sound-cue push variant (calls sound_queue_push). Not byte-exact (cross-unit bsr unresolved).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

extern void func_215f6c(int);

void func_216110(void *arg0) {
    if (*(int *)0x060E0DDC != 0) {
        func_215f6c((*(s16 *)(0x060542E8 + ((*(s8 *)((char *)arg0 + 0x34) - 1) * 2)) << 8) | 0xA00000FF);
    }
}
