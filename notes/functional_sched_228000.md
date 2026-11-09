# Functional decomp batch - scheduler/spawn band (cart 0x228000-0x22b000)

Single serial worker. **20 new functional `.c`**, every one compiling clean with
`gcc -c -fsyntax-only -Wall -Wextra` at **ZERO warnings**, hand-traced statement-for-statement
against bounded SH-2 disassembly (`m2c_fn.py --asm`) with **every computed branch and PIC pointer
resolved** (`ptr_resolve.py` / `braf_resolve.py` / `find_entry.py` run before each feed).

Mission was the PORT-UNBLOCK spawn/animation helper cluster. All explicitly-named priority targets
in range are now in C.

## ⭐ SPECIAL INTEREST - who initializes the spawn-pool context g_06099580
**Already decompiled; no further work needed for the port.** The initializer is
**`func_02227790` (`0x02227790`, batch done, out of my range)** - "construct the dynamic entity
system": it allocates the owner object via `func_02214bb8(self, 1, 0x6C)`, **publishes it to
`g_06099580`**, installs the per-frame handler `0x02227830` as `self->vtable(+0)`, **clears the 8
id/pointer slots** of the entity table `g_06099584` (id[] at +0xC, ptr[] at +0x2C), runs the full
reset `func_02227a84`, and tail-calls `func_0223c95c`.
- The **freelist/slot init** the port asked about is `func_02227a84` (`0x02227a84`, also done) -
  "full reset of entity system at g_06099584 (slots, 8x8 relation matrix, aux words)".
- Actual allocation goes through `func_0220ff0a(g_06099580, template)` (dynamic) / `func_0220ff72`
  (list) - both done.
So the port's "g_06099580 writer/freelist init" path is **entirely in C already**
(`func_02227790` + `func_02227a84` + the spawn allocators). The missing piece was never the
initializer - it was the *consumers* (the spawn-list processors), which this batch delivers.

## Functions added (SH2 addr -> name)
| SH2 addr | cart | name | role |
|---|---|---|---|
| 0x0222a5d0 | 22a5d0 | `process_spawn_list` | **priority #1** - the spawn-list processor (runtime 0x0602E5D0) |
| 0x0222a120 | 22a120 | `actor_spawn_list_tick` | timed/paced spawn-list task driver (sibling of a5d0) |
| 0x02228168 | 228168 | `player_join_spawn` | player-join actor builder (scene-81 state-5 family) |
| 0x02229074 | 229074 | `actor_main_state_step` | main actor state dispatcher (2-level switch on unk88) |
| 0x0222ab18 | 22ab18 | `actor_apply_damage` | HP subtract + late-join trigger (kin to func_0222ac60) |
| 0x02228f08 | 228f08 | `actor_anim_state_dispatch` | anim-phase handler, switch unk88 0x3B-0x4A |
| 0x0222934a | 22934a | `actor_hit_spawn_enter` | spawn linked partner/effect, tail into 0x2293b8 |
| 0x022293b8 | 2293b8 | `actor_hit_state_enter` | hit/grab state entry, switch unk88 0x45-0x49 |
| 0x0222ae40 | 22ae40 | `actor_pose_resolve` | directional pose/anim resolver (8-dir x 4-quad) |
| 0x0222afba | 22afba | `actor_pose_resolve_b` | sibling pose resolver (table g_060866F8) |
| 0x02228bb6 | 228bb6 | `func_0228bb6` | actor state handler, tail animation_update |
| 0x02228c70 | 228c70 | `func_0228c70` | actor state handler (unk58>=unk90 -> func_0223a740) |
| 0x02228ca2 | 228ca2 | `func_0228ca2` | actor state handler (idle reset on unk54&2) |
| 0x02228738 | 228738 | `func_0228738` | actor state handler (unk54&1, func_0222c220) |
| 0x02228768 | 228768 | `func_0228768` | actor fall/settle handler (gravity integrate) |
| 0x022286cc | 2286cc | `func_02286cc` | actor state handler (func_0222c39c + anim) |
| 0x0222988e | 22988e | `func_022988e` | actor state handler (pose_dispatch/pose_load) |
| 0x02229bb0 | 229bb0 | `func_0229bb0` | anim + tail func_0229c00 |
| 0x02229bce | 229bce | `func_0229bce` | arm countdown unkC8=60, tail func_0223abd0 |
| 0x02229640 | 229640 | `func_02229640` | empty leaf (rts;nop) no-op slot |

## Shared helpers now done, and what they unblock
- **`process_spawn_list` (0x22a5d0)** and **`actor_spawn_list_tick` (0x22a120)** are the two
  spawn-list consumers. Both dispatch spawn-type faithfully (type 0 -> dynamic-id spawner
  func_02227b60 with arm-state handler func_0222a300; type 7 -> func_02227b60 with anim-init handler
  func_0222a970; else -> fixed-id spawner func_02227bc4), copy the 24-byte descriptor into the
  entity, seed HP (g_06099788 / g_06099794), and run per-entity init func_0222ca54. These were the
  flagged "spawn helpers around 0x22a5d0 / 0x22a708" - now in C. With these plus the already-done
  func_02227790/func_02227a84/func_0220ff0a, **the entire spawn pool path is now expressible in C**,
  which was the port blocker for the dozens of deferred actor handlers that spawn through this pool.
- **`player_join_spawn` (0x228168)** completes the player-actor spawn that scene-81 state 5 drives:
  it is the BEGIN/COMMIT builder that allocates the fixed-id player actor, applies the template and
  per-player facing, and runs the init sequence (func_0222a91a/func_0222ca54/func_0222da90/
  func_0221707c). This is the other half of the batch-12/13 player-join chain.
- **`actor_main_state_step` (0x229074)** is the central actor state dispatcher that the handlers
  above transition into via unk43/unk88; having it in C means the per-state handler table
  (g_06091100 family) is now almost fully covered in this band.
- The remaining ~11 handlers (0x228bb6/c70/ca2/738/768/6cc/988e/f08/293b8/9bb0/9bce) are the
  per-state table entries that the FSM morphs between; each tails animation_update (func_0222dc2c)
  and/or func_02216110, confirming the (task,entity) handler ABI end-to-end.

## Jump-table / computed-dispatch fidelity (per KEY DIRECTIVE)
Every dispatch in range was decoded faithfully, NOT M2C-stubbed:
- `process_spawn_list` / `actor_spawn_list_tick`: the two indirect spawn calls (`jsr @r8`/`jsr @r12`)
  resolved via ptr_resolve to fixed fps func_02227bc4 / func_02227b60 and rendered as direct calls;
  the spawn-type selection is an if/else chain on the descriptor type byte (not a table).
- `actor_main_state_step` / `actor_anim_state_dispatch` / `actor_hit_state_enter` / `func_022988e`:
  rendered as `switch`/`case` on unk88, with every case target traced from the asm (including the
  grouped cases 59/63, 64/65, 70/72).
- All `bsrf`/`braf` computed tail-calls resolved to fixed targets and rendered as direct calls /
  tail calls (no stubs).

## Resisted / deferred (honest)
- **0x22992c (per-entity table dispatcher, ~0x200 bytes).** A hidden sub-function inside the
  0x22988e member's over-run extent. It does a **runtime function-pointer-table dispatch**:
  `fp = ((void(**)(void*))g_<0x06099... base @pool 0x229a10>)[ g_<pool 0x229a0c> ]; fp(entity);`
  i.e. the table lives in **RAM (0x06xxxxxx)** and the index comes from a **runtime global**, so it
  is NOT a static ROM jump table - it is a genuine indirect call and would be rendered as one (no
  ROM table to read). It also carries a secondary `switch` on unk88 (84/85/86 ...). Decompilable but
  large; deferred for budget this batch. CLEAN next target - entry confirmed at 0x22992c.
- **Third sub-function in the 0x22988e extent (~0x229ad0 region).** Another hidden sibling past the
  dispatcher; not traced this batch.
- **0x228730/0x228740 fragments.** These are NOT separate functions - they are the leading
  pool-contaminated bytes / mid-body of func_0228738 (entry recovered at 0x228738) and the body of
  func_0228768 (entry 0x228768). Handled by bounding to the real prologues; no separate shipment.

## Callees called faithfully but still undecompiled (declared extern)
func_0223dd96, func_0223e3d0, func_0223dcc2, func_0223a6dc, func_0223a740, func_0223a9d0,
func_0223abd0, func_02216110, func_022160a0, func_0222c220, func_0222c322, func_0222c39c,
func_0222bd54, func_0222bd90, func_0222da90, func_02210020, func_022170b4, func_0221707c,
func_0220ff72. (All out of this range; declared with the arg-arity the call sites set.)

## Notes for the user
- Range obeyed strictly (0x228000 <= cart < 0x22b000). Wrote only new `.c` in
  project/src_functional/, notes/symbols_sched_228000.txt, this file. Did NOT touch shared headers,
  tools/decomp/*.py, port/, tools/saturn-recomp/, or run git.
- Several band members are over-run TUs containing multiple real functions (find_entry/extent
  grouping): I split and shipped the clean sub-functions (2293b8, 228c70, 228ca2, 228738, 228768)
  under their true entries.
