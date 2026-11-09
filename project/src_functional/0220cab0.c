/* camera_test_zone_bounds [0x0220cab0]: Tests the selected player record against the stage region with fixed margins. */
/* func_0220cab0 (cart 0x20cab0) - leaf, returns s32.
 * AABB-style containment test. e = arg0->[8]; idx = ((e->[0xAE])-1)&1.
 * Gated by a bitmask table (0x0604EBE4[idx] & *0x0609977C). Tests an object
 * box (0x06099764 + idx*12, fields [0]=x,[8]=y) against a camera/region box
 * (0x0606080C + (*0x060B3D54)*44) with +/-0x20000 margins. On success stores
 * the object fields into e->[0x90..0x98] and e->[0xAE]=idx+1, returns 1.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

s32 camera_test_zone_bounds(void *arg0) {
    void *e = FPTR(arg0, 8);
    s32 idx = (FS16(e, 0xAE) - 1) & 1;
    void *obj;
    void *box;
    s32 ox, oy;

    if (!(MEM32(DH_ACTOR_SLOT_MASKS_ADDR + idx * 4) & MEM32(0x0609977C)))
        return 0;

    obj = (void *)(0x06099764 + (s8)(idx * 12));
    box = (void *)(0x0606080C + MEM32(0x060B3D54) * 44);

    oy = FS32(obj, 8);
    if (oy > FS32(box, 0x28) - 0x20000)
        return 0;
    if (oy < FS32(box, 0x1C) + 0x20000)
        return 0;

    ox = FS32(obj, 0);
    if (ox < FS32(box, 0x14) + 0x20000)
        return 0;
    if (ox > FS32(box, 0x20) - 0x20000)
        return 0;

    FS16(e, 0xAE) = idx + 1;
    FS32(e, 0x90) = FS32(obj, 0);
    FS32(e, 0x94) = FS32(obj, 4);
    FS32(e, 0x98) = FS32(obj, 8);
    return 1;
}
