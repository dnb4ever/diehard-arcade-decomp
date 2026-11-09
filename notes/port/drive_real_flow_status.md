# Die Hard Arcade (ST-V) RECOMP — DRIVING the real attract->play flow (no index-forcing)

Date: 2026-10-20. Port side only (port/ + tools/saturn-recomp/). Work in isolated copy ~/decomp/dh-portA/.
Address map: runtime(HWRAM) = cart SH-2 addr + 0x03E04000; cart SH-2 addr = cart.bin offset + 0x02000000.
Builds on: play_transition_status.md, player_spawn_scene_status.md, cooked_input_gate_status.md.
Reference C (NOT built into the port; the spec for what the recompiled code does): main's project/src_functional/
{022058a8 scene_bootstrap, 02204868 attract_handler, 022045f8 attract_credit_join_service,
02204afa round_controller_join, 02205232 round_controller_dual, 02205812 scene_run_handler,
02229c00 player_join_commit_fsm (SOLE setter of g_06098688), 02229b60 player_join_arm, 022016ac boot_scene_director}.

## TL;DR — the harness ALREADY drives the real flow; it runs live and self-morphs correctly up to attract_handler, then stalls at a genuine, decomp-confirmed dependency cycle (NOT a forcing artifact).

A clean boot with NO scene-forcing knobs (all STV_SCENE* default-OFF, gated behind the early-return in
stv_scene_probe_tick) runs the genuine native boot code and the real self-morphing resident task. Verified
by STV_COV/STV_COVDUMP coverage + a per-frame store watch on the task handler slot:

- scene_bootstrap RUNS and, at VB201, morphs the resident task object (*g_06098678 = obj 0x06092264) to
  attract_handler by writing the RUNTIME address 0x06008868 into the object's handler slot. (The recomp
  correctly translates the decomp's cart-address morph pointer 0x02204868 -> runtime 0x06008868.)
- attract_handler is ticked LIVE every frame from VB201 onward (handler slot = 0x06008868 steady;
  attract_credit_join_service 0x060085F8 runs each frame).
- boot_scene_director (func_022016a0, 0x060056AC) ticks (idles in its steady service state; see S4).
- A real COIN (default --stvinput 200:COIN) is delivered through the real BIOS panel/coin path and accrues a
  credit: g_06098674 goes 0->1 (coin-accept edge) ->2 (join-confirm edge) at VB232.

It then STOPS: the attract->round morph never fires, and nothing downstream runs. Per-chain COV (ran flag):

  scene_bootstrap            0x060098A8  ran=1   <- installs attract_handler, live
  attract_handler            0x06008868  ran=1   <- ticked live every frame
  attract_credit_join_service 0x060085F8 ran=1   <- ticked live every frame
  round_controller_join      0x06008AFA  ran=0   <- MORPH NEVER FIRES
  round_controller_dual      0x06009232  ran=0   <- MORPH NEVER FIRES
  scene_run_handler          0x06009812  ran=0   <- never entered play
  scene81_game_sm            0x0600A876  ran=0
  player_join_arm            0x0602DB60  ran=0   <- no player actor spawned
  player_join_commit_fsm     0x0602DC00  ran=0   <- no player actor spawned
  (0x0602Dxxx player-actor region coverage: 0 lines.)

## Per-item answers to the mission bar

| item | result | evidence |
|---|---|---|
| does scene_bootstrap run? | YES | COV 0x060098A8 ran=1; handler-slot store 0x06092264=0x06008868 by fn 060098A8-path at VB201 |
| is attract_handler ticked live? | YES (every frame) | COV 0x06008868 ran=1; handler slot = 0x06008868 steady VB201..end; credit service 060085F8 runs each frame |
| does boot_scene_director tick? | YES | COV 0x060056AC ran=1 |
| does START morph it to a round_controller? | NO | round_controller_join/dual ran=0; handler slot never changes off 0x06008868 |
| does it reach scene_run_handler? | NO | COV 0x06009812 ran=0 |
| does a player actor spawn? | NO | 0x0602Dxxx coverage = 0; player_join_arm/commit ran=0 |
| frames? | attract only | run/final/shot-{250,500,800}.png — "PLEASE WAIT" over the night-skyline (RBG1 rendering); no round/stage/characters |
| new gameplay walls? | NONE | all runs exit 0; no "call to ADDR, not an entry" faults; no new seeds |

## WHY the real chain breaks — exact condition, with evidence (a genuine chicken-and-egg)

attract_handler morphs iff `(s8)g_06098688 + (s8)g_06098689 != 0` (ref 02204868.c). That START-present flag
is NEVER set: a --watch on 0x06098674..0x0609868F across the whole run shows g_06098688 is only ever
CLEARED (VB0, and VB201 by the scene-setup fn 0x0600981A), never set to 1.

The SOLE setter of g_06098688 in the whole 2MB image is the player-join-commit FSM at cart 0x2229cac
(rt 0x0602DCAC), inside handler func_02229c00 (rt 0x0602DC00) — a PLAYER-CHARACTER actor FSM state. It
raises g_06098688[idx-1]=1 only when that actor's join countdown (entity[0xc8]) and a per-player word
counter (g_0609978c) both reach 0 (confirmed by reading project/src_functional/02229c00.c + symbols.txt).

That actor is never spawned:
 - Its entry handler 0x2229b60 (player_join_arm) is installed by NO code — it is data-driven, installed
   from a scene SPAWN-SCRIPT consumed by process_spawn_list (0x0602E5D0). (grep of all decomp: zero code
   references to 0x229b60 as an installed handler.)
 - process_spawn_list DOES run (COV ran=1) but, on the attract scene, finds a NULL spawn-script and exits
   (matches player_spawn_scene_status.md S4). The attract demo only spawns the demo-actor family
   (0x0602A/B/E...), never the player-actor family (0x0602Dxxx).
 - The scene the task ticks never changes to a credit/join or game scene: a --watch on g_06098668 shows
   the scene index stays 2 (attract demo, descriptor 0x06072700) for the entire run AFTER the coin; it is
   never set to 73 (credit/join) / 81 (game). The scene director idles on scene 2.
 - func_0223dcc2 (the join-confirm callee that DOES run at VB232) is a per-player jump-table HUD/record
   setup routine (disasm: `braf` dispatch on (arg-1)*16 + table 0x060e0e38), NOT a task spawner. So the
   credit-join confirm does not spawn the join actor either.

THE CYCLE (now confirmed end-to-end under the real, un-forced flow, with the full chain decompiled):
  attract morph  <-  g_06098688 start flag  <-  player-join actor (0x0602Dxxx)  <-  scene spawn-script
  <-  advance to the credit/join or game-enter scene  <-  (the attract morph itself, OR boot_scene_director
  states 5-10 whose trigger is still un-decompiled; boot_scene_director idles in state 4, per
  play_transition_status.md S5, which I re-confirm: states 5-10 never run).

Every door into the player-actor subsystem is behind another door inside the ring, within the code the
harness can currently reach. This is the same knot play_transition_status.md mapped on 2026-10-19 — now
re-tested with (a) ALL index-forcing removed and (b) the full attract/round/run/join chain decompiled to C.
The new decomp illuminates the complete DOWNSTREAM chain (what each handler does once reached) but does NOT
expose a reachable spawner for the player-join actor during attract.

## Was the OLD "forcing" the blocker? NO.

The premise that STV_SCENEADV/SEED/etc. force-setting the scene index was preventing the real morph chain
is disproven: with every STV_SCENE* knob OFF (default), the real flow runs on its own (scene_bootstrap ->
attract_handler live, coin -> credit) and still stalls at exactly the same place. The forcing knobs are
opt-in diagnostics (default-off, early-return guarded); the default harness path is already faithful. The
resident task *g_06098678 IS being ticked correctly each frame — there is no ticking bug to fix. The
blocker is upstream of the attract loop (the un-reachable / un-decompiled player-actor spawn trigger), not
in the harness drive.

## What a faithful breakthrough needs (no index-forcing)

One of, and only these are faithful (not index-forcing / not flag-poking g_06098688):
 1. Decompile scene_credit_join_sm (cart 0x20600e, rt 0x0600A00E; currently DEFERRED, not in src_functional
    — note it is actively being decompiled in main as of this session: project files 0220600e.c / credit.asm)
    to see whether it is the credit/join scene handler that, on a panel START with a credit, installs the
    player-join actor's spawn-script — i.e. the reachable door into 0x0602Dxxx. It is NOT currently ticked
    (COV 0x0600A00E = 0), so also determine what should instantiate/tick it (the scene director advancing to
    scene 73).
 2. OR decompile the boot_scene_director states 5-10 trigger (the separately-event-driven scene-instantiate
    sequence) — what sets entity->unk3c bit0 faithfully to advance the director from the attract loop into
    the game-enter scene sequence (72->73->...->81), which carries the player-actor spawn-script.

Either resolves the cycle: a reachable spawn of the player-join actor -> its FSM commits -> g_06098688=1 ->
attract_handler morphs -> round_controller -> scene_run_handler -> play. Nothing in the harness needs to
change for the morph itself; it already fires the instant g_06098688 is legitimately set.

## Files changed (isolated copy ~/decomp/dh-portA/) — EXACT list for reconciliation

MODIFIED (source — reconcile into main tree):
 - port/main_stv.cpp — extended the STV_COV diagnostic probe (default-OFF; prints only when STV_COV is set,
   at end of run) to cover the FULL morph chain: scene_bootstrap 0x060098A8, attract_credit_join_service
   0x060085F8, round_controller_join 0x06008AFA, round_controller_dual 0x06009232, scene_run_handler
   0x06009812, scene81_game_sm 0x0600A876, player_join_arm 0x0602DB60, player_join_commit_fsm 0x0602DC00.
   No behavior change to the real flow; it only names more functions in the ran[] report. This is the
   payoff-test instrument. NOTHING else in main_stv.cpp changed; no scene/flag forcing added.

SYNCED FROM MAIN at session start (task setup; NOT my edits):
 - tools/saturn-recomp/runtime/  (full rsync from main; includes bios.cpp STV_* diag knobs + video.cpp
   vblank tick + vdp2.cpp RBG1 — all default-off / unchanged by me).

REGENERATED (build artifact; no manual edit): port/recomp-build/* (cmake --build).

NEW SEEDS for merge into main port/seeds_extra.txt: NONE (no new walls; all runs exit 0).

NOT modified: project/, tools/decomp/, notes/symbols.txt, the runtime sources, port/seeds_extra.txt,
port/recomp/* .

## Reproduce (p620, in the copy)

    cd ~/decomp/dh-portA/port && cmake --build recomp-build --target saturn_stv -j"$(nproc)"
    # the payoff test — real flow, no forcing, real coin+START, full-chain coverage + frames:
    STV_BOOTMODE=1 STV_COV=1 STV_COVDUMP=1 ./recomp-build/saturn_stv --cart diehard_main.bin \
      --cartrom ../cart.bin --vblanks 900 --out run/final --stvinput "200:COIN,340:START1" \
      --shots 250,500,800
    grep "COV " run/final/log.txt        # chain: bootstrap/attract ran=1; round/run/joinFSM ran=0
    # handler-slot morph trace (resident task obj 0x06092264):
    STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 1200 \
      --out run/w3 --watch 06092264:06092267 --stvinput "200:COIN,340:START1"
    grep "store32 06092264" run/w3/log.txt   # ...->06005B8E (boot)->06008868 (attract) @VB201; then static

## Frames

run/final/shot-{250,500,800}.png (also in ~/decomp/dh-portA/port/run/final/ and the Mac scratchpad).
Visual: the attract/coin-wait screen — "PLEASE WAIT" text over the Die Hard night-skyline RBG1 backdrop,
animating across frames. No round, no characters, no stage — honest; nothing faked.
