/* 0x02245e58  set facing field f4D from (f54 & 0x30)>>4, flip bit1 if f26==0x4000.
   Code is byte-exact EXCEPT the single literal-pool-pointer displacement, which only
   resolves when the shared unit pool is placed in-unit; the unit cannot assemble
   because member 0 (0x2245db2) has a foreign-pool prefix (see 2245db2.c). Isolated
   code match 92.9% (only the mov.w pool displacement differs). */
typedef signed char s8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef struct Obj {
    s8 p0[0x26]; s16 f26;
    s8 p28[0x44-0x28]; s8 f44;
    s8 p45[0x4D-0x45]; s8 f4D;
    s8 p4E[0x54-0x4E]; u16 f54;
} Obj;
void f_245e58(Obj *p) {
    s32 v = (0x30 & (u16)p->f54) >> 4;
    if ((u16)p->f26 == 0x4000) v ^= 2;
    p->f4D = (s8)v;
}
