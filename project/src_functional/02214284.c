/* vector2_blend_four [0x02214284]: Four weights combine two columns of 8-byte rows, shifted sixteen and truncated. */
#include "dh.h"
/* 0x02214284 (transform2_weighted) - 4-tap weighted combine of two s32 columns;
 * 2-column sibling of transform3_weighted (0x02214194), sharing the g_06053570 weight
 * table. sel selects row base = rows + (s16)(sel>>16) (8-byte rows) and weight quad
 * wt = g_06053570[(sel>>8)&0xFF]. For each column,
 * out[col] = (s16)((sum over tap 0..3 of r[tap].col * (u16)wt[tap]) >> 16).
 * fidelity: HIGH - mul.l low-32 products, weights u16, result truncated to s16. */
typedef struct { s16 w0, w2, w4, w6; } Wt;    /* 8-byte weight quad */
typedef struct { s32 c0, c4; } Row8;
typedef struct { s32 o0, o4; } Out2;
extern Wt g_06053570[];

void vector2_blend_four(Row8 *rows, s32 sel, Out2 *out) {
    Wt *wt = &g_06053570[(sel >> 8) & 0xFF];
    Row8 *r = rows + (s16)((u32)sel >> 16);
    s32 a0, a4;

    a0  = r[0].c0 * (u16)wt->w0; a4  = r[0].c4 * (u16)wt->w0;
    a0 += r[1].c0 * (u16)wt->w2; a4 += r[1].c4 * (u16)wt->w2;
    a0 += r[2].c0 * (u16)wt->w4; a4 += r[2].c4 * (u16)wt->w4;
    a0 += r[3].c0 * (u16)wt->w6; a4 += r[3].c4 * (u16)wt->w6;
    out->o0 = (s16)((u32)a0 >> 16);
    out->o4 = (s16)((u32)a4 >> 16);
}
