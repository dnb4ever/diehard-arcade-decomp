# Functional decomp report - band 0x240 GIANTS (worker: band240giants)

Scope: the deferred large 0x240-band functions from notes/functional_band240big.md:
0x243628, 0x242a10 (+ its helper), 0x246ab8, with 0x2402c0 left deferred.
Rule followed: ship only zero-logical-error work; defer the rest with decoded CFG.
All shipped files compile `gcc -O1 -c -Wall -Wextra` with ZERO output.

## Shipped

### `project/src_functional/02243628.c` - func_02243628(void) [739 insn]
Per-frame team / pairing resolver, 5 phases (build active list from slots 3..6 ->
cleanup -> group-by into buckets + func_23e530 sort -> pairing buckets -> 4-wide
assign). m2c structured phases 1,2,4(control),5 but emitted M2C_ERROR at all four
computed calls. Resolved from asm + braf_resolve + callee prologues.

m2c / braf_resolve CORRECTIONS:
- **Pool-phantom confirmed:** braf_resolve "0x243814 bsrf r0" is NOT a branch;
  `.Lpool_243814 = 0x0003FFFF`, a mask literal (mov.l) used at .L2437d2 as
  `obj->0x8C & 0x3FFFF`. The function has exactly 4 real bsrf (2x func_23e530,
  2x func_22131c8).
- **func_22131c8 fully resolved** (the note's open risk): its prologue is
  `dmuls.l r4,r5 / sts mach,r1 / sts macl,r0 / rts / xtrct r1,r0`, i.e. a 2-arg
  16.16 fixed-point multiply returning the middle 32 bits of the 64-bit product
  = `((s64)a*(s64)b) >> 16`. At BOTH phase-4 call sites the delay slot `mov r5,r4`
  leaves r4 == r5 == the loaded coord, so each call **squares** one coord.
- **Sum sign resolved** (the note's other flag): the two results are **added**
  (asm `add r1,r4`), so metric = sq(coordA) + sq(coordB) - a sum of squares, not
  a difference. The phase-4 loop body is exactly the note's hand-decode, now
  confirmed: `v1 = entry[8 + b0*8 + b1*4]; v2 = entry[24 + b0*4];
  bucket.metric = f(v1,v1) + f(v2,v2)` with `f = func_22131c8`.
- **func_23e530 takes NO register args** - both call sites (0x24388c, 0x2439f0)
  set no argument registers; it works entirely through the globals (reads the
  0x06096EC4 bucket array / 0x06097050 count, writes the 0x06097070 result array).

### `project/src_functional/02242a10.c` - func_02242a10(void *ctx) [~332 insn]
Per-object state machine, `switch(*(s8)(ctx+0x10))` states 0..4; obj = *(ctx+8).
m2c fails on the whole census range ("Cannot find branch target .L242cd6")
because the FOLLOWING function (func_242cb8) has an inline jump table. Decoded the
real func_242a10 ([0x242a10, 0x242caa)) by hand from clean disasm + pool_read.

Layout clarification (corrects band240big note): the "2nd function at ~0x242caa"
is NOT at 0x242caa - that address is a 14-byte literal pool (the three longs
0x242cac/0x242cb0/0x242cb4). The second function is **func_242cb8** at 0x242cb8,
the local helper that func_242a10 calls 3x via PC-relative `bsr`.

Resolved computed calls / traps:
- jsr @r11 with r11 = mova(0x242a30)+0xFFFD353C = **func_215f6c** (PIC, hand-
  computed). It TAKES one arg (r4) - the command codes 0xA013xxFF - unlike
  func_210020/func_228106 which take none.
- 0x242a82 bsrf -> func_228106 (prologue `mov #-1,r4` overwrites r4 => no args).
- 0x242ab0 bsrf -> func_245b48 (arg = obj+8, via delay-slot `add #8,r4`).
- 0x242ca6 braf -> func_21c368(obj): the shared tail call every path reaches.
Modelled void (ends in the tail call), matching the band's other state handlers.
Fall-throughs (state 0 -> 1, state 3 -> 4) rendered with stacked case labels + a
`state ==` prelude guard to stay warning-clean.

## Deferred (with decoded CFG / table + reason)

### func_242cb8 [0x242cb8, ~0x242f4e) ~370 insn - local jump-table dispatcher
Self-contained (NO subroutine calls); a single inline braf jump table. Entry:
`switch(*(s8)(ctx+0x13))` 0..9 (idx>=10 -> default @0x242f4e). Table @0x242d00,
braf @0x242cfa, target = 0x242cfe + table[idx]:
```
case0 0x242d14  case1 0x242d2a  case2 0x242d42  case3 0x242d6a  case4 0x242dde
case5 0x242e02  case6 0x242e94  case7 0x242eee  case8 0x242f30  case9 0x242f3e
default 0x242f4e
```
Each case writes obj physics/flag fields (obj via r10=obj+8, r13=obj, r12=obj+24,
r14=obj+20: +0x14, +0x18, +0x80 ...). Deferred: 370 insn of dense per-case field
writes; ~37 insn/case is real hand-decode work and a separate TU. func_242a10
ships referencing it as extern, so the shipped TU is complete and compiles.

### func_02246ab8 [0x246ab8, ...] 785 insn - decrement-counter state machine
`switch(*(s32)(obj+0xC8))` states 0..3, wrapped by a `(counter!=0 && state!=0)`
guard; universal tail call **func_225c2c** (= mova(0x246ad0)+0xFFFE715C =
0x00225C2C). **NO table dispatch** - braf_resolve flags 32 computed branches but
31 resolve to real functions and the lone TABLE (0x246e68, `mov.b @(r0,r8)`) is
the pool phantom 0x0003FFFF. Resolved callee set (see symbols file): func_2489d0,
func_246208, func_249a82, func_249f08, func_245bcc, func_245d1c, func_24840a,
func_246180, func_247d6e, func_210020(no args), func_244e2e, func_24492c,
func_243fe0; braf jumps func_249fb0 (x3), func_249836.
Deferred per instruction (ship only if zero-error): m2c collapses to 120 lines
with 12 M2C_ERROR and loses the switch variable (renders `switch(0xC8)` and
`(temp_r4 != 0) && (M2C_ERROR(...), (0xC8 != 0))`). 785 insn of near-identical
per-state call sequences cannot be reconstructed with a zero-error guarantee in
one pass - exactly the silent-wrong risk the mission says to avoid.

### func_022402c0 (2379 insn) - left deferred per instruction (unchanged).

## Summary
- SHIPPED: 0x02243628, 0x02242a10  (both compile clean, zero warnings).
- DEFERRED w/ decoded CFG: 0x02242cb8 (jump table), 0x02246ab8 (full callee CFG),
  0x022402c0 (unchanged).
- Key m2c corrections: func_22131c8 = 16.16 fixed mul (square+sum of squares in
  243628); func_23e530 / func_228106 take no args; func_215f6c takes one (PIC);
  242a10's "2nd function" is func_242cb8 @0x242cb8 (pool, not code, at 0x242caa);
  246ab8 has no table dispatch (0x246e68 is a pool phantom).
