/* 0x0221478a  scu_dma0_transfer: spin on SCU DMA L0 status (0x25FE007C & 0x10010, cap
   0x10000), then program D0R/D0W/D0C (0x25FE0000..08), mode 0x101, enable (0x25FE0014=7),
   start (0x25FE0010=0x101). NONMATCHING: functionally correct but SHC r32 does not hoist
   the loop-invariant HW base addresses into callee-saved regs the way the ROM build does
   (register-alloc version trait); also shares the unit blocked by member-0's prefix. */
typedef int s32;
void f_21478a(s32 d0r, s32 d0w, s32 d0c) {
    s32 i = 0;
loop:
    if (*(s32 *)0x25FE007C & 0x10010) {
        i += 1;
        if (i < 0x10000) goto loop;
    }
    *(s32 *)0x25FE0010 = 0;
    *(s32 *)0x25FE0000 = d0r;
    *(s32 *)0x25FE0004 = d0w;
    *(s32 *)0x25FE0008 = d0c;
    *(s32 *)0x25FE000C = 0x101;
    *(s32 *)0x25FE0014 = 7;
    *(s32 *)0x25FE0010 = 0x101;
}
