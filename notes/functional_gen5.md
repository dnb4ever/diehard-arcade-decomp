# Functional decomp report - gen5

Ranges worked (cart offsets; SH2 = cart + 0x02000000):
1. **0x248000 <= addr < 0x24a000** - upper real-code pocket (actor task-handlers).
2. **0x200400 <= addr < 0x201400** - boot-entry sliver.

Single serial worker, no forks. Every `.c` compiles clean with
`gcc -c -fsyntax-only -Wall -Wextra` (**zero warnings**), was hand-traced
statement-by-statement vs `m2c_fn.py --asm`, and had every computed branch
resolved with `braf_resolve.py`. Three of the four were additionally
cross-checked against full `m2c_fn.py` C output (field offsets / sign logic
match; only the resolved calls differ from m2c's `M2C_ERROR` stubs).

## Added this wave (4) - SH2 addr -> file -> name
| SH2 addr | file | name | role |
|---|---|---|---|
| 0x02248104 | 02248104.c | actor_combat_think | 3-state actor FSM, 10 resolved branches, tail animation_update |
| 0x0224852c | 0224852c.c | actor_probe_handler | per-tick FSM; probe -> hand-off func_0224910e; tail animation_update |
| 0x0224910e | 0224910e.c | actor_spawn_sub_init | init sub-actor from variant record; link back to parent |
| 0x022499a6 | 022499a6.c | actor_enter_state14 | enter state 0xE from entity[0x188] param table |

### Key findings
- **The census member 0x24852c is not one function - it is ~21 merged functions**
  (misseg). `fn.extent(0x24852c)` returns 0x24852c..0x249e68 (0x193c bytes) with
  21 `rts` inside; find_entry treats the whole span as one unit. The first
  function (0x24852c..0x248582) is a clean actor handler; several of its neighbours
  are the exact callees of the handlers in this band (func_022489d0 @ 0x2489d0,
  func_022499a6 @ 0x2499a6, func_0224910e @ 0x24910e). I decompiled the entry
  handler plus two of its callees; the remaining ~18 are genuine and tractable but
  not yet done (see remaining).
- **ABI family:** all handlers here share the signature `(r4=task, r5=entity)`,
  push r14, keep entity in r14, save task on the stack, and tail-call
  animation_update (func_0222dc2c, 0x0222dc2c) via a braf. They match the
  already-done 0x02248436 / 0x022482dc / 0x0224840a.
- **Nonstandard-ABI callees.** func_022453de / func_02245eb8 / func_022449b2 /
  func_0210020 all overwrite r5 (or r4) in their own prologue, so they take fewer
  args than a naive call-site read suggests (I declared them accordingly; the
  sibling 0x02248436 over-modelled func_02245eb8 with a dead 2nd arg).
  func_024a360 (in the already-done 0x24a300-0x24a520 dispatch band) reads its
  argument from **r1**, not r4, and its return is discarded at this call site - I
  model it as a one-arg call with a comment, since the r1 ABI can't be expressed
  in portable C.
- **0x249xxx pools are clean** (4-aligned), unlike the 0x200xxx boot pools - the
  bsrf deltas and PC-relative field offsets all read raw/correctly there.

## Resisted / deferred (with reasons)
- **0x02200668 / 0x02200cd0 (sibling pair) and 0x02200d46** - genuine functions
  (real prologues), but they build pointer arguments with the `mov.l pool; mova;
  add` PC-relative idiom out of the **swapped 0x200xxx boot pool**. pool_read.py
  resolves the bsrf *call targets* (0x20eade / 0x20ec1c and 0x20f146 / 0x20f284),
  but reports every pointer-offset pool word as "NEITHER valid" under both raw and
  halfword-swapped interpretation, so the pointer args cannot be reconstructed to
  HIGH fidelity. **0x200d46 additionally has an unresolvable bsrf target** (raw and
  swap both invalid). Defer until the swapped-pool pointer encoding is pinned down.
- **0x02200400** - not a real entry: disassembles mid-function (no prologue, only
  an epilogue popping r8-r14) and branches to func_2003ca *below* the range; its
  true entry is < 0x200400 (outside range 2). Phantom split-point. Defer.
- **0x02201020 (main_init_entry)** - genuine, ~0x570 bytes, ~30 bsrf calls (all
  resolve raw) + mova-relative data pointers (these DO resolve here). Deferred for
  fidelity reasons: it is the reset->init->main boot routine and manipulates SR
  directly (`stc sr,r0; and; or #0xf0; ldc r0,sr` = interrupt-mask programming) and
  dispatches through BIOS function-pointer tables (`jsr @r2` on 0x06000628 /
  0x06000340 / 0x06000300). Those cannot be expressed faithfully in portable C
  without SR/intrinsic modelling; already fully described in notes/symbols.txt.
- **~18 remaining functions inside the 0x24852c..0x249e68 merged extent** - genuine
  actor code, same family, tractable, but each needs its own find_entry (pool gaps),
  branch resolution and callee-arity check. Left for a follow-up wave. Notable ones:
  func_022489d0 (0x2489d0, hit-interrupt handler, ~0x1e4 bytes), func_024923c-ish
  entries, 0x249240 (@0x247836 vtable), func_0249a82.

## Remaining undone genuine code in the two ranges
- Range 1: the ~18 functions inside the 0x24852c merged extent (above).
- Range 2: 0x201020 (deferred, SR/ISR setup), 0x200668/0x200cd0/0x200d46
  (deferred, swapped-pool pointer args), plus the ambiguous `misseg` members
  (0x2006b4, 0x2006de, 0x2007a6, 0x200afc, 0x200d1c, 0x200d78, 0x200d9a) which
  need per-member genuine-vs-data screening before any decomp.
