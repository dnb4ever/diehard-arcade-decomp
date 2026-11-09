/* 0x2245f48  [rank 7-12 unit]  f_245f48: shorter sibling of f_245eb8. Functionally-correct; not byte-exact (reg alloc).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

s32 func_245f48(void *arg0) {
    A *a = (A *)arg0;
    char *base = (char *)0x06099584;
    int r5, r6, i;
    r5 = 4;
    r6 = 4;
    i = 1;
    do {
        if (*(int *)(base + 12 + r5) != 0) {
            P *pe = *(P **)(base + 44 + r6);
            E *e = pe->unk8;
            s32 s = e->unk84;
            if (s & 0x20000000) {
                return 0;
            }
            if ((s & 0x3800) && e->unk4A == a->unk34) {
                return 1;
            }
        }
        i += 1;
        r6 += 4;
        r5 += 4;
    } while (i <= 2);
    return 0;
}
