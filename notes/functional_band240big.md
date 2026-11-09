# Functional decomp report - band 0x240-0x248 BIG residue (worker: band240big)

Scope: the large/hard undone entries flagged in notes/functional_band240.md:
0x243628, 0x246ab8, 0x242a10, 0x247106, 0x2402c0, plus verifying 0x225ee0.
Rule followed: ship only zero-logical-error work; defer the rest with decoded CFG.

## Shipped this pass
- **`project/src_functional/02247106.c`** - `func_02247106(void *other, void *obj)`.
  Per-object state handler: `switch (obj->0xD8)` states 0..3, most paths tail-call
  `func_22dc2c(other, obj)`. Compiles clean `gcc -O1 -c -Wall -Wextra` (zero output).
  **m2c FAILED** on this ("Cannot find branch target func_2469f2" = the tail
  `bra func_2469f2` to a function before this one). Done by full manual disasm +
  braf_resolve. All computed branches resolved; all 8 callee signatures confirmed
  from their prologues (see symbols_band240big.txt). fidelity HIGH on control
  flow / memory effects / calls. Two faithful-ABI modelling choices, documented
  in the file header: (1) return type void (every exit is a tail-call or a plain
  rts that sets no return value); (2) func_245382 modelled with all 5 ABI arg
  slots (r4,r5,r6,r7 + one pushed stack long 0x4000).

## Verified, NOT decompiled (correctly, per instructions)
- **`0x02225ee0` is a dispatch-ARM, not a standalone entry.** Confirmed: it has
  NO prologue - it opens `extu.w r0,r0 / tst #128,r0` and immediately reads frame
  locals set up by a caller (`@(52,r15)`, `@(50,r15)`, `@(8,r15)`, `@r15`) and does
  `bra func_225c8c`. `find_entry.py` reports it "already real" but it is a mid-
  function arm reached by a braf dispatch from a parent that owns the frame. Do NOT
  decompile in isolation. **Likely parent = `func_225c2c`** (= the universal tail
  call of func_246ab8, resolved via the mova+add PIC trap:
  0x246ad0 + 0xFFFE715C mod 2^32 = 0x225C2C). A future pass should decompile
  func_225c2c as the owning dispatcher and treat 0x225ee0 as one of its arms.

## m2c / braf_resolve CORRECTIONS (pool-phantom trap, important)
braf_resolve reports literal-pool longs of the form 0x0003xxxx as spurious
`bsrf r0 TABLE/COMPUTED`. Confirmed phantoms (NOT branches, trust resolved
TARGETs only):
- `243628`: "243814 bsrf r0 add #80,r0" -> `.Lpool_243814 = 0x0003ffff`, a pool
  const used at .L2437d2 (`mov.l .Lpool_243814,r3; tst r3,r2`). 243628 has only
  **4 real bsrf** (0x223e530 x2, 0x22131c8 x2).
- `247106`: "247318/247324 bsrf r0 TABLE" -> `.Lpool_247318=0x00030000`,
  `.Lpool_247324=0x0003ffff`, both mov.l pool consts. Real loop calls are
  func_245382 and func_210020.
- `246ab8`: "246e68 bsrf r0 @(r0,r8)" -> `.Lpool_246e68=0x0003ffff`, pool const
  used at .L246de8. **246ab8 has NO table dispatch** - all computed branches
  resolve to known functions.
- `242a10` / `247106`: func_210020 confirmed to take **no arguments** (overwrites
  r4/r6 on entry) - update any call sites that assumed args.

## Deferred (decoded CFG + reason) - the genuinely hard residue

### 0x242a10 - census "680 insn" is MULTIPLE functions (do NOT treat as one)
- **Real `func_242a10` ends ~0x242ca8** with a clean epilogue + tail `braf` to
  `func_21c368` (braf @0x242ca6, delta 0xfffd96be). ~332 insns. It is a state
  machine `switch(*(s8)(arg0+0x10))` states 0..4 operating on obj = *(s32)(arg0+8).
  Calls: func_228106 (bsrf), func_245b48 (bsrf), func_243590 (bsr, already done),
  and `jsr @r11` with r11 = mova(0x242a30)+0xFFFD353C = **func_215f6c** (PIC trap,
  hand-resolved). This first function is decompilable in a focused pass.
- **A SECOND function begins ~0x242caa** (own prologue `mov.l r14,@-r15 ...
  sts.l pr,@-r15`). It contains an INLINE JUMP TABLE: `mova .Lpool_242d00,r0;
  mov.w @(r0,r1),r0; braf r0` followed by halfword offsets that the disassembler
  mis-decodes as instructions (lines with `.byte func_fc,0x00` etc.). This inline
  table + the `braf r0` dispatch is why **m2c fails** ("Cannot find branch target
  .L242cd6"). Resolving it needs the jump-table halfwords at 0x242d00+ decoded by
  hand and the braf-r0 anchor computed. Deferred: real work but a separate TU.

### 0x243628 (739 insn) - 5-phase per-frame team/pairing resolver
m2c structures most of it well (see the 333-line C). Deferred because its THIRD
loop (m2c C lines ~214-248) is mis-rendered around the two func_22131c8 bsrf calls
(m2c dropped/duplicated the spC/sp1C assignments). Hand-decoded that loop body:
```
b0 = pairIdx & 1;  b1 = (pairIdx >> 1) & 1;        // pairIdx = sp[4], 0..3
res1 = func_22131c8(*(s32)(entry + 8  + b0*8 + b1*4));   // -> sp[12]
res2 = func_22131c8(*(s32)(entry + 24 + b0*4));
coord = res2 + res1;
playerIdx = *(s8)(entry->unk0 + 52);
*(s32)(bucket + 12 + count*8 + 0) = playerIdx;
*(s32)(bucket + 12 + count*8 + 4) = coord;
bucket->unk8 = count + 1;
```
Open risk to resolve before shipping: the exact r4/r5 arg pairing for func_22131c8
(delay slot `mov r5,r4` makes r4=r5=raw; confirm whether it is a 1-arg or 2-arg
callee) and the res1+res2 summation sign. Phases: (1) do-while 3..6 build active
list @0x060970A8, dispatch *(0x06098564) 0..3 -> func_243be0; (2) cleanup
func_243e22; (3) group by *(0x060E0F0C)[0..8] into 0x06096EC4 buckets, func_23e530
sort; (4) the pairing loop above; (5) 4x-unrolled assign func_243d78/func_243db4.

### 0x246ab8 (785 insn) - decrement-counter state machine, switch(obj->0xC8)
All 12 bsrf + 4 braf jumps resolved (no table dispatch - the one flagged was a
pool phantom). Universal tail call = **func_225c2c**. Deferred: 785 insns of dense
nested conditionals that m2c mangled at every bsrf (M2C_ERROR) and in the switch
(`(temp_r4 != 0) && (M2C_ERROR(...), (0xC8 != 0))`). Reconstructing all cases with
a zero-error guarantee in one pass is exactly the silent-wrong risk to avoid.
Teed up: callee list + resolved targets in symbols_band240big.txt.

### 0x2402c0 (2379 insn) - very large per-object update/physics routine
Proper prologue (real code). 58 computed branches, ALL resolved to a small helper
set (func_22ab18 ~30x, func_227704 ~15x, func_215f6c, func_210020, func_244df2,
func_24174e/241556/2415ee/24158c/2417a4/2416c4). Reads obj->unkE0/unk64/unk146/
unk56/unk84/... Deferred per instruction (attempt only if the whole CFG can be
fully resolved in one safe pass; at 2379 insns it cannot). CFG is linear-ish with
many guarded sub-blocks; a future pass should split it into the sub-phases around
the repeated func_22ab18/func_227704 call pairs.
