# Post-81 gameplay scene + the credit -> director wire (decomp analysis)

Wave goal: find/decompile the scene the director advances INTO after scene 81 (where the
player-character must spawn) and map the credit->director coupling. Read-only decomp on p620
(`~/decomp/diehard-stv`), cross-checked vs dh-portA notes. Honest; correctness over completion.

Builds on and CORRECTS parts of: notes/port/round_unblock_Z.md, credit_join_trigger.md,
scene_director_status.md, player_spawn_scene_status.md.

---

## TL;DR (the three answers)

1. **There is no separate "post-81 gameplay scene" in the table.** Scene 81 is the
   stage-enter/intro scene and is a FIXED POINT in the director. Its `next44 = 0` is the REAL
   static value, not a corruption: 81 -> scene 0 -> (scene-0 selector -1) -> **scene 2**, the
   first gameplay stage. The real gameplay/round scenes are the STAGE scenes (2..72, setup
   74..78), whose off14 handlers are `func_0221b080`->`func_02218038` (even) / `func_02232610`
   (odd) etc. - all already decompiled. I decompiled the one missing link, **scene 0's handler
   `func_02205c96`** (the literal `next44=0` target), to `project/src_functional/02205c96.c`.

2. **The player-character spawn-script is installed as scene DATA at descriptor `+0x1C`** (a
   3-pointer spawn-list sub-record), consumed by `scene_value_select` (`func_0222a878`) ->
   `process_spawn_list` (`func_0222a5d0`) -> type-1 entry -> `func_02227bc4(0)` -> player-actor
   FSM `func_0222a300`. Verified in the ROM: stage scenes 2/3/4/6 spawn-lists literally begin
   with a `type=1` (Player 1) entry. Scenes 79/80/81 have `+0x1C = 0` -> no player spawn.

3. **The missing credit->director wire is BIOS/system-synthesized, not cart code.** On a real
   coin-op boot the director starts at **scene 1** (credit-wait, off14 = `func_0220600e`). Scene
   1's selector-4 advances the director to scene 2 (gameplay) when `g_06098670` is armed; the
   actual start is gated by the START-enable bits `0x06098798` bit4/bit5, which **no cart code
   ever writes** - they are supplied by the ST-V BIOS on a real credit+START. The port has no
   BIOS to supply them, so the director never leaves the attract/credit park (or, when forced,
   dies on scene 81's reset-timeout). The coupling is data-driven selectors + a BIOS signal; the
   cart has no instruction that writes the director index for the credit->gameplay transition.

---

## 1. The scene table past 73, and why 81 is terminal

Scene table: cart `0x26E660` (hwram `0x06072660`), stride `0x50`, 90 entries. Handler fields hold
HWRAM pointers (`hwram = cart + 0x03E04000`). Selector byte `[0x2B]` signed; next indices at
`[0x2C]`(=44) / `[0x2D]`(=45). Director `func_02205b20` selects per `[0x2B]`:
`-1`->2, `0`->n44, `1`->n44+(g&1), `2`->n44+(g&3), `3`->(P1&P2 credit==2 ? n45 : n44),
`4`->(g_06098670 ? n45 : n44).

Reel topology (from the ROM):

```
0  sel -1 -> 2                         (attract/scene-select pivot: func_02205c96)
1  sel  4 -> 2 (armed) / 1 (loop)      (credit-wait: func_0220600e scene_credit_join_sm)
2..72  linear stage reel (n44 = next)  (gameplay stages; +0x1C spawn-lists w/ player entries)
73 sel  3 -> 79 (1P) / 74 (2P both)    (credit/character branch)
74 -> 75 -> {76,77,78 by sel 2} -> 79  (2P character/setup scenes)
79 -> 80 -> 81                          (final setup -> game-enter)
81 sel  0, n44 = 0 -> scene 0           (TERMINAL in the director)
```

`func_02205b20` special-cases 81: once `g_06098668 == 81` it **early-returns** every frame
(re-asserts `entity->unk3c` bit0, clears `g_06098670`, plays a sound) and never reads the
selector, never advances, and the reset-lever (`task->unk13 < 0`) branch is also skipped. So in
pure cart logic scene 81 is a fixed point. Scene 81's game SM `func_02206876` state 5 sets the
director-advance bit, but the director ignores it at 81. Nothing in decompiled cart code moves
`g_06098668` off 81 by itself -> this is exactly the port's observed "81 -> reset-timeout ->
reboot to attract" loop.

**What `next44 = 0` points to:** scene 0, handler `func_02205c96` (decompiled this wave). Scene 0
state 0 forces `g_06098668 = 2`; its own selector is `-1` -> idx 2. So the intended flow out of
81 is `81 -> 0 -> 2` = back to the first gameplay STAGE. "Attract fallback" and "first stage" are
the SAME scene (scene 2); only the CONTEXT differs (credit bytes / `g_06098670` / human vs demo
input), not the scene index.

### Scene 81's two handlers (both already decompiled - not the round loop)
- `+0x14` off14 = `func_02206876` scene81_game_sm: START gate (`0x06098798` bit4/5) + stage
  setup (states 0->4->1->2->3->5); state 5 = game-start COMMIT (teardown + director-advance bit).
  NO player spawn here (no `+0x1C`).
- `+0x10` off16 = `func_02206c88`: the 3D camera intro mover (dolly/crane, parks at state 3).
  NOT the round loop (prior wave's "candidate home" for the player spawn was WRONG - this is a
  camera handler; confirmed this wave against the decompiled file).

---

## 2. Where the player-character spawn-script installs (JOB 2)

The install point is DATA, not code: the scene descriptor's `+0x1C` field points at a 3-pointer
sub-record of spawn-lists:
- `unk0` = 1P, even parity ; `unk4` = 1P, odd parity ; `unk8` = 2P / default (also the attract
  demo path).

`scene_value_select` (`func_0222a878`) picks one: if `g_06098670 == 1` (2P present) it uses
`func_0222a8d8()` to choose unk0/unk4 by parity `g_06098564`; otherwise it returns `unk8`.

An `evt_actor_spawn_*` task (`func_0221ca1e`, `0221d4fc`, `0221bf72`, `0221c690`, `0221cee8`,
`0221eb62`, `0221f004`, `0221f088`, ... all already decompiled) runs inside the stage and does:
```
entity->unk30 = scene_value_select();   // the chosen spawn-list
process_spawn_list(entity);             // func_0222a5d0
```
`process_spawn_list` walks 24-byte entries. `type` = `(s8)entry->unk0`:
- `type == 0` -> `func_02227b60(&func_0222a300)`  (dynamic-id NPC, same player FSM family)
- `type == 7` -> `func_02227b60(&func_0222a970)`  (dynamic-id anim-init)
- else (**type-1**) -> `func_02227bc4(type - 1)`  (FIXED id = type-1 slot)

`func_02227bc4(0)` registers the actor at **slot id 1 = Player 1** in table `0x06099584`, installs
`func_0220ff0a`-spawned object, stamps `obj->unk34 = 1`. The spawn record seeds position
(`unk8/C/10`), config pointer (`unkD0 = entry->unk14`), facing (`unk5E`). The player-actor then
runs `func_0222a300` (arm) -> `func_0222a418` (config-script anim step).

**ROM-verified player entries** (spawn-lists begin with the player):
- Scene 2 (first stage): entries `type=1`(P1, cfg 0x06063B98) and `type=2`(P2) + NPCs (`type=0`).
- Scene 3: `unk0` (1P-even) = `type=1`(P1) only; `unk4` (1P-odd) = `type=2`(P2) only;
  `unk8` (2P) = both P1+P2. (Parity selects which character spawns in 1P.)
- Scene 4/6 similar (P1/P2 leading entries).
- Scenes 79/80/81: `+0x1C = 0` -> these scenes spawn NO player. The player is a STAGE actor.

So the player-character spawn-script is installed by **every gameplay stage scene** via its
`+0x1C` spawn-list; it is NOT installed at/around scene 81. The credit-join chain
(`022045f8 -> 02228168 -> ...`) ALSO spawns a player-family actor via `func_02227bc4`, but that is
the coin/continue JOIN coordinator, architecturally decoupled from the scene director (see §3).

---

## 3. The missing credit -> director wire (JOB 3)

### The real ST-V flow (reconstructed, all cart-side pieces decompiled)
1. **Boot** `func_022058a8` (scene_bootstrap) sets the initial director index:
   - test/service mode (`0x06000798` bit5) -> `g_06098668 = 0` (scene 0).
   - auto-start dip (`0x06000654 & 0x100 == 0`) -> `g_06098670 = 1`, `g_06098668 = 0`, installs
     attract_handler.
   - **normal coin-op (else) -> `g_06098668 = 1`**  (director starts at the CREDIT-WAIT scene).
2. **Scene 1** off14 = `func_0220600e` (scene_credit_join_sm): draws INSERT-COIN / PRESS-START,
   reads the panel via BIOS `(*0x06000660)`, consumes coins via BIOS `(*0x06000664)`, reads the
   coin countdown from BIOS word `0x06000654`. On join it sets `g_06098670 = 1`.
3. **Scene 1 selector = 4**: `(g_06098670 != 0) ? next45(=2) : next44(=1)`. So once armed, the
   director advances scene 1 -> **scene 2 = first gameplay stage**. (This is the data-level
   credit->director coupling. There is also scene 73 selector-3, the mid-game continue check on
   the credit bytes.)
4. In scene 2+, `process_spawn_list` spawns the player (type-1). With a live credit the player is
   HUMAN-driven; in attract the SAME scenes run as the demo (AI-driven). The spawn mechanism is
   identical - only input routing + context differ.
5. The actual round START is gated by `0x06098798` bit4 (P1) / bit5 (P2) ("start-enable"),
   read by scene81_game_sm, scene_credit_join_sm, and the boot-director preamble.

### What is missing (why the port can't reach a live round)
A full image scan (prior waves) confirms **no cart instruction writes `0x06098798`**; the cart
only reads it. Likewise the coin/credit services behind `*0x06000660 / *0x06000664 / *0x06000680`
and the words `0x06000654 / 0x06000798` are **BIOS/system fn-ptr slots** the cart calls but does
not implement. These are the ST-V BIOS. On real hardware the BIOS:
- synthesizes `0x06098798` start-enable on a real credit + panel START, and
- backs the coin/credit services + config/dip words.

Without the BIOS, `g_06098670` never arms from a real coin and the start-enable bits never set, so
scene 1 never advances to scene 2, the director parks on the attract/credit scene, and any forced
jump lands on the terminal scene 81 (reset-timeout -> reboot). **No recomp seed and no additional
decompile fixes this** - it is a BIOS-emulation gap, exactly as `round_unblock_Z.md` /
`credit_join_trigger.md` concluded, now pinned to the specific signals and the specific
data-driven selectors (scene 1 sel-4 / scene 73 sel-3) that consume them.

### Correction to the prior "post-81 off16 = player home" hypothesis
`round_unblock_Z.md` (DIRECTOR-COUPLING RESULT) listed scene-81 off16 `func_02206c88` as the
candidate player-spawn home. That is wrong: `func_02206c88` is the camera-intro mover (decompiled;
parks at state 3, no spawn). The player's home is the STAGE scenes' `+0x1C` spawn-lists, reached
via `81 -> scene 0 -> scene 2` (or directly on the credited scene-1 -> scene-2 advance).

---

## 4. Shipped vs deferred

SHIPPED (to `project/src_functional/`, compiles clean `-O1 -c -Wall -Wextra`, zero output):
- `02205c96.c` scene0_attract_select - scene-0 off14 handler = the literal `next44=0` target.
  HIGH fidelity (hand-traced vs m2c --asm; all 11 PIC calls + tail braf resolved). The only
  modeled uncertainty is the variadic UI-print thunk `(*0x060902A0)` ABI (documented in-file).

DEFERRED (deliberately, correctness over completion):
- `02228168.c` / `02228332.c` (credit-join context builder + spawn). Semantics already fully
  hand-decoded in `credit_join_trigger.md` §3; it is a ~0x390-byte dual-entry function and is the
  CREDIT-JOIN variant spawn (decoupled from the director), not the gameplay spawn. Writing it to
  HIGH fidelity is a sizable job and it is not on the critical path for this wave's questions;
  flagged for a dedicated pass. The gameplay player-spawn path (process_spawn_list ->
  func_02227bc4 -> func_0222a300/a418) is already fully decompiled.

NOT TOUCHED (per constraints): git, shared headers, tools/, port/ runtime, notes/symbols.txt.

## 5. Next step
- Port team: with the start-enable (`0x06098798` bit4/5) harness knob held AND the director parked
  on a real STAGE scene (2/3) rather than jumped to 81, confirm `process_spawn_list` reaches a
  `type=1` entry and the human player actor (`func_0222a300`) spawns + persists. This is the clean
  test the prior waves never ran (they parked before the content task or jumped past the stages).
- Decomp: do the dedicated `func_02228168`/`02228332` pass (credit-join spawn) to close the last
  undecompiled function in the credit chain.
