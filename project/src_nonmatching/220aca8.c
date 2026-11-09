/* 0x0220aca8  SMPC-style command via fp table *0x06000644: fp(2, *base+*0x06098804+0x108,
   (u16)(fp(0, same, 0)+1)). NONMATCHING: SHC r32 promotes fp+base to callee-saved
   r14/r13 with no frame, where the ROM build keeps fp in r8, spills base to an 8-byte
   frame, and homes arg0 -- register-alloc version trait (opposite callee-save fill order). */
typedef unsigned short u16;
typedef int s32;
typedef int (*smpc_fp)(int, int, u16);
void f_20aca8(s32 arg0) {
    smpc_fp fp = *(smpc_fp *)0x06000644;
    s32 *base = (s32 *)(0x06051230 + (arg0 & 0x3f) * 4);
    fp(2, *base + *(s32 *)0x06098804 + 0x108,
       (u16)(fp(0, *base + *(s32 *)0x06098804 + 0x108, 0) + 1));
}
