# Functional decomp report - band 0x237-0x240 "jump-table" trio

Worker: band-jt (hard-tail). Targets handed over as m2c-unlowerable
"table-driven computed jumps". Investigation shows NONE of the three actually
use a `mov.b @(r0,rN)` indexed jump table, and there is no real `stc tbr`
instruction anywhere (the byte that disassembles as `stc tbr,r1` at 0x23fdf8
is the literal-pool halfword 0x014A = the constant 0x14A loaded at 0x23fdde).
What defeats m2c in all three is: (a) PC-relative computed branches
(`bsrf`/`braf`) whose targets m2c cannot resolve, and (b) literal pools
embedded mid-function that corrupt m2c's instruction stream.

All computed branches were resolved with braf_resolve.py (and two `mova`+`add`
computed `jsr` targets in 0x2399bc resolved by hand). Entries confirmed with
find_entry.py.

## Shipped (2)

| file | summary | fidelity |
|------|---------|----------|
| 02239462.c | sprite/anim sub-object per-frame handler; cel-record lookup in g_06097d58, advances frame, tail-calls draw func_02153f6. | HIGH |
| 0223fcb6.c | actor hit/collision-resolution decision handler; walks sub-decisions and tail-calls specialised reaction handlers; descriptor-table lookup at g_06099584. | HIGH |

Both compile clean: `gcc -c -fsyntax-only -Wall -Wextra` (ZERO warnings).

### 0x02239462 decoded branches
- `braf r1` @0x2394cc -> 0x02153f6 (delta 0xfffdbf26) [tail call; draw_child_sprite]
  (The second `braf` to 0x02153f6 at 0x23956a belongs to the adjacent function
   func_02239500, not to this target.)
- Dispatch is a plain `if (actor->unk10 == 0)` first-frame init, NOT a table.

### 0x0223fcb6 decoded branches
bsrf (standard-ABI calls):
- @0x23fcd0 -> 0x0247d6e (0x809a)   actor_block_setup, returns s32
- @0x23fd3a -> 0x0245d1c (0x5fde)   returns target ptr or 0
- @0x23fd52 -> 0x0244e2e (0x50d8)   actor_state_dispatch, returns s32 (switch 0..3)
- @0x23fd6c -> 0x02470a2 (0x7332)   (case 2)
- @0x23fdc4 -> 0x0243fe0 (0x4218)   returns s32 (switch 0..2)
- @0x23fdda -> 0x024840a (0x862c)   actor_set_state_7 (case 1)
- @0x23fe92 -> 0x0245fc4 (0x612e)   (actor, entity)->s32
braf (tail calls):
- @0x23fce4 -> 0x02469f2 (0x6d0a)
- @0x23fd4c -> 0x0249836 (0x9ae6)   (actor, target)
- @0x23fd7c -> 0x0247036 (0x72b6)   (case 1)
- @0x23fd88 -> 0x02470a2 (0x7316)   (case 3)
- @0x23fdf0 -> 0x0249fb0 (0xa1bc)   (case 2)
Two cmp/eq dispatch chains (func_0244e2e result, func_0243fe0 result) are
if/else ladders, NOT indexed jump tables.

## Deferred (1)

### 0x022399bc - DEFERRED (nonstandard-ABI mid-function call)
Fully decoded; control flow and every computed branch are known (see
notes/functional_jumptable_analysis.md). It is a 3D skeletal/aim transform
routine: dispatch `switch (actor->unk10)` {0,1,2,default} (plain cmp/eq
ladder, not a table), two command-byte-string loops driving matrix rotations,
and a shared draw tail.

Reason for deferral: the shared tail issues `jsr @r9` where r9 is computed
(mova 0x239bb4 + 0xfffdae30) = 0x022179e4, which is NOT a function entry - it
is the interior (+0x24) of func_022179c0 (real prologue at 0x2179c0, code flows
continuously into 0x2179e4). This mid-function alternate entry takes its inputs
in callee-saved registers r13/r14 (= the record `e` and `obj`), and on the
shared-tail path r5 (which func_022179c0 dereferences at @(4,r5)/@r5/...) is not
defined by any standard call. This is a nonstandard register ABI identical in
character to the func_024a4xx r0/r1 family that band23hi already ruled
"cannot be expressed faithfully in functional C". The second command-string
loop (0x239c5c-0x239ca2) also pipelines two cursors (r10 and the @r15 local)
offset by one byte, with the first byte skipping one of the two per-byte calls
- a further silent-equivalence hazard. Deferred rather than guessed.

(All its OTHER computed branches resolve to ordinary standard-ABI matrix/vector
helpers and are listed in the analysis file; only the func_022179c0 alt-entry
call blocks a faithful lowering.)

## m2c corrections / notes
- No `stc tbr` instruction exists in 0x23fcb6; it is misdisassembled pool data.
- No `mov.b @(r0,rN)` indexed jump table exists in any of the three; the
  "table-driven" description was inaccurate. The real m2c blockers were
  bsrf/braf targets + mid-function literal pools.
- braf_resolve.py emits spurious "TABLE/COMPUTED via ..." rows for bytes that
  fall inside embedded data pools (0x239a1c in 0x2399bc, 0x23fdfc in 0x23fcb6);
  these are data, not real computed branches.
- find_entry.py reports "already real" for addresses that are mid-function
  (e.g. 0x2179e4); it rejects pool phantoms but does not detect interior
  addresses. Confirm enclosing-function boundaries by disassembling upward.
