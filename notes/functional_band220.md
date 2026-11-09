# Functional decomp report - band 0x220000-0x22c000 (actor/combat core)

Worker: band220. Single serial worker, no forks.

## Census triage
census.py --tsv filtered to this band, triage_cat in (callC,leafC) AND
bytefrac_class==code, minus the done-set (src_functional/*.c stems parsed as hex
vs both cart and sh2 addr) gave 14 candidate rows.

Running find_entry.py on all 14 showed **12 were pool-contaminated census rows
whose resolved real entry was a function already in the done-set** (e.g.
227c86->228106, 221842->22184c, 22154e->22156c, 22bfc2->22c220 [out of band],
22a3e2->22a418, 223abe->223c20, 222c06->221bd4, 22a2e8->22a300, 227a72->227a84,
228efc->228f08, 22a872->22a878, 22812e->228134). So the band is almost exhausted.

Two rows had genuine undone content: 22b0fe (ninsn~1277) and 225ee0 (ninsn~102).

## Added .c (all compile `gcc -O1 -c -Wall -Wextra` with ZERO output)

The census row cart=22b0fe is NOT one function. It is **six contiguous sibling
functions** (each with its own prologue + rts; all six confirmed as real entries
by find_entry.py and all undone). They are the attack-animation-selector family,
direct siblings of the already-done 0x0222ae40 / 0x0222baf8. Decompiled all six:

- `project/src_functional/0222b0fe.c`  func_0222b0fe  (grounded variant; tail-delegates to b7e8)
- `project/src_functional/0222b1e2.c`  func_0222b1e2  (block variant; tail-delegates to b7e8)
- `project/src_functional/0222b2a8.c`  func_0222b2a8  (launch variant; sets unk78/80/C8 physics)
- `project/src_functional/0222b42c.c`  func_0222b42c  (hitstun variant; 2-D else-table + rng)
- `project/src_functional/0222b630.c`  func_0222b630  (knockback variant; flat else-table)
- `project/src_functional/0222b7e8.c`  func_0222b7e8  (full selector; two banks, combo gate, tail-calls 0x020160a0)

Each models unk84 flag classification -> (group, sub, table-bank) -> unk5C via a
3-level anim-table lookup `bank[group][sub][dir*4 + col]`, where
`col = (u16)(unk60+0x4000) >> 14` and `dir = (u16)unk62 >> 13` (xor/offset by
facing unk44), matching the sibling 0222baf8 exactly. Common tail sets
unk84/unk46, raises hit-reaction fx (func_0223a9d0) on unk8C&0xFFFFFF, and some
variants publish g_06099780[unk34-1] and/or fire combo SFX (func_02215f6c).

## Findings / corrections
- The 6-in-1 census merge: braf_resolve over cart=22b0fe reports 32 computed
  branches spanning all six functions; 31 are bsrf calls to a tiny helper set
  (0x227704, 0x223e70, 0x215f6c, 0x23a9d0, 0x210020) and the last is a braf that
  is a **tail-jump** out of func_0222b7e8 to 0x020160a0 (not an internal jump
  table). m2c, started at an early entry, follows the `bra` tail-calls and
  inlines later siblings, so its whole-row output is misleading; per-entry m2c
  is clean for b2a8/b42c/b630/b7e8.
- **m2c correctness trap caught**: in func_0222b42c the else-branch column index
  is `func_02210020() & 1` (unresolved rng bsrf at 22b536). m2c mis-renders the
  clobbered register as the constant `(0x43 & 1)`. The .c uses the rng call.
- **Literal-pool / phantom-branch trap**: fn.py renders many pool longs inside
  func_0222b7e8 as bogus `bsr`/`mov` ops (e.g. "bsr 0x22b60e", "bsr 0x22b4b6").
  Those are the PC-relative deltas for the real bsrf sites; I trusted
  braf_resolve's resolved TARGETs, not the labels.

## Deferred
- **cart 225ee0** (sh2 0x02225ee0, ninsn~102): find_entry reports it as "already
  real", but the disassembly has an epilogue (`add #56,r15; lds.l macl/pr;
  rts; mov.l @r15+,r8`) with **no matching prologue** at the top, and it sits
  immediately behind a `braf r0` jump-table dispatch at 0x225d9c. It is a
  mid-function branch target / dispatch arm of a larger braf-dispatched function,
  not a standalone entry; its stack frame is established elsewhere. Decompiling
  it standalone would be silent-wrong (it reads @(r15) frame slots it never set
  up), so I deferred it. The function right after it (0x02225fac) is already in
  the done-set. This falls in the jump-table territory other workers own.

## Naming
Function names left as func_<sh2>; naming guesses + all new globals recorded in
notes/symbols_band220.txt. Did not touch git, shared headers, tools/, port/, or
notes/symbols.txt.
