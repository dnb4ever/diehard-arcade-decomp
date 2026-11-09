/* crc16_buffer [0x0220074c]: Byte loop with 0x10210000 polynomial, 0xDEBDEB00 seed and eight flush shifts. */
#include "dh.h"
/* 0x0220074c  func_0220074c: CRC-16 (poly 0x1021 carried in the high half) over
 * [buf, buf+len). Seed 0xDEBDEB00 (0x200778); each byte (mov.b @r4+ / extu.b at
 * 0x200750-0x200754) is OR'd into the low bits, then 8 MSB-first shifts:
 * shll r0 sets T from the bit shifted out (OLD bit 31) and bf skips
 * xor r3,r0 (0x10210000) only when T is clear, so the XOR condition is the
 * PRE-shift MSB, not the post-shift one. After the byte loop (dt r5), 8
 * trailing flush shifts run (0x200766-0x200770), and shlr16 r0 in the rts
 * delay slot (0x200774) returns the high 16 bits in r0.
 * fidelity: HIGH - exact arithmetic; leaf; guest buffer read via MU8. */
u32 crc16_buffer(u32 buf, s32 len) {
    u32 crc = 0xDEBDEB00u;
    u32 n = (u32)len; /* dt r5 wraps; keep the 32-bit decrement well-defined */
    u32 i;
    u32 msb;
    do {
        crc |= MU8((void *)(uintptr_t)buf, 0); /* mov.b @r4+,r1 ; extu.b r1,r1 */
        buf += 1;
        for (i = 8; i != 0; i--) { /* mov #8,r2 ... shll/bf/xor ... dt r2 */
            msb = crc >> 31; /* T from the OLD bit 31 (shll carry-out) */
            crc <<= 1;
            if (msb)
                crc ^= 0x10210000u; /* xor r3,r0 */
        }
    } while (--n != 0); /* dt r5 ; bf 0x200750 */
    for (i = 8; i != 0; i--) { /* mov #8,r2 ... dt r2 flush loop */
        msb = crc >> 31;
        crc <<= 1;
        if (msb)
            crc ^= 0x10210000u;
    }
    return crc >> 16; /* shlr16 r0 in the rts delay slot */
}
