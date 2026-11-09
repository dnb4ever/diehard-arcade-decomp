/* 0x02245db2  actor pre-update gate: decrement timer f4E (clamp >=0); if f48 set,
   test weapon id f4A against table 0x06099590, then state flags fE0 (0x2000/0x4000/
   0x08000000) and fBA low-6 bits, clearing f48 / setting fBA|=0x100, returning a code.
   NONMATCHING BLOCKER: fn.py gives this unit a ~26-byte foreign-pool DATA prefix at the
   unit start (0x2245db2..~0x2245dcc) belonging to the previous function; real code
   starts at the `mov #78,r0` (f4E) test. That prefix cannot be emitted from C, so this
   first member (and hence the whole unit's alignment) cannot byte-match. Logic below is
   reconstructed and functionally correct. */
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef struct Obj {
    s8 p0[0x48];
    s8 f48;            /* 0x48 */
    s8 p49;            /* 0x49 */
    s8 f4A;            /* 0x4A */
    s8 p4B[0x4E-0x4B];
    s8 f4E;            /* 0x4E */
    s8 p4F[0xBA-0x4F];
    s16 fBA;           /* 0xBA */
    s8 pBC[0xE0-0xBC];
    s32 fE0;           /* 0xE0 */
} Obj;
s32 f_245db2(Obj *arg0) {
    if (arg0->f4E != 0) {
        s8 v = arg0->f4E - 1;
        arg0->f4E = v;
        if (v < 0) arg0->f4E = 0;
    }
    if (arg0->f48 != 0) {
        s8 k = arg0->f4A;
        if (k >= 1 && k <= 2 && *(s32 *)(0x06099590 + k * 4) == 0) {
            arg0->f48 = 0;
            return 0;
        }
        {
            s32 e = arg0->fE0;
            if (e & 0x2000) {
                if (!(e & 0x4000)) arg0->f48 = 0;
                return 0;
            }
            if ((e & 0x08000000) && (arg0->fBA & 0x3F) < 0xB) {
                arg0->f48 = 0;
                arg0->fBA |= 0x100;
                return 0;
            }
            return 1;
        }
    }
    return 0;
}
