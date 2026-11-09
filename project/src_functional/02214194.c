/* vector3_blend_four [0x02214194]: Four weights at 06053570 combine three columns of 12-byte rows, shifted seven. */
#include "dh.h"
/* 0x02214194 (transform3_weighted) - 4-tap weighted combine of three s32 columns.
 * sel selects a row base = rows + (s16)(sel>>16) (12-byte rows) and a weight quad
 * wt = g_06053570[(sel>>8)&0xFF] (four u16 weights). For each of the 3 columns,
 * out[col] = (sum over tap 0..3 of r[tap].col * (u16)wt[tap]) >> 7.
 * fidelity: HIGH - mul.l low-32 products, weights u16 (extu.w), rows s32x3, >>7 arith. */
typedef struct { s16 w0, w2, w4, w6; } Wt;    /* 8-byte weight quad */
typedef struct { s32 c0, c4, c8; } Row12;
typedef struct { s32 o0, o4, o8; } Out3;
extern Wt g_06053570[];

void vector3_blend_four(Row12 *rows, s32 sel, Out3 *out) {
    Wt *wt = &g_06053570[(sel >> 8) & 0xFF];
    Row12 *r = rows + (s16)((u32)sel >> 16);
    s32 a0, a4, a8;

    a0  = r[0].c0 * (u16)wt->w0; a4  = r[0].c4 * (u16)wt->w0; a8  = r[0].c8 * (u16)wt->w0;
    a0 += r[1].c0 * (u16)wt->w2; a4 += r[1].c4 * (u16)wt->w2; a8 += r[1].c8 * (u16)wt->w2;
    a0 += r[2].c0 * (u16)wt->w4; a4 += r[2].c4 * (u16)wt->w4; a8 += r[2].c8 * (u16)wt->w4;
    a0 += r[3].c0 * (u16)wt->w6; a4 += r[3].c4 * (u16)wt->w6; a8 += r[3].c8 * (u16)wt->w6;
    out->o0 = a0 >> 7;
    out->o4 = a4 >> 7;
    out->o8 = a8 >> 7;
}
