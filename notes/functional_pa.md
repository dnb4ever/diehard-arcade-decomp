# Functional decomp batch - PLAYER-ACTOR / round-cycle cluster (range 0x229000..0x22e000)

Worker: player-actor FSM. Range = cart offsets [0x229000, 0x22e000) (SH2 0x02229000..0x0222e000).
All new .c compile clean with `gcc -c -fsyntax-only -Wall -Wextra` (ZERO warnings) and were
hand-verified statement-for-statement against `m2c_fn.py --asm`.

## Added functions (4)

| file                | SH2 addr   | name                             | notes |
|---------------------|------------|----------------------------------|-------|
| 0222bd54.c          | 0x0222bd54 | actor_state_enter_mode_dispatch  | state-entry setup; per-game-mode fn-ptr dispatch via g_060911bc[g_0609979a] |
| 0222ae40.c          | 0x0222ae40 | actor_facing_anim_select         | per-frame facing/animation frame selector (ground 8-dir / 2-dir) |
| 0222baf8.c          | 0x0222baf8 | actor_jump_anim_select           | airborne/special-move anim selector (sibling of 0x0222ae40) |
| 0222bdf2.c          | 0x0222bdf2 | player_input_anim_select         | walk/turn anim selector from the per-player directional-input record |

These four are the player-actor animation/facing-state handlers that `actor_main_state_step`
(0x0229074, already done) and the arm/join states route into each frame. They all program the
actor's current animation id (unk5C), heading (unk26), motion-state flags (unk84) and then feed
`animation_update` (0x0222dc2c) directly or via the common per-frame driver func_02216110.

### Key cross-checks / corrections
- In 0x0222ae40 the registers r5=0x400 and r6=0x30 are **flag-test bitmask constants** (used in
  `tst r5/r6, unk84`), NOT arguments to the hit-box helpers func_02227704 / func_02223e70. Those
  helpers take only (entity) - confirmed by 0x0222baf8 which calls the same two with a single arg
  and by the existing extern in 0222a5d0.c / 02229df4.c. Both files decode them as 1-arg.
- `find_entry.py` was essential: many triage "function" rows in this band are pool-contaminated
  phantoms that remap onto already-done real entries (e.g. 0x22a2e8->0x22a300[done],
  0x22a3e2->0x22a418[done], 0x229d9e->0x229df4[done], 0x22ae1c->0x22ae40, 0x22bfc2->0x22c220[done],
  0x22dbf6->0x22dc2c). Ran it on every candidate before touching it.

## PLAYER-ACTOR SPAWN INSIGHT (for the port's round cycle)

Traced the full "what installs the player-actor entry handler" chain. The 0x2229b60 handler
(player_join_arm) is a LATER state, not the spawn handler. The actual spawn path:

1. **Scene spawn-scripts** in the 0x0221caxx..0x0221fxx band (e.g. evt_actor_spawn_child
   0x0221f004, and siblings 0x0221eb62/0x0221f262/0x0221ccf8/...) set `entity->unk30` to a
   24-byte spawn-descriptor list (chosen by `scene_value_select` 0x0222a878) and call
   **process_spawn_list (0x0222a5d0)**. A paced variant, **actor_spawn_list_tick (0x022a120)**,
   drives the same descriptor table on a frame timer.
2. **process_spawn_list** walks the descriptor list. For a **type-0 descriptor (player actor)** it
   calls **func_02227b60(&func_0222a300)** - the dynamic-id task registrar (already decompiled) -
   which creates a new task whose **state handler is func_0222a300**. (type 7 -> func_0222a970;
   other -> fixed-id spawner func_02227bc4.)
3. **func_0222a300** is the player-actor's true ENTRY/ARM handler. It initialises the entity
   (anim-param init via func_0222db44, slot registration via actor_slot_record_reg, HP seed, etc.),
   sets unk5C/unk41/unk84, runs animation_update once, then **morphs the task to func_0222a418**
   (`*task = &func_0222a418`), which runs the per-frame player FSM - `actor_main_state_step`
   (0x0229074) plus the anim/facing selectors decoded in this batch.
4. The join-countdown states (player_join_arm 0x2229b60 -> join commit 0x2229c00 ->
   player_join_next 0x2229df4) are states the task morphs through later during the join sequence.

**Implication for the port's parked round goal:** a round is replayable by running the scene's
spawn-descriptor list through `process_spawn_list` (0x0222a5d0). The player actor is spawned by
`func_02227b60(&func_0222a300)`; everything downstream (FSM, anim, join) follows from that handler
install. The scene descriptor tables live in RAM at the addresses the 0x0221xxxx scripts compute
(scene->unk1C rows, g_0609856C/g_0609866C-indexed), so the round entry point for the port is the
scene script -> process_spawn_list -> func_02227b60(&func_0222a300) sequence.

## Resisted / deferred (with reasons)

- **0x0222db44** (0xb2, player anim-param initialiser, called by func_0222a300 at spawn): makes
  three `bsrf` calls into the sibling-owned 0x24a280 / 0x24a360 helpers using a **non-standard
  register ABI** (count in r0=108, dst in r1, src in r2) and consumes the first call's **return
  value** in byte arithmetic for unk40. Faithful clean C needs those callees' real signatures,
  which belong to the sibling working 0x24a3xx. Deferred to avoid guessing their ABI. Its ROLE is
  documented: player-actor animation-parameter init (copies anim-entry fields into the per-player
  param block at g_06096328 + unk34*0x6C, seeds g_06096c74[unk34] and unk5C from g_0608ce74[unk6D]).
- **0x0222dc2c animation_update** (the single most-referenced undone function in range): 1016 asm
  lines, 34 computed branches, and its body **crosses the 0x22e000 range boundary** (calls at
  0x22e052..0x22e37a). It is built almost entirely on the sibling's non-standard-ABI 0x24a280 /
  0x24a360 / 0x24a420 helpers plus the 0x213xxx/0x214xxx animation subroutines. Out-of-range body
  + sibling-ABI dependency -> deferred.
- **0x0222b0fe** (0x9fa, cc=38, mac.l), **0x0222c39c** (0x6b8, cc=10, div1+mac.l),
  **0x0222ca98** (0x650, cc=134 - the player_join_next tail dispatch), **0x0222d0e8** (0x326,
  cc=63, with a jump table at 0x22d448), **0x0222d40e** (interior of the 0x22d0e8 unit, jump-table
  dispatch): large switch/dispatch bodies. Deferred for size/quality; no M2C_ERROR stubs produced.
