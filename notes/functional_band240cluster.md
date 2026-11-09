# Functional decomp report - BAND 240 CLUSTER (worker: band240cluster)

Scope: the previously-deferred **0x240622..0x24124e blob-sibling cluster** (the
per-state actor handlers inside the func_02402c0 census blob [0x2402c0, 0x241556))
plus the standalone **func_02240054**.

Rule followed: ship only zero-logical-error work; bound each function individually
to avoid a silent-wrong splice. Every shipped TU compiles
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` with ZERO output
(verified for all 26, both in /tmp staging and in project/src_functional).

## How the cluster was bounded (the crux)

`fn.extent(0x2402c0)` returns the **whole 0x1296-byte merged blob** as one body:
every sibling shares a single literal pool and the compiler **tail-merges** shared
epilogues, so there are no per-sibling census entries. I reconstructed each
sibling by hand from the raw `fn.listing` and then confirmed each with
`tools/decomp/decomp_bounded.py <start> <name> --end <real end>` (which clamps the
window, drops out-of-window targets, and converts out-of-window `bra` tails into
tail calls). Key hazards handled:

- **scratch-first prologue (variant B):** real entry is TWO insns before the
  `mov.l r14,@-r15` push (`mov #100,r0; mov.l <pool>,r3; mov.l r14,@-r15; ...`).
  Affected: func_02240bba, 02240c3e, 02240c6a, 02240c9a, 02240cca, 02240efe,
  0224110e.
- **internal (mid-function) literal pools:** the linear disassembly runs straight
  through them and emits garbage `.byte`/bogus `bra`. The flagged
  "tail-merge" targets 0x240748 / 0x240c32 / 0x240c46 / 0x240bf2 / 0x240c4a /
  0x240de2 / 0x240e02 / 0x240d86 are a MIX of (a) real in-function shared tails
  reached by a real `bra` and (b) PHANTOM "branches" that are actually pool words
  (e.g. the `bra 0x240c4a`/`bra 0x240bf2` at 0x240ad2/0x240ada are the pool
  constants 0xfffea0ba/0xfffea08a of the already-shipped func_02240a4a group, NOT
  code). I treated every branch inside a pool region as phantom and bounded by
  the real prologue->rts/bra-tail+delay-slot only.
- **shared tails:** 0x240748 (func_02240622's own rts tail after its pool),
  0x240c32 (func_02240bba's tail after its pool) and 0x240e02 (func_02240d1c's rts
  tail) are the same-function continuations, folded inline. 0x241164 is a REAL
  function entry (its own prologue) that several siblings tail-call -> modelled as
  `func_02241164(obj, out)`.

## Shipped (26 new .c files, all compile clean)

**Standalone driver**
- `02240054.c` - func_02240054(s32 arg0, void *obj) -> s32. Per-actor state
  tick/transition driver [0x240054,0x2401d6). Dispatches through the two-level
  fn-ptr table `g_060918a0[obj->0xBA & 0x3F][obj->0x108]` (writes an out-value via
  a stack slot, returned at the end); on "arrived" (0x56<0x96) runs the
  spawn/handoff sequence (func_02245c88 / func_0222ca54 / func_0223ed94 /
  func_0220ff54 / func_0223cbb0 / func_0223a924), else (0x56<0xD6) nudges facing
  via func_0223edce. All 11 bsrf resolve cleanly.

**Cluster handlers (all `void func(void *obj, s32 *out)`)**
- Big/branchy: `02240622.c`, `022408a0.c`, `02240d1c.c`, `02241164.c`.
- Dispatch wrapper: `0224081e.c` (E4==9 -> func_02402c0; E4==13 w/ gates ->
  func_022408a0; else func_02241164).
- "Enter statewalk else face" family (func_02241556 gate + facing-table lookup):
  `022409a0.c` (table g_0608d884), `02240e54.c` (g_0608d88c), `02240efe.c`
  (g_0608d890), `02241034.c` (limit 6, g_0608d894, out=3), `02240fd8.c` (no
  facing branch), plus the rng-facing variant `02241164.c` (g_06086720).
- Facing/flag setters: `022407b0.c`, `02240bba.c`, `02240c3e.c`, `02240c6a.c`,
  `02240c9a.c`, `02240cca.c`, `0224110e.c`.
- Leaf flag-setters (`g_060e0f2c[M32(obj,0x94)]=1; -> <handler>`): `0224080e.c`,
  `0224086c.c`, `02240a3a.c`, `02240e0e.c`, `02240eee.c`, `02240fc8.c`,
  `02241024.c`, `022410fe.c`.

## Findings / corrections

- **rng (func_02210020) takes no args.** m2c prints a phantom arg (leftover r4);
  all call sites use `func_02210020()`. Same for the sound call being guarded by
  `g_060e0ddc` everywhere in this band.
- **func_02215f6c == sound_queue_push** (notes/symbols.txt). Declared as
  `func_02215f6c(s32 cmd)` to match the existing shipped TUs; the band callee note
  called it "PIC trigger" - it is the sound-queue push with a 0xA0...FF command.
- **No `tst #imm8 >= 0x80` anywhere in this set.** All byte masks are small
  (`tst #7`, `tst #3`) or reg-form (`tst r3,r2`); the m2c sign-extend trap does
  not apply here.
- **The 409a0/e54/efe handlers are exact clones** differing only in the facing
  table (g_0608d884 / g_0608d88c / g_0608d890). func_02241034 is the same shape
  with func_02241556 limit 6 and an out=3 / 0x1000 fallthrough; func_02241164 is
  the same shape but picks facing with a random bit (rng()&1).
- **func_02240054's 0x0223EE1A argument is a fixed ROM data pointer**
  (mova 0x240100 + (-0x12E6)) handed to func_0220ff54. Symbol unknown, so it is
  kept as a raw `(void *)(uintptr_t)0x0223EE1A` cast (consistent with the raw-
  address style in e.g. 022404ce.c). The g_06097e08 record index uses the SIGNED
  0x34 byte CACHED before func_02245c88 (func_02245c88 could mutate the live
  field), matching the asm.

## Deferred / out of scope

- The larger ambiguous band-240 entries listed as out-of-scope by the previous
  pass (0x241e82, 0x242272, 0x2425d0, 0x243066, 0x246308, 0x246678, 0x2473ee) are
  untouched. 0x2401d6 (the second member of unit 24003e-2402c0, code 0xb4) was not
  requested and is left for a later pass.
- Nothing in the 0x240622..0x24124e range was deferred: all per-state handlers,
  the func_022408a0 helper and the func_02241164 shared tail are shipped.

## Summary
- **26 new TUs shipped**, all compile `gcc -O1 -Wall -Wextra` with zero output:
  02240054 + the full 0x240622..0x24124e cluster (17 handlers + 8 leaf
  flag-setters + the func_022408a0 helper, counted within the 25 cluster TUs).
- Every braf/bsrf in the blob and in func_02240054 resolves to a real function
  (func_022ab18, func_227704, func_02210020, func_0224174e, func_02215f6c,
  rec_delta_angle_neg, and the cross-band func_02240054 callees). No phantoms.
- Names/globals appended to notes/symbols_band240cluster.txt. notes/symbols.txt,
  shared headers (dh.h), tools/, port/ untouched. No git actions taken.
