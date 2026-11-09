/* vdp2_reset_transform_block [0x02221ffa]: Ordered 0x70-byte block stores identity basis and preset vectors. */
#include "dh.h"
/* 0x02221ffa  reset a 0x70-byte transform block (struct pointer passed in GBR) to an
 * identity-ish matrix with two preset vectors (0x34/0x38/0x3C = 0xB00070/0xFB0000/0xB00070).
 * fidelity: HIGH - exact; all stores 32-bit; arg passed via GBR (ldc r4,gbr). */
void vdp2_reset_transform_block(void *p) {
    M32(p, 0x0)  = 0;
    M32(p, 0x4)  = 0;
    M32(p, 0x8)  = 0;
    M32(p, 0xC)  = 0;
    M32(p, 0x10) = 0x10000;
    M32(p, 0x14) = 0x10000;
    M32(p, 0x18) = 0;
    M32(p, 0x1C) = 0x10000;
    M32(p, 0x20) = 0;
    M32(p, 0x24) = 0;
    M32(p, 0x28) = 0;
    M32(p, 0x2C) = 0x10000;
    M32(p, 0x30) = 0;
    M32(p, 0x34) = 0xB00070;
    M32(p, 0x38) = 0xFB0000;
    M32(p, 0x3C) = 0xB00070;
    M32(p, 0x40) = 0;
    M32(p, 0x44) = 0;
    M32(p, 0x48) = 0;
    M32(p, 0x4C) = 0x10000;
    M32(p, 0x50) = 0x10000;
    M32(p, 0x54) = 0;
    M32(p, 0x58) = 0;
    M32(p, 0x5C) = 0;
    M32(p, 0x60) = 0;
    M32(p, 0x64) = 0;
    M32(p, 0x68) = 0;
    M32(p, 0x6C) = 0x10000;
}
