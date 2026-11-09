/* 0x221482e  [rank 7-12 unit]  f_21482e: SCU-DMA push/trigger. Functionally-correct; not byte-exact (loop-counter lands in r14 vs r-caller: SHC-version reg-alloc).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_21482e(void) {
    int i;
    int *sp;
    sp = *(int **)0x06098558;
    sp -= 3;
    *(int **)0x06098558 = sp;
    i = 0;
    if ((u32)sp >= *(u32 *)0x0609855C) {
        (*(int **)0x06098558)[2] |= 0x80000000;
        while (*(int *)0x25FE007C & 0x10010) {
            i++;
            if (i >= 0x10000) break;
        }
        *(int *)0x25FE0010 = 0;
        *(int *)0x25FE0004 = *(int *)0x0609855C;
        *(int *)0x25FE000C = 0x101;
        *(int *)0x25FE0014 = 0x01000007;
        *(int *)0x25FE0010 = 0x101;
    }
}
