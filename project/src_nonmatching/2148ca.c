/* 0x22148ca  [rank 7-12 unit]  f_2148ca: dual region zero-fill. Loop bodies are byte-exact; setup differs because SHC constant-folds (base|0x40000000) while the ROM keeps base as a relocatable symbol (no linker here).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_2148ca(void) {
    unsigned base;
    int *p;
    int i;
    base = 0x06099584;
    *(int *)(base | 0x40000000) = 0;
    p = (int *)((base + 76) | 0x40000000);
    i = 16;
    do {
        i -= 2;
        *p = 0;
        p += 4;
        *p = 0;
        p += 4;
    } while (i != 0);
    p = (int *)(0x060E0F6C | 0x40000000);
    i = 16;
    do {
        i -= 2;
        *p = 0;
        p += 4;
        *p = 0;
        p += 4;
    } while (i != 0);
}
