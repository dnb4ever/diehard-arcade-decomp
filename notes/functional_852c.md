# Functional decomp report - the 0x24852c merged extent (sub-function wave)

Target unit: the census member **0x24852c**, which is actually a merged extent
`0x24852c..0x249e68` (0x193c bytes) holding ~19 genuine functions (fn.units groups
them into one because they share a literal-pool band; the 21 `rts` the prior wave
counted include early-returns inside functions, so the real function count is ~19).

Scope for this wave: everything in `[0x24852c, 0x249e68)` EXCEPT the three already
done by gen5 (0x24852c actor_probe_handler, 0x24910e actor_spawn_sub_init,
0x2499a6 actor_enter_state14).

Method per function: `decomp_bounded.py <addr> --end <next-real-entry> --asm` to
isolate the clamped body, then hand-trace statement-by-statement against the asm;
every `braf`/`bsrf` resolved with `braf_resolve.py`, every PIC data/handler pointer
with `ptr_resolve.py` (trusting resolved TARGET addresses, not labels). Each `.c`
compiles with `gcc -c -fsyntax-only -Wall -Wextra` at **ZERO warnings**.

## Sub-function map of the extent (real entries found)
find_entry/linear-rts scan + prologue screening gave these genuine entries (the
`add #4,r15` / `lds.l @r15+,pr` "entries" the scanner also emits are dead epilogues
after braf tails - same function - and 0x249358 is a pool/data misdisassembly, not
an entry):

| cart entry | SH2 | status | name |
|---|---|---|---|
| 0x24852c | 0x0224852c | prior wave | actor_probe_handler |
| 0x2485b8 | 0x022485b8 | **this wave** | actor_grapple_handler |
| 0x24886e | 0x0224886e | **this wave** | actor_carry_handler |
| 0x2489d0 | 0x022489d0 | **this wave** | actor_hit_react |
| 0x248bb8 | 0x02248bb8 | **DEFERRED** | (double jump-table dispatcher) |
| 0x248f88 | 0x02248f88 | **this wave** | actor_command_handler |
| 0x2490a0 | 0x022490a0 | **this wave** | actor_spawn_clash_task |
| 0x2490e4 | 0x022490e4 | **this wave** | actor_try_grab |
| 0x24910e | 0x0224910e | prior wave | actor_spawn_sub_init |
| 0x249240 | 0x02249240 | **this wave** | actor_enter_state4 |
| 0x2492f8 | 0x022492f8 | **DEFERRED** | (14-state jump-table machine) |
| 0x249898 | 0x02249898 | **this wave** | actor_grab_lock_handler |
| 0x2499a6 | 0x022499a6 | prior wave | actor_enter_state14 |
| 0x249a04 | 0x02249a04 | **this wave** | actor_hit_or_hazard_handler |
| 0x249a82 | 0x02249a82 | **this wave** | actor_enter_clash |
| 0x249b80 | 0x02249b80 | **this wave** | actor_pick_anim_variant |
| 0x249bba | 0x02249bba | **this wave** | actor_clash_resolve_handler |
| 0x249cda | 0x02249cda | **this wave** | actor_enter_despawn |
| 0x249da8 | 0x02249da8 | **this wave** | actor_retract_task |

## Added this wave (14) - all HIGH fidelity, zero-warning, hand-verified
Files are `project/src_functional/<SH2hex>.c`, function named `f_<cart>`.
See notes/symbols_852c.txt for per-function one-liners. Shared family ABI:
(r4=task, r5=entity), tail animation_update (func_0222dc2c 0x0222dc2c). Setup/leaf
members take a single r4=entity arg (0x2489d0, 0x2490a0, 0x2490e4, 0x249240,
0x249a82, 0x249b80) and one is a task-form task (r4=task) 0x249da8.

Key cross-function findings
- **func_022489d0 = actor_hit_react** is the shared hit/stun interrupt: every
  per-tick handler calls it when `entity[0x4c] != 0`. It snaps the actor next to
  its attacker (`g_060995B0[entity[0x4c]]->unk8`) and picks a reaction animation
  from the command byte entity[0x88] (0x2b/0x2c/0x2d). Default knock-X deltas are
  symmetric: +-43950 / +-50060 chosen by the attacker's facing.
- **Callee arity corrections (verified from their own prologues):**
  func_02227704, func_022444b6 each read only r4 -> genuine 1-arg (m2c over-models
  them with 2-3 args from leftover r5/r6). func_022444b6 RETURNS the clash outcome
  (s32). func_02210020 / func_022449b2 overwrite r4/r5 in prologue -> effectively
  0/1-arg. func_02245e58 writes its result to entity[0x4d] AND leaves it in r5,
  which is why 0x249240's `add #-4,r5` (m2c "unset r5") is faithfully M8(,0x4d)-4.
- **func_0223ffcc(task, entity) -> new-state** is the common "enter custom state"
  callee; its return goes into entity[0xd8]. Consistent across 0x2485b8/0x24886e/
  0x248f88 and the prior 0x24852c.
- **g_060995B0** (void*[] indexed by actor-slot byte, each ->unk8 is the actor
  record) is the player/partner slot table used throughout. Clash globals match
  symbols.txt's actor_clash_state family (0x0609882c, 0x06099764, 0x06097128,
  0x0609977c, 0x06099580, 0x0609958c).

## Deferred (2) - with reasons and a precise roadmap
Both are large runtime **braf jump-table dispatchers**. m2c cannot lower a runtime
`braf Rn` table - it collapses the whole table into the fall-through (first) case
(verified: m2c emitted one linear flow + `M2C_ERROR(unknown instruction: braf)`),
so the dispatch MUST be hand-decoded. I decoded both tables exactly (below) but did
NOT ship them: each table target is itself a dense block (coordinate math, combo
sub-tables, 15-40 further bsrf calls), i.e. a function's worth of tracing apiece,
and the house rule is zero logical bugs / defer-with-reason over rushing. They are
fully tractable for a focused follow-up; everything needed is here.

### 0x2492f8 - 14-state combat-combo machine (0x2492f8..0x249898, ~0x5a0 bytes)
ABI (r4=task, r5=entity). Prologue consts: r6=0xdfff(mask), r7=8, r8=7, r9=1,
r10=3, r11=0, r12=animation_update (ptr_resolve: mova 0x249320 + 0xfffe490c =
0x0222dc2c). Dispatch: `temp = (u16)entity[0x54]&2` then `if (entity[0xd8] >= 14)
return;` then braf table.

Jump table @ **0x249354** (14 x s16 offsets), **braf PC = 0x249344**, so
`target = 0x249348 + (s16)tbl[state]`. Decoded state -> block entry:
```
 0 -> 0x249370   1 -> 0x249404   2 -> 0x24941e   3 -> 0x2494a4
 4 -> 0x2494da   5 -> 0x249590   6 -> 0x2495e0   7 -> 0x249600
 8 -> 0x24965a   9 -> 0x2496d8  10 -> 0x24972a  11 -> 0x249788
12 -> 0x24979c  13 -> 0x2497ec
```
Shared tail blocks: **.L249822** (= "animation already issued this path, pop & ret"),
**.L24981c** (pop & ret, a second epilogue), **.L249742** (state-10 wrap tail).
State 4 (0x2494da) has a SECONDARY cmp-chain sub-dispatch on func_022454e4()'s
return (0..6). Combo sub-tables used by state 0: 0x060540e8 (s8[]), 0x06054080
(24-ish-byte records), 0x06053f04 (s16[]), 0x060e0ddc (enable flag).
Resolved bsrf targets inside the extent (braf_resolve):
```
2493ba->func_02215f6c  2493e8->func_022458ec  2493f6->func_022469f2
2494f0->func_022454e4  249592->func_022449b2  2495e2->func_022449b2
249602->func_022449b2  249636->func_022469f2  249640->func_02245eb8
249652->func_022469f2  24978e->func_022469f2  2497a8->func_02245e58
2497ae->func_0223d0c6  249804->func_0223d0fe  24980a->func_02245c88
249818->func_02241d6e  249854->func_02210020  249886->func_02239ce8
```
Also tails to sibling handlers: 0x249240 (bra, state3/9), 0x249cda (bra, state8),
0x2499a6 (bsr, state7). PIC pointers all in the clean 0x249xxx pool (resolve raw).

### 0x248bb8 - double jump-table command dispatcher (0x248bb8..0x248f88, ~0x3d0 bytes)
ABI (r4=task, r5=entity). r11=animation_update (ptr_resolve 0x248bd8+0xfffe5054),
r9=func_02244988 (ptr_resolve mova 0x248c44+0xffffbd44 = 0x02244988), r10=1, r12=0,
r5=entity[0x34] (prologue). Top-level switch (entity[0xd8]): state0 (anim + maybe
unk64=0/unkD8=1 when engaged), state1 (the dispatch); else return.

State 1 (entity[0x4c]==0) splits on `entity[0x56] > 0`:
- **unk56 <= 0**: cmp-chain on entity[0x88]: 0x3b/0x3c->0x248c84, 0x3d/default->
  0x248d18 (tail func_0223ffcc), 0x42->0x248d64, 0x43->0x248d78, 0x44->0x248cea,
  0x49->0x248d04, 0x4a->0x248cca. (All straightforward; already traced, could be
  shipped alone, but kept with the whole function.)
- **unk56 > 0**: `cmd = entity[0x88]; if ((cmd-0x28) < 35)` braf table, else return.
  Jump table @ **0x248db8** (35 x s16), **braf PC = 0x248db2**, `target =
  0x248db6 + (s16)tbl[cmd-0x28]`. Decoded (cmd = index + 0x28):
  ```
  0x28 -> 0x248e9e                       0x30 -> 0x248f34
  0x33,0x34,0x37 -> 0x248dfe             0x35,0x38,0x39 -> 0x248e30
  0x3a -> 0x248ed2   0x3b -> 0x248eea    0x3c -> 0x248e4e
  0x49 -> 0x248e66   0x4a -> 0x248e7e
  all other cmds -> 0x248f72 (= return, no-op)
  ```
  Shared tails: .L248f6c (anim + ret), .L248f72 (bare ret).
Resolved bsrf in range: 248cb2->func_02215f6c, 248d10->func_0223ffcc,
248e18->func_02247d6e, 248e28->func_022469f2, 248e72->func_0223ffcc,
248eb8->func_02210020; tails 248d2e->func_0249cda (braf), plus direct bsr to
func_249cda / func_249240. NOTE: the first `bsrf r2` at 0x248cb2 has a non-nop
delay slot (`or r3,r4`) so braf_rewrite leaves it raw - resolve by hand to
func_02215f6c. Also decomp_bounded's --asm WRONGLY collapses the 0x248db2 table
braf to a single `func_02249cda` - ignore it and use the decoded table above.

## Remaining undone genuine code in the extent
- 0x2492f8 (14-state combo machine) and 0x248bb8 (double command table) - both
  deferred above with decoded tables + resolved callees. Nothing else remains in
  `[0x24852c, 0x249e68)`: 3 prior + 14 this wave + 2 deferred = 19 genuine funcs.
