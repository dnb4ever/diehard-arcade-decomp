# Functional mop-up pass (census-driven)

Scope: densest undone genuine functions across the whole code range EXCEPT the
0x240000-0x248000 band. Method: census.py --tsv filtered to triage in (callC,leafC)
AND bytefrac_class==code, minus the done-set; each census member corrected with
find_entry.py (pool-contaminated starts -> real entries), deduped, re-checked against
the done-set. 107 raw candidates collapsed to 44 genuinely-undone real functions once
pool-contaminated starts were corrected (many census "callC" members resolve to
already-done functions, e.g. cart 20734a -> real 207360, already done).

## Decompiled (7 functions, each compiles -O1 -Wall -Wextra with zero output)

| file | SH2 | band | what |
|------|-----|------|------|
| 0220d8d0.c | 0x0220d8d0 | 200 | per-player entity motion / post-hit field updater |
| 0222ece4.c | 0x0222ece4 | 220 | broad-phase collision sweep over the 32x16 spatial hash |
| 0220e6d4.c | 0x0220e6d4 | 200 | motion integrator + lookat-angle builder |
| 0220d58c.c | 0x0220d58c | 200 | swing / pendulum motion handler (sine-driven) |
| 022306c4.c | 0x022306c4 | 230 | action/attack state setup task (A) |
| 02230ae0.c | 0x02230ae0 | 230 | action/attack state setup task (B) - sibling of A |
| 0223db2a.c | 0x0223db2a | 230 | projectile / tumbling-debris state machine |

All seven hand-verified against m2c_fn --asm + braf_resolve. Callee ABIs taken from
their already-decompiled sources where available. Fidelity HIGH except the
func_022173f4 draw-helper arg list in the two action-setup tasks (MED - that callee
is undecompiled; same matrix-draw cluster the author already flagged MED under
effect_tumble_task).

## Findings / corrections

- m2c DROPS the func_0224a420/ashr_var shift. A delayed `bsrf r3` to ashr_var with the
  value loaded in the delay slot renders in m2c as a plain assignment of the delay-slot
  load (e.g. `unk14 = arg1->unk0`) when the stored value is actually
  ashr_var(arg1->unk0, g_06096c90). Confirmed in func_0220e6d4 (6 calls) and
  func_0223db2a (6 calls). Anyone trusting raw m2c on these loses the >> count.
- New helper surfaced: func_022132d4 (transform/AABB builder) - the single real bsrf in
  func_0222ece4; delta 0xfffe4564 is a genuine negative branch delta (NOT a phantom).
- func_02210020 returns a value (used as rng: &1, &3, &0x1fff by callers); the tumble
  state machine stores its low word into unk24.

## DATA mis-flagged as code (do NOT decompile)

The 16 census candidates in 0x280000-0x290000 (SH2 0x02289600-0x0228c21a, cart
289600-28c21a) are DATA: they disassemble as the repeating `0604 mov.b r0,@(r0,r6)`
filler interleaved with trapa/gbr-relative ops - classic misdisassembly. Reported, not
decompiled. (Band is literally labelled "upper (data)" in census.)

## Deferred (with reason)

- 0x02200400 / 0x02200668 / 0x02200cd0 / 0x02200d46 (tiny boot stubs): braf_resolve
  yields out-of-range targets (deltas 0xe4680000, 0xe5940000, 0x305542...). The pool
  longs are not valid branch deltas (one is 0x49c82fe6 whose low half 0x2fe6 is a
  prologue opcode) - these look data-adjacent / mis-segmented and need deeper
  reconstruction. Honest deferral over a guessed decomp.
- 0x02225ee0: not a standalone entry - reads incoming r0 and @(N,r15) stack slots and
  branches backward to 0x02225c8c (outside its extent). Fragment of the larger function
  at ~0x02225c8c.
- 0x0223cfdc: 8-byte tail fragment (add #4,r15; pop pr/r14; rts) - a shared epilogue,
  no body.
- 0x0224a280: reads unset r0 (non-standard ABI / continuation), dispatches through a
  jump table at g_0604e2dc; m2c output unusable.
- 0x0224a884: m2c output is garbage (saved_reg/unset-register reads, nonsense
  bit-munging) - needs full asm reconstruction.
- 0x0221dc04, 0x022305e2: braf jump-table state machines; m2c mangled. Need the jump
  table hand-resolved - left for a focused pass.

## Not attempted (budget; genuine, left for follow-up)

Large/call-heavy single functions, all genuine code: 0x022082be (1740), 0x0220b0c6
(654), 0x0220c782 (422, jump table), 0x0221f2bc (630), 0x0222c39c (1702), 0x0222ca98
(1560), 0x02239ce8 (656, m2c stack-var merge artifacts), 0x0223a9d0 (2234), 0x0223d768
(536), 0x0223e690 (1930), 0x0223f10e (2808, single function). These are the densest
remaining real work.
