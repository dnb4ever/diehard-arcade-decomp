/* vertex_blend_four [0x0221400c]: Four unsigned table weights blend four consecutive 0x10-byte vertex records. */
/* 0x0221400c  vtx_blend4: 4-tap weighted blend of four consecutive 0x10-byte
 * vertices into a vec3 (s32) and a 2D s16 pair.
 *   W   = (s16*)(0x06053570 + ((sel>>8)&0xFF)*8)   -> four u16 weights w0..w3
 *   v0  = vtx + (s16)(sel>>16)*0x10 ; v1..v3 follow at +0x10
 *   out3[k] = (w0*v0[4+4k] + w1*v1[.] + w2*v2[.] + w3*v3[.]) >> 7     (mul.l, s32 fields)
 *   out2[0] = (w0*v0[0]  + w1*v1[0]  + w2*v2[0]  + w3*v3[0])  >> 15   (mulu.w, u16 fields)
 *   out2[1] = (w0*v0[2]  + w1*v1[2]  + w2*v2[2]  + w3*v3[2])  >> 15
 * fidelity: faithful - unsigned weights (extu.w / mulu.w), mul.l 32x32 low for
 *   the vec3 terms, mulu.w 16x16 for the s16 terms, arithmetic shar by 7 / 15. Leaf. */
#include "dh.h"

void vertex_blend_four(void *vtx, s32 sel, s32 *out3, void *out2) {
    s16 *W = (s16 *)(uintptr_t)(0x06053570u + (u32)((sel >> 8) & 0xff) * 8);
    u32 w0 = (u16)W[0], w1 = (u16)W[1], w2 = (u16)W[2], w3 = (u16)W[3];
    void *v0 = (u8 *)vtx + (s16)((u32)sel >> 16) * 0x10;
    void *v1 = (u8 *)v0 + 0x10;
    void *v2 = (u8 *)v1 + 0x10;
    void *v3 = (u8 *)v2 + 0x10;

    out3[0] = (s32)(w0*(u32)M32(v0,4)  + w1*(u32)M32(v1,4)  + w2*(u32)M32(v2,4)  + w3*(u32)M32(v3,4))  >> 7;
    out3[1] = (s32)(w0*(u32)M32(v0,8)  + w1*(u32)M32(v1,8)  + w2*(u32)M32(v2,8)  + w3*(u32)M32(v3,8))  >> 7;
    out3[2] = (s32)(w0*(u32)M32(v0,12) + w1*(u32)M32(v1,12) + w2*(u32)M32(v2,12) + w3*(u32)M32(v3,12)) >> 7;

    M16(out2,0) = (s16)((s32)(w0*MU16(v0,0) + w1*MU16(v1,0) + w2*MU16(v2,0) + w3*MU16(v3,0)) >> 15);
    M16(out2,2) = (s16)((s32)(w0*MU16(v0,2) + w1*MU16(v1,2) + w2*MU16(v2,2) + w3*MU16(v3,2)) >> 15);
}
