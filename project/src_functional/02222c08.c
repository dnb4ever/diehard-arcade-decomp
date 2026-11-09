/* perspective_build_edge_samples [0x02222c08]: Pipelined SH-2 divider register accesses build perspective samples in original store order. */
/* 0x02222c08: build perspective edge samples with the SH-2 DIVU pipeline.
 * Launch the next divide before storing the preceding sample. Retain divider
 * register writes and the midpoint's intermediate store (06026d12..26d16).
 */
#include "dh.h"
#include "dh_band21.h"

static void launch_edge_divide(s32 divisor, s32 numerator) {
    MEM32(DIVU_DVSR) = divisor;
    MEM32(DIVU_DVDNTH) = numerator >> 16;
    MEM32(DIVU_DVDNTL) = (u32)numerator << 16;
}

void perspective_build_edge_samples(void *entity, s32 *out, s32 count, s32 flag) {
    void *m = (u8 *)entity + 0x34;
    s32 numerator, base, divisor, position, value, quotient;
    s32 *cursor = out + count;

    numerator = -((M16(m, 0) - M16(m, 8)) * M32(m, 0x30)
                + (M16(m, 2) - M16(m, 0xa)) * M32(m, 0x34)
                + (M16(m, 4) - M16(m, 0xc)) * M32(m, 0x38)
                + (s32)((u32)M16(m, 0xc) << 16) + M32(m, 0x2c));
    base = -M16(m, 4) * M32(m, 0x38);
    position = count - 1;
    divisor = M32(m, 0x34) * (position - M16(m, 2)) + base;
    launch_edge_divide(divisor, numerator);

    position -= 2;
    divisor = M32(m, 0x34) * (position - M16(m, 2)) + base;
    quotient = MEM32(DIVU_DVDNTUL);
    value = (u32)quotient >> (flag ? 3 : 4);
    launch_edge_divide(divisor, numerator);
    if ((value & 0xfff80000) || value == 0) value = (s32)0x80000000;
    *--cursor = value;

    do {
        position -= 2;
        divisor = M32(m, 0x34) * (position - M16(m, 2)) + base;
        quotient = MEM32(DIVU_DVDNTUL);
        launch_edge_divide(divisor, numerator);
        value = (u32)quotient >> (flag ? 3 : 4);
        if ((value & 0xfff80000) || value == 0) {
            *--cursor = (s32)0x80000000;
            *--cursor = (s32)0x80000000;
        } else {
            u32 midpoint;
            cursor -= 2;
            cursor[0] = value;
            midpoint = ((u32)value + (u32)cursor[2]) >> 1;
            if (midpoint & 0xfff80000) {
                cursor[1] = (s32)0x80000000;
            } else {
                cursor[1] = (s32)midpoint;
                if (midpoint == 0) cursor[1] = (s32)0x80000000;
            }
        }
    } while (position > 0);
    cursor[-1] = cursor[0];
}
