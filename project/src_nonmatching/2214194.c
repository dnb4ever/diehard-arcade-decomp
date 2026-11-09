/* 0x2214194  weighted 3-column transform: out[j] = (sum_i row[i].c[j] * (u16)wt[i]) >> 7
   rows are 12-byte stride (3x s32), wt table @0x06053570 (8-byte entries, 4x s16),
   row index = (s16)(sel>>16), weight index = (sel>>8)&0xFF.
   NONMATCHING ~27% (code+pool) with SHC v5.0r32 -optimize=1 -speed. Functionally
   verified. Blockers are exact-build codegen traits r32 cannot reproduce:
   ROM uses `and` with a pooled 0x00ff mask (r32 emits extu.b), 3x redundant extu.w
   per weight (r32 CSEs to 1x), and a different callee-saved register allocation. */
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;

typedef struct { s16 w0; s16 w2; s16 w4; s16 w6; } Wt;
typedef struct { s32 c0; s32 c4; s32 c8; } Row12;
typedef struct { s32 o0; s32 o4; s32 o8; } Out;

void f_214194(Row12 *r, s32 sel, Out *out) {
    Wt *wt = (Wt *)0x06053570 + ((sel >> 8) & 0xFF);
    s32 a0, a4, a8;
    r += (s16)((u32)sel >> 16);
    a0 = r[0].c0 * (u16)wt->w0; a4 = r[0].c4 * (u16)wt->w0; a8 = r[0].c8 * (u16)wt->w0;
    a0 += r[1].c0 * (u16)wt->w2; a4 += r[1].c4 * (u16)wt->w2; a8 += r[1].c8 * (u16)wt->w2;
    a0 += r[2].c0 * (u16)wt->w4; a4 += r[2].c4 * (u16)wt->w4; a8 += r[2].c8 * (u16)wt->w4;
    a0 += r[3].c0 * (u16)wt->w6; a4 += r[3].c4 * (u16)wt->w6; a8 += r[3].c8 * (u16)wt->w6;
    out->o0 = a0 >> 7; out->o4 = a4 >> 7; out->o8 = a8 >> 7;
}
