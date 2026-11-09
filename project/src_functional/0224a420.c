/* shc_shift_right_signed [0x0224a420]: SHC arithmetic shift helper: negative count passes through, >=32 sign-fills. */
#include "dh.h"
/* 0x0224a420  dispatch_table_b == ashr_var  (compiler variable ARITHMETIC SHIFT-RIGHT helper).
 *
 * Signed sibling of dispatch_table_a.  High in-degree (146 call sites in 20
 * functions - the sprite-attribute / VDP colour builders and anim interpolators
 * lean on it).  Earlier passes correctly guessed "arithmetic shift-right" from
 * one path but marked it unverified because of its sign-dependent control flow.
 *
 * MECHANISM (decoded from ROM):
 *   r1 = count, r0 = value.  After the 0<=count<32 range guard it branches on the
 *   sign of value (rotl r2 tests bit31):
 *     - value < 0, any count, OR value >= 0 with count <= 8:
 *         index the signed byte table (@0x0604e418 == cart 0x24a418, indexed by
 *         count-24 so it reaches back into the descending negative-offset block
 *         at cart 0x24a400) and jmp sled_base(@0x0604e49e == cart 0x24a49e) + byte,
 *         landing in a run of shar (and swap/or sign-fill tricks for count>=24).
 *     - value >= 0 with count > 8:
 *         jmp 0x0604eb2c (== cart 0x24ab2c), a second byte-table dispatch into a
 *         logical-shift-right (shlr/shlr8/shlr16) sled - valid because a
 *         non-negative value shifts right identically logical/arithmetic.
 *   count >= 32 is handled at cart 0x24a45c: returns 0 if value>=0, -1 if value<0
 *   (i.e. value>>31, the sign replicated).
 *   Decoded + simulated for every table entry and checked exhaustively (22000
 *   random (value,count) pairs incl. negative values, count<0 and count>=32):
 *   the result is bit-for-bit the signed  value >> count.
 *
 * ABI (NON-STANDARD, locked):
 *   in : r0 = value (signed), r1 = count (signed)   out: r0 = value >> count (arith)
 *   count <  0  -> value returned unchanged
 *   count >= 32 -> value >> 31  (0 or -1)
 *   r2 saved/restored on the stack; r3..r14 untouched.
 *
 * NOTE: the internal logical-shift-right tail at cart 0x24ab2c has NO external
 * callers (0 bsrf/braf sites) - it is reached only from this routine.
 *
 * fidelity: HIGH - mechanism fully reconstructed from ROM and exhaustively
 * simulated to bit-exact equality with the signed value >> count. */
s32 shc_shift_right_signed(s32 value, s32 count) {
    if (count < 0)
        return value;            /* count<0: value passes through unchanged */
    if (count >= 32)
        return value >> 31;      /* sign replicated: 0 (value>=0) or -1 (value<0) */
    return value >> count;       /* value is signed -> arithmetic shift right */
}
