# 0x24a300-0x24a520 computed-goto dispatch family - cracked

Status: the long-banned "0x24a300-0x24a520 dispatch family" is **not** a game-logic
dispatcher. It is the SHC compiler runtime library`s **variable-count shift helpers**.
SH-2 has no variable shift instruction, so the compiler emits a 32-way computed goto
(`mov.b @(table,count)` + `jmp @(sled_base+offset)`) into a sled of fixed 1/2/8/16-bit
shift instructions. Both dispatchers were decoded from ROM, simulated for every table
entry, and checked exhaustively (22,000 random (value,count) pairs incl. count<0 and
count>=32) to **bit-exact** equality with a plain C shift.

## Key structural fact (why it looked unresolvable)
The whole band is **boot-copied into HWRAM** and runs there. The literal-pool
constants hold the RUNTIME (HWRAM) addresses of the jump tables and sleds, not ROM
addresses - so capstone/pool reads show `0x0604exxx` targets that look like data, not
code. The relocation is constant:

    HWRAM_addr = cart_offset + 0x05E04000      (e.g. cart 0x24a38a -> HWRAM 0x0604e38a)
    (equivalently HWRAM = SH2_addr + 0x03E04000)

The HWRAM copy is byte-identical to ROM, so every table/sled was reconstructed by
reading ROM at (HWRAM_target - 0x05E04000). No swap games (unlike the 0x200xxx boot
cluster); the raw HWRAM pool values are already valid.

## ABI 1 - ashl_var @0x0224a360 (was dispatch_table_a)
    s32 ashl_var(s32 value /*r0*/, s32 count /*r1*/) -> r0 = value << count
      count <  0  : value returned unchanged
      count >= 32 : 0
      r2 saved/restored on stack (callee-saved); r3..r14 untouched.
Byte offset table @cart 0x24a340 (32 signed bytes), sled base @cart 0x24a38a:
    0e 0c 0a 08 06 04 02 00  20 1e 1c 1a 18 16 14 12
    34 32 30 2e 2c 2a 28 26  40 3e 3c 3a 48 56 62 6c
Entry i jumps to sled_base+table[i]; the sled runs exactly i single-bit-equivalent
left shifts (built from shll / shll8 / shll16, and for i=28..31 `and #mask;rotr` which
is `<<i` for the surviving low bits). Verified index-by-index == value<<i.
In-degree: **180 call-sites across 25 functions.**

## ABI 2 - ashr_var @0x0224a420 (was dispatch_table_b)
    s32 ashr_var(s32 value /*r0*/, s32 count /*r1*/) -> r0 = (signed)value >> count  (arithmetic)
      count <  0  : value returned unchanged
      count >= 32 : value >> 31  (0 if value>=0, -1 if value<0)
      r2 saved/restored; r3..r14 untouched.
Dual, sign-dependent dispatch:
  * value<0 (any count) OR value>=0 & count<=8: signed byte table @cart 0x24a418
    (indexed by count-24, reaching back into the descending-offset block at cart
    0x24a400), sled base @cart 0x24a49e - a `shar` sled (+ swap/sign-fill tricks for
    count>=24).
  * value>=0 & count>8: jmp cart 0x24ab2c -> second byte-table dispatch (table @cart
    0x24ab00, sled @cart 0x24ab4a) into a `shlr/shlr8/shlr16` sled. Valid because a
    non-negative value`s logical and arithmetic right shift are identical.
0x24ab2c has **no external callers** - purely an internal tail of ashr_var.
In-degree: **146 call-sites across 20 functions.**

## Callers decompiled (previously deferred)
Both pass args in the non-standard r0/r1/r2 form and call ashl_var via `bsrf` with the
value placed in r0 in the branch delay slot (braf_resolve.py confirms all targets):
  * **func_02221354 vdp2_shadow_setbit(bit)** - set bit `bit` of M16(*g_06098F08,0x20).
    Two bsrf -> 0x24a360 (deltas 0x28ffa, 0x28fe6); clear-then-set form = set.
  * **func_02221384 vdp2_shadow_clrbit(bit)** - clear bit `bit` of M16(*g_06098F08,0x20).
    One bsrf -> 0x24a360 (delta 0x28fcc).

## Functions added (project/src_functional/)
  0224a360.c  func_0224a360 (ashl_var)         HIGH
  0224a420.c  func_0224a420 (ashr_var)         HIGH
  02221354.c  func_02221354 (vdp2_shadow_setbit) HIGH
  02221384.c  func_02221384 (vdp2_shadow_clrbit) HIGH
All compile clean: gcc -c -fsyntax-only -Wall -Wextra (zero warnings).

## Resisted / out of scope (with reason)
  * 0x24a120 (2 callers) - unsigned-divide runtime helper; RESERVED for another agent.
  * 0x24a280 (18 callers) - copy/dispatch helper; RESERVED band neighbour, not a shift.
  * 0x24ab2c - internal logical-shr tail of ashr_var, no external callers; documented
    above, not shipped as a standalone function (nothing calls it directly).
  * 0x24a300 longword table (8 entries -> cart 0x24a296..0x24a2b2) and the small fn at
    cart 0x24a320 - a separate dispatcher in the same band, outside the a/b/shift
    mandate; left for a follow-up (its jump table is in ROM at 0x24a300 and already
    reconstructed here, so the follow-up is cheap).

## How the ABI unblocks others
Every one of the ~326 call-sites above can now be lowered: `ashl_var(v,n) == v<<n`,
`ashr_var(v,n) == (signed)v>>n` (with the documented count<0 / count>=32 edge cases).
The heavy users (e.g. func_02234b58/02235198/0223577c at 45 calls each = sprite/VDP
colour builders; func_02221842/02221bd4 at 33 ashr calls each) are now straightforward.
