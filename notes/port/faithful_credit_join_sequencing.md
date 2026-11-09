# Faithful credit-join sequencing — countdown + coin-seq -> g_06098688 by REAL cart code (2026-10-22)

Work in ~/decomp/dh-portB only. Runtime edits logged below. Ghidra read-only. No git.
Reproduce base: STV_BOOTMODE=0 STV_CFG798=40. Build: cmake --build recomp-build --target saturn_stv.

## RESULT (headline) -- UPDATED 2026-10-22: STV_CLEARSLOT2 ELIMINATED (now fully faithful to ENTER PLAY)
The start flag g_06098688 is set FAITHFULLY (real cart code, NO STV_STARTFLAG) AND the executive now
morphs attract_handler -> round_controller_DUAL -> scene_run -> scene81 (ENTER PLAY) with NO FORCE KNOB.
The last forced element (STV_CLEARSLOT2, the demo-P2 slot poke) is REPLACED by STV_DEMOSTOP, which runs
the game's OWN per-slot despawn func_02227c5c (cart 0x02227c5c) on the attract demo's P2 fighter -- the
exact routine a player-actor's terminal FSM runs to clear its own entity slot. Every link in the chain
is now real cart code: the countdown, the coin sequencing, the join coordinator, the per-slot demo
teardown, the start-flag commit, and the executive morph/scene_run/scene81.

COV (STV_BOOTMODE=0 STV_CFG798=40 STV_COINSEQ=100 STV_DEMOSTOP=980, --stvinput "99999:COIN" -- NO force):
  player_join_commit_fsm(0x02229c00) ran=1   <- FAITHFUL start-flag setter
  round_controller_join(0x02204afa)  ran=0
  round_controller_dual(0x02205232)  ran=1   <- the DUAL path (NO STV_CLEARSLOT2)
  scene_run_handler(0x02205812)      ran=1   <- ENTER PLAY
  scene81_game_sm(0x02206876)        ran=1
Slot watch (STV_SLOTTRACE): demo spawns id[1]=1 & id[2]=2 on fade scene 3 @VB980; STV_DEMOSTOP despawns
demo P2 via func_02227c5c @VB991 -> id[2]=0; commit @VB1605 clears id[1] -> (id[1]+id[2])==0 ->
attract_handler picks round_controller_DUAL. Scene reaches 81 @VB2889. Frame run/shot/shot-2000.png =
"CONTINUE? 6" (round-enter countdown; INSERT-COIN gone, portraits + health bars over the skyline) --
byte-for-byte the same ENTER-PLAY visual the forced STV_CLEARSLOT2 path produced.

Post-scene81 landing (documented, unchanged gap): the director installs scene 81 @VB2889 (scene_run ->
scene81_game_sm), then the display BLANKS (black by VB3000) and holds -- the separate long-standing
post-81 soft-reset/sustain gap. Reaching ENTER PLAY faithfully was this task's goal; the post-81 sustain
is out of scope here.

For reference, the old forced path (STV_COINSEQ=100 STV_CLEARSLOT2=1) still reaches the SAME COV/frame;
STV_CLEARSLOT2 is now superseded (kept only as a labeled diagnostic). Without EITHER demo-stop
(STV_COINSEQ alone) the executive still morphs to round_controller_JOIN (ran=1) and does NOT reach
scene_run -- confirming id[2] was the sole remaining gate.

## THE MECHANISM (traced, Ghidra + live)
- Credit-wait scene = scene index 1, handler func_0220600e (scene_credit_join_sm). It reads
  `timer = (u16)0x06000654 & 0x7F` and COMMITS (-> attract_handler -> attract reel) when timer==0.
  With 0x06000654 low7 left STATIC 0x7F, timer never reaches 0 -> scene 1 FREEZES (no attract reel).
- Join coordinator: attract_credit_join_service (func_022045f8, run every frame by attract_handler)
  P1 coin-accept edge (pending 0x0609880C==1 && ready 0x0609880E==1) bumps join byte 0x06098674 0->1;
  then the P1 join-confirm edge: if byte==1 AND gate g_0609868E==1 -> player_join_spawn(0) (FAITHFUL);
  if gate!=1 -> byte 1->2 (LOCKED, no spawn). gate==1 only on attract FADE scenes 3/5/7/9 (desc+42).
- player_join_spawn -> func_02227bc4 (spawn P1 id[1]) -> actor init -> func_02229c00
  (player_join_commit_fsm): entity[0xc8] join countdown; on 0 it decrements g_0609978c[idx-1] and at 0
  COMMITS: g_06098688[idx-1]=1 (the sole real setter). This is cart 0x02229c84.
- attract_handler consumes g_06098688 and morphs the executive:
    (g_06099594 + g_06099598) != 0 ? round_controller_join(0x02204afa) : round_controller_dual(0x02205232)
  Only round_controller_dual installs scene_run_handler (ENTER PLAY) on countdown expiry.
  round_controller_join for a single player returns to attract on expiry (needs a cross-player start
  edge to reach dual).

## ATTRACT AUTO-CREDIT (traced, as asked)
The attract reel runs a 2-PLAYER auto fight-demo. On each content/fade scene process_spawn_list spawns
TWO actors via func_02227bc4: id[1]=1 (g_06099594) and id[2]=2 (g_06099598) (watch: pr 0602BBF2).
The P2 join byte 0x06098675 is NEVER written to 1/2 (it is the entity-SLOT, not a byte credit). On the
standard coin route the scripted coin lands on scene 1/2 (gate=0) and func_022045f8 consumes the join
byte 0->1->2 there, LOCKING it before the first fade (scene 3), so the coordinator never fires. That is
why the faithful join needs the countdown (so the reel reaches a fade) + coin sequencing (deliver on a
fade with slot empty).

## PIN — RESOLVED 2026-10-22 (the "coin exits the attract demo" coupling, now faithful)
WAS: at the frame attract_handler consumes the start flag, g_06099598 == 2 (the demo P2 fighter slot,
id[2], stamped by func_02227bc4 and never cleared); the commit FSM clears only id[1] (g_06099594), so
(g_06099594 + g_06099598) != 0 -> morph picks JOIN not DUAL. The forced STV_CLEARSLOT2 poke confirmed
id[2] was the sole gate.

CORRECTION to the old "next step": the referenced fn 0x0602E5D0 (cart 0x0222a5d0) is process_spawn_list
and it SPAWNS actors, it does NOT clear slots. The real slot teardown is entity_system_reset
(func_02227a84, cart 0x02227a84 / rt 0x0602BA84), which every scene bootstrap (func_02232610 state 0)
and the credit-wait scene INIT run to clear ALL 8 slots. That is what clears the slots at each demo-reel
scene transition (observed @VB474/VB970 via STV_SLOTTRACE).

MECHANISM (traced Ghidra + live): on the faithful coin route the demo P1 (id[1], an AI-driven player
actor) is REPURPOSED as the real join actor and reaches player_join_commit_fsm on its own -- it must
survive. So a FULL entity_system_reset is wrong (it nukes id[1] too; a fresh player_join_spawn actor is
not AI-driven and never commits -- verified: commit FSM ran=0). Only the demo P2 (id[2]) must go. The
cart's faithful despawn of ONE player actor is func_02227c5c (cart 0x02227c5c / rt 0x0602BC5C): a
player-actor's terminal FSM (func_0222a418, idx<3 path, unk3c==16) calls it to clear its own slot
(id[]+ptr[]). STV_DEMOSTOP sh2_call's func_02227c5c on the demo P2 entity -> id[2]=0, demo P2 despawns
exactly as on its own death, id[1] untouched -> commit -> (0+0)==0 -> round_controller_DUAL. The only
harness action is CHOOSING to despawn the demo P2 on the coin (the BIOS/director coin->demo-stop coupling
that is genuinely absent in the harness, same class as sub-fix #1/#2). FAITHFUL (real cart teardown
routine), not forced (no raw slot poke).

## RUNTIME EDITS (precise)
File: tools/saturn-recomp/runtime/src/machine/bios.cpp
 1. stv_system_bios_tick() [~line 358]: FAITHFUL SUB-FIX #1 (default, not a knob). Added a per-frame
    decrement of the 0x06000654 low-7-bit insert-coin/attract countdown to 0 (hold at 0), preserving
    high bits (0x100 credit-req, 0x200 2P panel). Gated by stv_sysbios_on() so STV_NOSYSBIOS disables
    it (A/B). This is the real BIOS coin/attract timer: scene 1 (credit-wait) now auto-commits at
    timer==0 and the attract demo reel runs (scene 1 -> 2 -> 3 fade...) instead of freezing.
 2. stv_scene_probe_tick() [~line 445]: FAITHFUL SUB-FIX #2, STV_COINSEQ="vb" (default off). From
    VB>=vb, on the first frame where gate(0x0609868E)==1 AND join byte(0x06098674)==0 AND no coin
    pending, delivers ONE coin via deliver_coin (real coin-detect substitute). Models a player inserting
    a coin during the attract fade; the whole downstream chain is real cart code.
 3. stv_scene_probe_tick() [~line 474]: FAITHFUL SUB-FIX #3, STV_DEMOSTOP="vb" (default off). Once, at
    VB>=vb, when a join is in progress on a gate=1 fade scene (join byte 0x06098674 != 0) AND the demo P2
    slot id[2] (0x06099598) is still live, sh2_call's the game's OWN per-slot despawn func_02227c5c
    (rt 0x0602BC5C) on the demo P2 entity (entity = ptr[2]->unk8; func_02227c5c reads entity->unk34=2 and
    clears id[2]+ptr[2]), with full master context save/restore + re-entrancy guard. This is the faithful
    "a coin exits the attract demo" demo-stop: it despawns the demo P2 exactly as its own terminal FSM
    would, leaving the demo P1 -> join actor intact. Replaces the forced STV_CLEARSLOT2. Uses the
    already-seeded cartrom entry 0602BC5C (NO new seed). Default off => byte-identical.
 4. stv_scene_probe_tick() [~line 540]: STV_SLOTTRACE (diagnostic, default off). Logs the entity-slot
    id[]/ptr[] table, join byte and start flag on change; used to trace the demo spawn/teardown. Read-only.
 5. stv_scene_probe_tick() [~line 490]: STV_CLEARSLOT2 (FORCED diagnostic, default off) -- UNCHANGED but
    now SUPERSEDED by STV_DEMOSTOP (kept as a labeled confirmation knob only; the faithful route no longer
    needs it).
File: port/main_stv.cpp
 6. 0x06000654 init comment [~line 305]: documents that bios.cpp now decrements the low-7-bit countdown
    (initial value still 0x037F; STV_CFG654 overrides initial, STV_NOSYSBIOS disables the layer).
NO seed change this session (func_02227c5c/0602BC5C was already a learned cartrom entry; a transient bad
seed 0602B884 was added then removed -- seeds_extra.txt is net-unchanged).
All diagnostics default-off (byte-identical when unset); sub-fix #1 is the one intended default change.

## REPRODUCE
cd ~/decomp/dh-portB/port
# FULLY FAITHFUL to ENTER PLAY (NO force knob): coin-seq + the real demo-stop despawn ->
#   round_controller_DUAL -> scene_run -> scene81. Expect COV dual=1 join=0 scene_run=1 scene81=1.
STV_BOOTMODE=0 STV_CFG798=40 STV_COINSEQ=100 STV_DEMOSTOP=980 STV_COV=1 ./recomp-build/saturn_stv \
  --cart diehard_main.bin --cartrom ../cart.bin --vblanks 3000 --out run/final \
  --stvinput "99999:COIN" --shots 2000
#   (STV_DEMOSTOP=1 also works -- it fires on the first join-in-progress frame; and add
#    STV_SLOTTRACE=1 to watch id[1]/id[2]: demo spawns 1/2 @VB980, P2 despawned @VB991, commit @VB1605.)
# Faithful commit only, no demo-stop: morphs to round_controller_JOIN, does NOT reach scene_run
STV_BOOTMODE=0 STV_CFG798=40 STV_COINSEQ=100 STV_COV=1 ./recomp-build/saturn_stv \
  --cart diehard_main.bin --cartrom ../cart.bin --vblanks 3000 --out run/x --stvinput "99999:COIN"
# (Superseded) old forced path -- same COV/frame via the raw slot poke:
STV_BOOTMODE=0 STV_CFG798=40 STV_COINSEQ=100 STV_CLEARSLOT2=1 STV_COV=1 ./recomp-build/saturn_stv \
  --cart diehard_main.bin --cartrom ../cart.bin --vblanks 3000 --out run/clearslot --stvinput "99999:COIN"
# Sub-fix #1 alone (no coin): scene 1 auto-advances into the attract reel (no freeze)
STV_BOOTMODE=0 STV_CFG798=40 STV_SCENETRACE=1 ./recomp-build/saturn_stv \
  --cart diehard_main.bin --cartrom ../cart.bin --vblanks 1200 --out run/x --stvinput "99999:COIN"

## NOTE / tradeoff for the user
Sub-fix #1 changes the DEFAULT scene-1 behavior: credit-wait no longer dwells indefinitely for a coin
(the prior "fully faithful round" relied on the coin landing on the frozen scene 1 -> stage spawn-list).
This is the requested faithful behavior (a real cabinet runs the attract reel without a coin), but the
standard coin route now locks the join byte on scene 2; the faithful credit-join requires STV_COINSEQ
(coin timed to a fade). Could optionally make COINSEQ the default for scripted coins if desired.
