/* 0x22148ac  [rank 7-12 unit]  f_2148ac (zero_fill_aligned): CODE IS BYTE-EXACT in isolation; unit non-matching only via shared pool with f_2148ca.
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_2148ac(int arg0, int arg1) {
    int *p;
    int *end;
    p = (int *)((arg0 & 0x1FFFFFF0) | 0x40000000);
    end = (int *)((int)p + arg1);
    while (p < end) {
        *p = 0;
        p = (int *)((int)p + 0x10);
    }
}
