/* Round 4: Keep the original shared reset helper as an address-named function so all three ROM calls are observed at the same boundary; helper body is unchanged. Evidence: work/decomp4/callee_missing.asm. */
/* view_reset_transform_records [0x022161dc]: Initializes the three 0x120-byte transform shadow records and identity coefficients. */
/* 0x022161dc  init_xform_records: initialise the three 0x120-byte transform
 * records at 0x06098ba8 (stride 0x120). Each record is set to an identity-like
 * default: +0x00 = 1.0 (0x10000), +0x10..+0x1C = -1, +0x78/+0x7C = 1.0,
 * +0x88/+0x8C = 1.0, all other longs = 0. Arguments are ignored (the base is
 * hard-coded). Reached via GBR in the original (ldc r4,gbr per record).
 * fidelity: faithful - inner GBR-relative struct fill reproduced as a helper,
 *   invoked for base, base+0x120, base+0x240. */
#include "dh_band21.h"

static void view_reset_one_transform_record(u32 base) {
    u32 off;
    MEM32(base + 0x00) = 0x10000;
    MEM32(base + 0x04) = 0;
    MEM32(base + 0x08) = 0;
    MEM32(base + 0x0c) = 0;
    MEM32(base + 0x10) = -1;
    MEM32(base + 0x14) = -1;
    MEM32(base + 0x18) = -1;
    MEM32(base + 0x1c) = -1;
    for (off = 0x20; off <= 0x74; off += 4) {
        MEM32(base + off) = 0;
    }
    MEM32(base + 0x78) = 0x10000;
    MEM32(base + 0x7c) = 0x10000;
    MEM32(base + 0x80) = 0;
    MEM32(base + 0x84) = 0;
    MEM32(base + 0x88) = 0x10000;
    MEM32(base + 0x8c) = 0x10000;
    for (off = 0x90; off <= 0x11c; off += 4) {
        MEM32(base + off) = 0;
    }
}

void view_reset_transform_records(s32 a, s32 b) {
    u32 base = 0x06098ba8;
    (void)a;
    (void)b;
    view_reset_one_transform_record(base);
    view_reset_one_transform_record(base + 0x120);
    view_reset_one_transform_record(base + 0x240);
}
