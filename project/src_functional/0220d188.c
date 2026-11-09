/* camera_reset_orbit_offsets [0x0220d188]: Resets the orbit accumulators and recomposes position from base plus offset. */
/* func_0220d188 (cart 0x20d188) - leaf.
 * e = arg0->[8]. Reset accel accumulators, derive [0xA0] from [0xA8]<<10,
 * then position [8/C/10] = base[90/94/98] + accum[9C/A0/A4]. Sets spin fields.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void camera_reset_orbit_offsets(void *arg0) {
    void *e = FPTR(arg0, 8);

    FS32(e, 0x9C) = 0;
    FS32(e, 0xA0) = (FS16(e, 0xA8) << 10) + 0x14000;
    FS32(e, 0xA4) = 0;
    FS32(e, 0x08) = FS32(e, 0x90) + FS32(e, 0x9C);
    FS32(e, 0x0C) = FS32(e, 0x94) + FS32(e, 0xA0);
    FS32(e, 0x10) = FS32(e, 0x98) + FS32(e, 0xA4);
    FS16(e, 0x24) = (s16)0xC000;
    FS16(e, 0x26) = FS16(e, 0x26) + 0x100;
    FS16(e, 0x28) = 0;
}
