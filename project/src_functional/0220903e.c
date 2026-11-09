/* background_cycle_palette [0x0220903e]: Increments work+32; emits four paired CRAM words from eight-entry 06050d3c table. */
/* func_0220903e (cart 0x20903e) - callC.
 * e = arg0->[8]; bump the s16 at e+0x32 (cycling palette index).
 * Call func_0221632e(14, *(u32*)0x06099510, 0x25F004F0, 4)  (bsrf -> 0x21632E);
 * r6/r7 literals from the pool are live argument registers across the call.
 * Then, four times, read the cursor from 0x06099510, store the high CRAM word
 * (table 0x06050D3C[(idx+2i)&7] << 16), advance the cursor by 4, re-read the
 * cursor and store back (read-modify-write) the same dword ORed with the low
 * CRAM word table 0x06050D3C[(idx+2i+1)&7] -- i.e. two separate memory writes
 * per dword, exactly as the SH-2 code does.
 * Call target: bsrf -> cart 0x21632E -> RAM 0x0601A32E.
 * Confidence: HIGH. Hand-traced vs disassembly.
 */
#include "dh.h"

extern s32 vdp2_queue_attribute(s32 cmd, s32 arg1, s32 arg2, s32 arg3);

void background_cycle_palette(u32 arg0) {
    u32 ent = MU32(arg0, 8);
    s32 i;

    /* e->0x32++ (16-bit, wraps) */
    M16(ent, 50) = (s16)(M16(ent, 50) + 1);

    /* r4=14, r5=*(0x06099510), r6=0x25F004F0, r7=4 */
    vdp2_queue_attribute(14, (s32)MU32(DH_DISPLAY_TRANSFER_CURSOR_ADDR, 0), (s32)0x25F004F0, 4);

    for (i = 0; i < 4; i++) {
        s32 base = M16(ent, 50);           /* re-read @(50,r13) */
        u16 hi   = (u16)MU16(0x06050D3C + (u32)(((base + 2 * i) & 7) * 2), 0);
        u16 lo   = (u16)MU16(0x06050D3C + (u32)(((base + 2 * i + 1) & 7) * 2), 0);
        u32 cur  = MU32(DH_DISPLAY_TRANSFER_CURSOR_ADDR, 0);     /* mov.l @r14,r3 */

        M32(cur, 0) = (u32)hi << 16;        /* mov.l r2,@r3 */
        M32(DH_DISPLAY_TRANSFER_CURSOR_ADDR, 0) = cur + 4;       /* advance cursor */

        {
            u32 cur2 = MU32(DH_DISPLAY_TRANSFER_CURSOR_ADDR, 0) - 4;  /* reload, add #-4 */
            M32(cur2, 0) = (u32)M32(cur2, 0) | (u32)lo;  /* read-OR-write */
        }
    }
}
