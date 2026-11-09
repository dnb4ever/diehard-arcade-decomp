# Functional decomp report - GIANTS2 worker

Scope: the four largest deferred targets from the band-240 giants + bandresidue notes:
0x242cb8 (jump table), 0x246ab8 (785), 0x2402c0 (2379), and the ~0x225xxx whole function.
Rule followed: ship only zero-logical-error work; defer the rest with decoded CFG.
Shipped TU compiles `gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional`
with ZERO output.

## SHIPPED

### `project/src_functional/02242cb8.c` - func_242cb8(void *ctx, void *obj) [~370 insn]
Per-object motion/mode dispatcher - the local helper func_02242a10 calls 3x. A single
inline braf jump table, `switch(*(s8)(ctx+0x13))` 0..9 (unsigned-compared vs 10; any
other value falls straight to the epilogue). Table @0x242d00, braf @0x242cfa,
target = 0x242cfe + table[idx]; cases 0x242d14/d2a/d42/d6a/dde/e02/e94/eee/f30/f3e,
default/epilogue 0x242f4e. Decoded entirely from clean disasm + pool literals.

Register map at entry: r11=ctx, r13=obj, r10=obj+8, r14=obj+0x14, r12=obj+0x18,
r6=M32(obj,0x14) cached once (used by cases 1/5/6 as the per-frame velocity delta),
r8=2, r4=-3(mask ~2), r5=7, r9=0, r7=-0x400. Each case mutates obj position(+0x08),
velocity(+0x14), accel(+0x18), counter(+0x74), flagword(+0x80 bit1) and advances the
substate ctx+0x13. Cases 8 and 9 share code (8 falls through to 9).

**m2c / note CORRECTION:** band240giants.md called func_242cb8 "self-contained (NO
subroutine calls)". That is WRONG - **case 3 calls func_242f60** (`bsr 0x242f60`, its
only call). func_242f60 = mode_resolve_0x242f60 (already in symbols.txt): no args,
returns an index into g_060995b0[]; case 3 uses it to pick a target object
(e = g_060995b0[i]; p = *(e+8)), sets accel toward it, and on range gates advances
substate to 5. All constants verified from the inline literal pool (0x2f8000, 0x2ed000,
0x310000, 0x2ca000, 0x324000, 0x2c0000, ±0x200/0x400/0x1000/0x2000, 0x4000).

## DEFERRED

### player_combat_statemachine @ **0x02223ee0** - the ~0x225xxx whole function
**TRUE ENTRY = 0x02223ee0** (the task/bandresidue said "<=0x225000"; find_entry is
unreliable here - it returns 0x225ee0, an interior arm). Confirmed rigorously:

- The only function epilogue in the whole span is @0x225fa2:
  `add #56,r15 / lds.l @r15+,macl / lds.l @r15+,pr / rts / mov.l @r15+,r8`.
- The matching prologue (push r8, sts.l pr, sts.l macl, add #-56,r15) exists exactly
  once - at 0x223ee0. A raw-cart scan of [0x223ef0, 0x225fac) finds NO other rts, pr/macl
  save, frame alloc, or register push. So [0x02223ee0, 0x02225fac) is ONE function,
  0x20cc bytes (~4150 insn, ~8.3KB) - BIGGER than the note's 4KB estimate.
- The entry spills the arg entity ptr r4 -> @(52,r15) (the "spilled entity pointer" the
  bandresidue note observed) and immediately indexes g_06098734 (per-player input record,
  stride 0x18) + g_060987ac by (M8(entity,0x34)-1) - i.e. this IS the player-input/combat
  state machine. Preamble decodes button masks 0x300 and 0xC0 into stack slots.

Structure map (for the eventual whole-unit decode):
- Args: r4 = entity ptr. Only callee-saved reg used is r8; everything else lives in the
  56-byte r15 frame (@(4,r15)=entity[0x84], @r15=entity[0x88], @(8,r15)=entity[0x34],
  @(36/40,r15)=the two input-record ptrs, @(46/48/50,r15)=decoded button words,
  @(52,r15)=entity).
- MAIN jump table @0x22403e: `switch(entity[0x88] - 14)` 10 cases, table@0x224044
  (s16, target = 0x224042 + entry), default 0x2243ca; guard `bra 0x2243dc` @0x224024.
- Computed intra-function jump: braf@0x223fb8 -> 0x225c12 (the 0x225c2c region).
- Computed calls: bsrf@0x223fc2 -> func_20fb98; bsrf x4 -> func_23c7f6
  (@0x224b06/225654/225716/225d36). Shared blocks via bsr: @0x22585a->loc_22583e,
  @0x225b52->loc_225a36, @0x225c6c->func_225fac (the already-shipped next function).
- **TRAP correction:** braf_resolve flags "0x225d9c braf r0 via `and r1,r0`" - this is a
  **POOL PHANTOM**, not a table. The real code @0x225d70 ends `bra 0x225ea2`; 0x225d80..
  0x225d9a is a literal pool (0x0609xxxx input-record ptrs + masks 0x0400ffff/0xfbfd0084),
  so the "braf r0" (0x0023) at 0x225d9c is a misdisassembled pool word. The ONLY real
  jump table in this function is @0x22403e.

Deferred because: 8.3KB / ~4150 insn with a 10-way main table plus deep nested
conditionals cannot be reconstructed with a zero-logical-error guarantee in one pass.
This is a dedicated whole-unit task; the true entry + table/trap map above is the handoff.

### func_246ab8 [0x02246ab8, +0x622) 785 insn - decrement/hit-interrupt FSM
Prologue: r14=obj, r13=ctx(r4), r11=M32(obj,0xE0), r12 = mova(0x246ad0)+0xFFFE715C =
**0x00225C2C** (universal tail block, interior of player_combat_statemachine). The body
is NOT a clean 4-state switch: it opens with a deep nested-if cascade (hit-interrupt
M8(obj,0x4C), flags M32(0x6097418), M16(obj,0x54)&2, ...) each arm firing a computed call
and branching to the 0x246c96 tail, with `switch(obj->0xC8)` states 0..3 further in.
31 of 32 computed branches resolve to real fns (see symbols_giants2.txt); the lone
"table" 0x246e68 (`mov.b @(r0,r8)`) is the pool-mask phantom 0x0003FFFF (confirmed, as the
band240giants note said). braf tail-calls: func_249fb0 (x3), func_249836.
Deferred: the near-identical per-state call sequences + subtle early-exit structure are
exactly the silent-wrong risk the mission forbids; cannot carry zero-error in one pass.

### func_2402c0 [0x022402c0, +0x1296) 2379 insn, 58 computed branches
Left deferred (unchanged) - far beyond a zero-error single pass. First two calls:
func_22ab18 (damage), func_227704.

## Summary
- SHIPPED: 0x02242cb8 (compiles clean, zero warnings).
- TRUE ENTRY found: the ~0x225xxx player-input/combat state machine starts at
  **0x02223ee0**, spans to 0x02225fac (~8.3KB), deferred with full table/trap map.
- DEFERRED w/ CFG: 0x02246ab8, 0x022402c0.
- Key corrections: func_242cb8 is NOT call-free (calls func_242f60 in case 3);
  0x225d9c is a pool phantom (real table is @0x22403e); the 225-function entry is
  0x223ee0, not <=0x225000 / not find_entry's 0x225ee0.
