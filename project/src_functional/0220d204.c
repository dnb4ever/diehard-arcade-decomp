/* camera_tick_linear_offset [0x0220d204]: Advances the timed horizontal offset and recomposes camera position and angles. */
/* func_0220d204 (cart 0x20d204) - leaf.
 * e = arg0->[8]. Decrement timer [0x70]; while >0 advance accumulator
 * [0x9C] += [0x74]. Then position [8/C/10] = base[90/94/98] + accum[9C/A0/A4];
 * set gravity-ish fields [0x24]=-0xC00, [0x26]=[0x9C]>>4, [0x28]=0.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void camera_tick_linear_offset(void *arg0) {
    void *e = FPTR(arg0, 8);
    s32 t = FS32(e, 0x70) - 1;

    FS32(e, 0x70) = t;
    if (t > 0)
        FS32(e, 0x9C) = FS32(e, 0x9C) + FS32(e, 0x74);

    FS32(e, 0x08) = FS32(e, 0x90) + FS32(e, 0x9C);
    FS32(e, 0x0C) = FS32(e, 0x94) + FS32(e, 0xA0);
    FS32(e, 0x10) = FS32(e, 0x98) + FS32(e, 0xA4);
    FS16(e, 0x24) = (s16)-0xC00;
    FS16(e, 0x26) = (s16)(FS32(e, 0x9C) >> 4);
    FS16(e, 0x28) = 0;
}
