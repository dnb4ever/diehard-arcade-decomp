# Functional decomp report - MONSTERS1 (hard-tail specialist)

Targets: 0x0223f10e, 0x0223e690, 0x0223a9d0 (the three "large flag-gated
command routers / handler" the brief assigned).  Rule followed: build the full
CFG first, ship only zero-logical-error work.  Every shipped TU compiles
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` with ZERO
output.  Names/globals in notes/symbols_monsters1.txt (NOT symbols.txt).

## Headline: all three targets were pool-unit MIRAGES (as functional_lastgiants.md)

fn.py's `extent()` follows branches until the next literal-pool gap; for these
three addresses the gap is the END OF THE WHOLE TRANSLATION UNIT, so the prior
bandbig23 pass measured a run of ~10 sibling functions as one "monster" and
deferred.  Verified by hand (fresh `mov.l r14,@-r15` prologues after each
rts/tail): the real functions are small/medium and ALL fully resolvable.

| brief said | fn.py "code" | REAL extent | real insn | real computed branches |
|------------|--------------|-------------|-----------|-------------------------|
| 0x23f10e ~2808 insn / 46 br | 0xaf8 | [0x23f10e,0x23f160) | ~40 | 0 (all calls direct/1 computed-ptr) |
| 0x23e690 ~1930 insn / 36 br | 0x78a | [0x23e690,0x23e878) | ~244 | 13 |
| 0x23a9d0 ~2234 insn / 11 br | 0x8ba | [0x23a9d0,0x23aada) | ~90 | 1 |

The "46 / 36 / 11 computed branches" from braf_resolve are UNIT-wide; only
13 / 1 belong to the real e690 / a9d0, and f10e's lone computed site is the
g_0608D6C4 handler-table call.  The phantom "bsrf r0 index table" at 0x23e72c /
0x23f3dc is confirmed the mask literal 0x0003FFFF (`tst r3,r2` on ent->0x8C),
not a jump table - matches the prior notes.

## SHIPPED (4) - all whole, zero logical error, zero warnings

### `0223e690.c` - func_0223e690(void *ent)   [cmd_router_a]
The genuine router at the target address.  entA=ent->0x158; tbl=entA->4 (nullable
halfword base); flags=ent->0xE0.  flags&0x100 -> {func_02247d6e gate, tail
func_022469f2}.  (ent->0x8C&0x3FFFF)==0 && flags&0x40 -> func_02245d1c, nonzero
tail func_02249836(ent,r).  switch(func_02244e2e){1:tail 02247036; 3:tail
022470a2; 2:call 022470a2 then common; 0/def:common}.  Common: unless ent->0x54&2,
gate on tbl!=0, ent->0x40==0, tbl[ent->0x100<<1]==ent->0x58.  Then
switch(func_02243fe0){1:func_0224840a + ent->0x5C=ent->0x14A; 2:tail func_02249fb0;
0/def:0x23e804 block}.  0x23e804: sel=g_060995D0[ent->0x34<<5 + ent->0x128<<2],
gate 0x40000<=sel<0x190000, (re-read)flags&0x10, !(flags&0x200),
10<=(u16)(g_0609740C + (ent->0x44?8:0))<=14 -> ent->0x120=ent->0x128,
tail func_022473ee(ent, saved).

### `0223f29a.c` - func_0223f29a(void *ent)   [cmd_router_b]  (BONUS)
This is the actual large flag-gated router the brief's "0x23f10e" description was
about; the address was mis-attributed by the mirage.  switch(ent->0xD8):
- state 0: the SAME predicate chain as func_0223e690 (independent cross-check of
  that decode - identical guards + both inner switches; 243fe0 default -> call
  sibling func_0223f494, return).
- state 1: inline construction - g_06098580 |= ent->0x100; if g_06098670==1 and
  g_06098830>=1 then {g_06098830=1; p=g_0609882C; p->13=1; q=p->8;
  q->0xAE=q->0xB0=q->0xB2=0;}.  (ent->0x54&2 -> tail func_02246cac.)
- else: return.

### `0223f10e.c` - func_0223f10e(void *task)   [state handler]
task->8 = ent.  if func_02245dcc(ent): ent->0xD4=17, ent->0xD8=func_0223ffcc
(task,ent).  Dispatch g_0608D6C4[ent->0xD4](task,ent); func_0222ca98(ent); tail
func_0222eee4(ent).  (Small per-state handler; siblings func_0223f198/f1ea/f23c
are near-identical.)

### `0223a9d0.c` - func_0223a9d0(void *ent, void *a1)   [actor retarget/handoff]
ent->0x8C &= 0xFF000000; g_06096C74[ent->0x34]=(u16)g_0607C3BC[ent->0x5E];
if ent->0x34<3 func_022260c8(ent).  Two near-identical halves:
- if ent->0x6C!=0: dispatch func_0223a924(ent->0x6E, a1, rec+0x30, ent) - for
  ent->0x34>=3 only when (s8)ent->0x6C in {13,14}, else unconditionally - then
  clear 0x6C/0x6E and copy g_060E11E8[ent->0x34]->g_06096250[ent->0x34<<5].
- if ent->0x6D!=0: if (s8)ent->0x6D==8 set g_060E0F0C[ent->0x6F]=7; dispatch
  func_0223a924(ent->0x6F, a1, rec+0x50, ent); clear 0x6D/0x6F; copy
  g_060E11EA[ent->0x34]->g_06096256[ent->0x34<<5].
Tail func_0223b254(ent).  rec = 0x06097D58 + (ent->0x34<<8).

## m2c corrections / traps resolved
- **sp[0] is not a func_02243fe0 argument.** e690 writes **(ent->0x174) to the
  reserved stack slot at 0x23e7aa then calls func_02243fe0; this had cast doubt
  on the shipped 1-arg 02243fe0.  Resolved: the slot is a hoisted local 'saved'
  reloaded at 0x23e860 as the 2nd arg of the tail call func_022473ee(ent,saved).
  Independently confirmed by func_0223f29a, which calls func_02243fe0 with NO
  stack setup -> 243fe0 is 1-arg.  The shipped 02243fe0.c stands.
- **flags re-read:** e690's 0x23e804 block re-reads ent->0xE0 (0x23e82c) rather
  than reusing the cached value, because intervening callees may mutate it;
  modeled with a fresh `flags2` read.
- **tst #2 on ent->0x54** is a true 16-bit bit-1 mask (extu.w first), not the
  m2c sign-extend trap (that only bites tst #imm8 >= 0x80).  No >=0x80 tst
  occurs in any of the four functions.
- **dead fn-ptr spill:** f10e's `mov.l r2,@r15` at 0x23f144 spills the handler
  pointer to an unused slot (no stack arg consumed, tail-call function) - dropped.
- m2c produced no usable scaffold for any of these TUs (e690 unit: "stc tbr,r1"
  parse failure on a pool datum; f10e unit: all-M2C_ERROR) - 100% hand-traced
  against fn.py + braf_resolve.py.

## Deferred (none of the targets) - siblings left for future passes
All four shipped whole; nothing tractable deferred.  NOT shipped (declared
extern): the sibling functions exposed by the mirage (listed at the foot of
notes/symbols_monsters1.txt).  func_0223f494 (f29a's reach helper; uses the
documented reg-shift helper func_0224a360) and func_0223f54e (a task->0x10
dispatcher) are the strongest next candidates.

## Summary
- SHIPPED WHOLE: func_0223e690, func_0223f29a, func_0223f10e, func_0223a9d0.
- All three named targets were pool-unit mirages (fn.py over-measure); the real
  functions are 40-244 insn and fully resolvable.
- Key cross-check: func_0223e690 and func_0223f29a share an identical predicate
  chain; decoding both independently validated the chain.
- Settled the long-standing func_02243fe0 stack-arg ambiguity (1-arg confirmed).
