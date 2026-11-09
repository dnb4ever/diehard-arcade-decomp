/* 0x2214818  [rank 7-12 unit]  f_214818: display-cmd stack push. CODE IS BYTE-EXACT in isolation; unit non-matching only because it shares a literal pool with f_21482e.
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_214818(int arg0, int arg1, int arg2) {
    *(*(int **)0x06098558) = arg2;
    (*(int **)0x06098558)[1] = arg1;
    (*(int **)0x06098558)[2] = arg0;
    *(int **)0x06098558 += 3;
}
