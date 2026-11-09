/* 0x2217c10  collision/particle bucket-insert (coords from *obj->f8): hash the two
   world coords into the 0x060B659C spatial-grid bucket table, push obj onto the
   bucket list, and cache coord +/- half-extents. NONMATCHING: SHC r32 omits the
   dead idx stack-spill (ROM reserves a 4-byte frame and does mov.l r0,@r15 never
   reloaded) and does not hoist the obj->f8 load into the first branch's delay slot
   (register-allocation / scheduling version trait vs the 1996 build). */
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

void f_217c10(Obj *e) {
    s32 *c;
    s32 *slot;
    if (e->f18 > 0 && e->f20 > 0) {
        c = e->f8 + 2;
        slot = (s32 *)(((0xF & (s16)((u32)(c[2] - *(s32 *)0x06097434) >> 0x10)) << 7)
                       + 0x060B659C
                       + ((0x1F & (s16)((u32)(c[0] - *(s32 *)0x0609742C) >> 0x10)) * 4));
        e->f40 = (s32)slot;
        e->f44 = *slot;
        *slot = (s32)e;
        e->f0 = 1;
        e->fC = c[0];
        e->f10 = c[1];
        e->f14 = c[2];
        e->f28 = c[0] - e->f18;
        e->f2C = c[1] - e->f1C;
        e->f30 = c[2] - e->f20;
        e->f34 = c[0] + e->f18;
        e->f38 = c[1] + e->f24;
        e->f3C = c[2] + e->f20;
    }
}
