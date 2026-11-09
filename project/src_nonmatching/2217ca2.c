/* 0x2217ca2  same bucket-insert as 0x2217c10 but using obj's own cached coords
   (obj->fC/f10/f14) instead of *obj->f8. NONMATCHING: same register-allocation /
   scheduling version trait (shares the 0x2217d64 literal pool with 0x2217c10, so
   it cannot byte-match unless 0x2217c10 does). */
typedef int s32;
typedef short s16;
typedef unsigned int u32;
typedef struct Obj {
    s32 f0;  s32 f4;  s32 *f8; s32 fC;
    s32 f10; s32 f14; s32 f18; s32 f1C;
    s32 f20; s32 f24; s32 f28; s32 f2C;
    s32 f30; s32 f34; s32 f38; s32 f3C;
    s32 f40; s32 f44;
} Obj;

void f_217ca2(Obj *e) {
    s32 *slot;
    if (e->f18 > 0 && e->f20 > 0) {
        slot = (s32 *)(((0xF & (s16)((u32)(e->f14 - *(s32 *)0x06097434) >> 0x10)) << 7)
                       + 0x060B659C
                       + ((0x1F & (s16)((u32)(e->fC - *(s32 *)0x0609742C) >> 0x10)) * 4));
        e->f40 = (s32)slot;
        e->f44 = *slot;
        *slot = (s32)e;
        e->f0 = 1;
        e->f28 = e->fC - e->f18;
        e->f2C = e->f10 - e->f1C;
        e->f30 = e->f14 - e->f20;
        e->f34 = e->fC + e->f18;
        e->f38 = e->f10 + e->f24;
        e->f3C = e->f14 + e->f20;
    }
}
