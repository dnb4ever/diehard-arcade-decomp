/* 0x02245e80  set f26 = 0xC000 (f44=0) if f26<0 else 0x4000 (f44=1).
   NONMATCHING: SHC r32 tail-merges the f44 store (one shared `mov.b`) while the ROM
   build keeps the two stores separate (distinct regs r1/r2) -- register-alloc version
   trait; also cannot bank because the unit will not assemble (member-0 prefix). */
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef struct Obj {
    s8 p0[0x26]; s16 f26;
    s8 p28[0x44-0x28]; s8 f44;
} Obj;
void f_245e80(Obj *p) {
    s32 v;
    if (p->f26 < 0) { v = 0xC000; p->f44 = 0; }
    else { v = 0x4000; p->f44 = 1; }
    p->f26 = (s16)v;
}
