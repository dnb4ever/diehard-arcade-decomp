/* 0x2214284  weighted 2-column transform: out[j] = (s16)((sum_i row[i].c[j] * (u16)wt[i]) >> 16)
   rows are 8-byte stride (2x s32), wt table @0x06053570 (8-byte entries, 4x s16),
   row index = (s16)(sel>>16), weight index = (sel>>8)&0xFF. Shares the unit pool
   (.short 0x00ff, .long 0x06053570) with 0x2214194.
   NONMATCHING ~14% (code) with SHC v5.0r32 -optimize=1 -speed. Functionally verified.
   Same exact-build codegen blockers as 0x2214194 (extu.b vs and-mask, extu.w CSE,
   register allocation). */
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;

typedef struct { s16 w0; s16 w2; s16 w4; s16 w6; } Wt;
typedef struct { s32 c0; s32 c4; } Row8;
typedef struct { s32 o0; s32 o4; } Out2;

void f_214284(Row8 *r, s32 sel, Out2 *out) {
    Wt *wt = (Wt *)0x06053570 + ((sel >> 8) & 0xFF);
    s32 a0, a4;
    r += (s16)((u32)sel >> 16);
    a0 = r[0].c0 * (u16)wt->w0; a4 = r[0].c4 * (u16)wt->w0;
    a0 += r[1].c0 * (u16)wt->w2; a4 += r[1].c4 * (u16)wt->w2;
    a0 += r[2].c0 * (u16)wt->w4; a4 += r[2].c4 * (u16)wt->w4;
    a0 += r[3].c0 * (u16)wt->w6; a4 += r[3].c4 * (u16)wt->w6;
    out->o0 = (s16)((u32)a0 >> 16); out->o4 = (s16)((u32)a4 >> 16);
}
