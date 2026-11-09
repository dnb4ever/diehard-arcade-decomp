/* 0x2245eb8  [rank 7-12 unit]  f_245eb8: entity/flag scan over globals @0x06099584. Structure matches; not byte-exact (reg alloc + base-ptr CSE; permuter 7800->7095, not 0).
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

s32 func_245eb8(void *arg0) {
    A *a = (A *)arg0;
    char *base = (char *)0x06099584;
    int r6, r7, i;
    r6 = 4;
    r7 = 4;
    i = 1;
    do {
        if (*(int *)(base + 12 + r6) != 0) {
            P *pe = *(P **)(base + 44 + r7);
            E *e = pe->unk8;
            s32 s = e->unk84;
            s8 e4a, a34;
            if (s & 0x20000000) {
                return 0;
            }
            e4a = e->unk4A;
            a34 = a->unk34;
            if (a->unk8C & 0x39E) {
                if ((s & 0x3800) && e4a == a34) {
                    a->unk114 += 0x78;
                    return 1;
                }
            } else {
                if ((s & 0x3000) && e4a == a34) {
                    return 1;
                }
            }
        }
        i += 1;
        r7 += 4;
        r6 += 4;
    } while (i <= 2);
    return 0;
}
