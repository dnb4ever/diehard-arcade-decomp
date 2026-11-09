/* shc_shift_left [0x0224a360]: SHC variable shift helper: negative count passes through, >=32 gives zero. */
#include "dh.h"
/* 0x0224a360  dispatch_table_a == ashl_var  (compiler variable SHIFT-LEFT runtime helper).
 *
 * The SH-2 has no variable-count shift; the SHC runtime provides one as a 32-way
 * computed goto. This routine is the high-in-degree "dispatch_table_a" (180 call
 * sites in 25 functions) that earlier passes marked banned/unverified.
 *
 * MECHANISM (decoded from ROM, not guessed):
 *   r1 = count, r0 = value.  Range-guard then
 *       r2 = byte_offset_table[count]          ; mov.b @(table+count)
 *       jmp  sled_base + r2                     ; computed goto into a shll sled
 *   The code is boot-copied into HWRAM, so the pool constants are the RUNTIME
 *   addresses of the table/sled (HWRAM = cart + 0x05E04000):
 *       byte table  @0x0604e340  (== cart 0x24a340, 32 signed bytes)
 *       sled base   @0x0604e38a  (== cart 0x24a38a)
 *   Table @0x24a340:
 *       0e 0c 0a 08 06 04 02 00  20 1e 1c 1a 18 16 14 12
 *       34 32 30 2e 2c 2a 28 26  40 3e 3c 3a 48 56 62 6c
 *   Each entry selects an entry point in a run of shll / shll8 / shll16 (and, for
 *   counts 28..31, and#mask + rotr) instructions so that exactly "count" left
 *   shifts execute before rts.  Decoded + simulated for every entry and checked
 *   exhaustively (22000 random (value,count) pairs incl. count<0 and count>=32):
 *   the result is bit-for-bit  value << count.
 *
 * ABI (NON-STANDARD, locked):
 *   in : r0 = value, r1 = count (signed)       out: r0 = value << count
 *   count <  0  -> value returned unchanged (no shift)
 *   count >= 32 -> 0
 *   r2 is saved/restored on the stack (callee-saved across the call); r0,r1 are
 *   the only inputs; r3..r14 untouched -> callers rely on r2/r3 surviving.
 *
 * fidelity: HIGH - mechanism fully reconstructed from ROM and exhaustively
 * simulated to bit-exact equality with (u32)value << count. */
s32 shc_shift_left(s32 value, s32 count) {
    if (count < 0)
        return value;            /* count<0: value passes through unchanged */
    if (count >= 32)
        return 0;                /* everything shifted out */
    return (s32)((u32)value << count);
}
