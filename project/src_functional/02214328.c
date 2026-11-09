/* lz_decompress_buffer [0x02214328]: LSB-first control stream emits literals and short/long backreferences; returns output length. */
#include "dh.h"
/* 0x02214328 (lz_decompress) - LZ/RLE bitstream decompressor. Reads a control byte,
 * then consumes one control bit at a time (LSB first, refilled 8 bits at a time from the
 * source). A 1 bit emits a literal byte; a 0 bit introduces a back-reference: a short
 * form (two more control bits give length 2..5 at a fixed small offset) or a long form
 * (a 13-bit offset + optional extra length byte). Back-references copy from the already
 * written output. Returns the number of bytes written (dst advanced - dst start).
 * fidelity: HIGH - faithful m2c transcription of the tight shlr/dt/T-bit stream loop;
 * verified equal to m2c -t sh2-gcc-c output. Control flow preserved via gotos. */
s32 lz_decompress_buffer(s8 *src, s8 *dst) {
    s32 var_condition_bit;
    s32 var_condition_bit_2;
    s32 var_condition_bit_3;
    s32 var_condition_bit_4;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r2;
    s32 var_r7;
    s32 var_r7_2;
    s8 *temp_r3;
    s8 *var_r2_2;
    s8 *var_r4;
    s8 *var_r5;
    s8 temp_r0;
    s8 temp_r0_2;
    s8 temp_r1;
    s8 temp_r1_2;
    s8 temp_r1_3;
    s8 temp_r2;
    s8 temp_r6;
    s8 temp_r6_2;
    s8 temp_r6_3;
    s8 temp_r6_4;
    s8 var_r6;
    u32 temp_r2_2;
    u32 var_r6_2;

    var_r5 = dst;
    var_r6 = *src;
    var_r4 = src + 1;
    var_r7 = 9;
    temp_r3 = var_r5;
loop_2:
    var_r7 -= 1;
    var_condition_bit = var_r6 & 1;
    var_r6 = (s8) ((u32) var_r6 >> 1);
    if (var_r7 == 0) {
        temp_r6 = *var_r4;
        var_r4 += 1;
        var_r7 = 8;
        var_r6 = (s8) ((u32) temp_r6 >> 1);
        var_condition_bit = temp_r6 & 1;
    }
    if (var_condition_bit) {
        temp_r1 = *var_r4;
        var_r4 += 1;
        *var_r5 = temp_r1;
        var_r5 += 1;
        goto loop_2;
    }
    var_r7 -= 1;
    var_condition_bit_2 = var_r6 & 1;
    var_r6 = (s8) ((u32) var_r6 >> 1);
    if (var_r7 == 0) {
        temp_r6_2 = *var_r4;
        var_r4 += 1;
        var_r7 = 8;
        var_r6 = (s8) ((u32) temp_r6_2 >> 1);
        var_condition_bit_2 = temp_r6_2 & 1;
    }
    if (!var_condition_bit_2) {
        var_r7_2 = var_r7 - 1;
        var_r6_2 = (u32) var_r6 >> 1;
        var_condition_bit_3 = var_r6 & 1;
        if (var_r7_2 == 0) {
            temp_r6_3 = *var_r4;
            var_r4 += 1;
            var_r7_2 = 8;
            var_r6_2 = (u32) temp_r6_3 >> 1;
            var_condition_bit_3 = temp_r6_3 & 1;
        }
        var_r7 = var_r7_2 - 1;
        var_r6 = (s8) (var_r6_2 >> 1);
        var_condition_bit_4 = var_r6_2 & 1;
        if (var_r7 == 0) {
            temp_r6_4 = *var_r4;
            var_r4 += 1;
            var_r7 = 8;
            var_r6 = (s8) ((u32) temp_r6_4 >> 1);
            var_condition_bit_4 = temp_r6_4 & 1;
        }
        temp_r2 = *var_r4;
        var_r4 += 1;
        var_r0 = ((u32) ((0U << 1) | var_condition_bit_3) << 1) | var_condition_bit_4;
        var_r2 = temp_r2 | ~0xFF;
block_13:
        var_r0_2 = var_r0 + 2;
        var_r2_2 = var_r5 + var_r2;
        do {
loop_14:
            temp_r1_2 = *var_r2_2;
            var_r2_2 += 1;
            var_r0_2 -= 1;
            *var_r5 = temp_r1_2;
            var_r5 += 1;
        } while (var_r0_2 != 0);
        goto loop_2;
    }
    temp_r0 = var_r4[0];
    temp_r1_3 = var_r4[1];
    var_r4 = var_r4 + 1 + 1;
    temp_r2_2 = (u8) temp_r0 | (temp_r1_3 << 8);
    if (temp_r2_2 != 0) {
        var_r0 = temp_r0 & 7;
        var_r2 = (temp_r2_2 >> 3) | ~0x1FFF;
        if (var_r0 == 0) {
            temp_r0_2 = *var_r4;
            var_r4 += 1;
            var_r2_2 = var_r5 + var_r2;
            var_r0_2 = (u8) temp_r0_2 + 1;
        } else {
            goto block_13;
        }
        goto loop_14;
    }
    return var_r5 - temp_r3;
}
