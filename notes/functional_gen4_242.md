# Functional decomp - Gen 4, band 0x242000-0x248000 (cart) = runtime 0x02242000-0x02247fff

Re-attempt wave for the actor-combat AI band. Target: the ~27 members an earlier wave
deferred as "table-dispatch too hard". Per the standing lesson, each deferred member's
computed branches were re-resolved with `braf_resolve` (trusting resolved TARGET
addresses, not capstone's instruction labels) + `pool_read`. `find_entry.py` was run on
every target first. Every file below compiles with `gcc -c -fsyntax-only -Wall -Wextra`
at ZERO warnings and was hand-verified against `m2c_fn.py --asm`. mac.l -> s64 (none hit
here). Single worker, no forks. Stayed strictly in [0x242000, 0x248000); the
0x24a300-0x24a520 dispatch band was not touched.

## Key finding (confirms the lesson)
Of the deferred members, the overwhelming majority had NO real jump table: capstone had
mis-decoded literal-pool words as `bsrf r0`/`braf r0`. Two headline examples now done as
ordinary code:
- **0x022453de** - braf_resolve flagged a "bsrf r0 via mov.b @(r0,r8)". There is no bsrf
  in the function at all; the 0x0003FFFF pool word was misread. It is a plain leaf
  target-scan loop.
- **0x02243e22** - flagged "bsrf r0 via cmp/eq #9" at 0x243e64, which is literally the
  0x00038000 pool word. The one real bsrf resolves to the timing gate 0x02245fc4.

## Functions ADDED this wave (10 files, all HIGH fidelity)

Small combat classifiers:
- `022447d4` func_022447d4 - facing-agreement flag. cls = func_02243fba(a,b) (2-bit
  position mask); base = (cls in {0,3}); XOR with (unk44 != sign(arg2)); return base.
- `02244820` func_02244820 - move-permit flag from the same cls: {0,1}->(unk130>0),
  2->0, 3->1. NOTE a prior-wave call site (02243db4.c) declares this with 4 args
  returning void*; the actual body uses only (arg0,arg1) and returns an s32 flag.

Movement / approach setup:
- `02243be0` func_02243be0 - commit a side-step/approach toward the slot target: reserve
  a lane in g_06096EC0, latch unk120=slot and unk124=lane, set step unk130=+/-0x16666,
  write per-lane distances into the probe block arg2; returns 0/1. (leaf, no calls.)
- `02243e22` func_02243e22 - set up a lateral approach toward the mode-selected peer
  (g_06098564). Chooses unk130 x-offset respecting arena bounds g_060972D0/D4 and lane
  blocking g_06096EC0; toggles unkE0 bits from the func_02245fc4 timing gate; void.

Attack / reaction decision:
- `022458ec` func_022458ec - reaction/attack-type selector (rng&3). Grab fast-path,
  then calls func_02249214 (special) or func_0224806a (proximity dispatch); returns 0..4.
- `02244fdc` func_02244fdc - team-mode attack picker keyed on g_06098564; unk6D==0 tail-
  calls func_02244e54 (non-team sibling of 0x0224519e). Near/far reach gates; returns 0/1/2.
- `02245632` func_02245632 - the core action/side selector feeding func_022454e4. Fills
  *action (0..4) and *side out-params across near-attack / far-grab / grounded branches
  keyed on the mode-peer. Its only computed branch is the rng tick (0x210020).
- `022453de` func_022453de - leaf: scan combat slots 1,2 for an in-range strikeable
  target using the per-state reach-window table g_0608EF6C; returns the target or 0.

Reaction-slot dispatch:
- `02244670` func_02244670 - reaction dispatch over g_0608DBDC[mode]: single write
  (func_022447d4), single-slot or 5-slot scan with the faction/hit gate (g_0604EBE4,
  unk38 & 0x10FFFF); fires the queued move func_02244988 on a match. Has a 5th
  stack-passed s16 argument (the "dir" word) that m2c surfaced as sp->unk26.

Per-frame behaviour:
- `02247a8c` func_02247a8c - actor behaviour handler (task-handler ABI). Steps the
  unkD8 0..7 sub-state (cooldown/attack/grab/throw phases) then tail-calls
  animation_update (0x0222dc2c, the PIC jsr @r12). All 17 computed branches resolve to
  fixed targets - NONE is a real jump table.

## Resisted / deferred (honest, with reason)
- `02244e50`, `0224537a` - 4-/8-byte shared epilogue fragments ("rts; mov.l @r15+,r14" /
  "add #32,r15; lds.l @r15+,pr; ..."), not standalone functions. Skipped (matches wave 2).
- `02243588` (-> real 0x02243590, already done), `02245db2` (-> real 0x02245dcc, already
  done) - pool-contaminated unit starts that find_entry maps onto already-decompiled
  functions. Not separate members.
- **Genuine jump tables (left for a dispatch-focused pass):** `02242a10` (braf r0 via
  `mov.w @(r0,r1)`), `02243628` (bsrf r0 via `add #80,r0`), `02246678`, `02246ab8`,
  `02247106` (two), `02247e14` - each has at least one REAL RAM/ROM dispatch that
  braf_resolve correctly refuses to collapse. These need the 0x24a300-0x24a520 dispatch
  tables (out of range) to render the switch/case faithfully; not stubbed.
- **m2c-failing large handlers (resolvable but need full hand-trace):** `02242272`
  (two delay-slot instructions in a row), `022425d0`, `02243066` (branch into
  pool/table region) - actor handlers tail-calling animation_update / func_022eee4.
  Tractable but each ~0x400 bytes; deferred rather than ship unverifiable.
- `02243fe0` - large (0x4d6) four-way path-walk state machine: four copies of the
  direction-walk loop over trig tables g_0608DB6C/g_0608DBC0/g_0608DBD4/g_0608DBDC with
  nested probe calls (func_022130b4, func_02244670/874/820). Resolvable (all bsrf fixed)
  but too intricate to verify to HIGH in this pass. Deferred, not guessed.
- `022444b6` - trajectory/lead predictor (0x1ba): bsrf mid-expression (func_022130b4),
  a trig direction-walk, and a stack-built probe block fed to func_02217838. Resolvable
  but intricate; deferred.
- `02246308` - cutscene/sequence director (0x2f4): installs PIC handler self-pointers
  (actor->unk0 = &pool+delta) and steps 0x060973xx sequence state; needs ptr_resolve on
  several handler addresses. Deferred to a cutscene-focused pass.

## Band remaining (genuine-code members still UNDONE in 0x242000-0x247fff)
242272, 2425d0, 242a10, 243066, 243628, 243fe0, 2444b6, 246308, 246678, 246ab8,
247106, 2473ee, 247e14. (2473ee: 0x684, all bsrf resolve but very large - a good next
clean target; the rest are the genuine tables, the m2c-failing handlers, and the
path-walk/cutscene machines above.)
