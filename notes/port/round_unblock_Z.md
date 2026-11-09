# Round unblock — the real gate (corrects the "credit byte 4" model)

Analysis from the port investigation (coin-driven runs on dh-portA + decomp read).
No source/runtime edits were made; all functions named here are already decompiled.

## Credit byte 4 is NOT a scene-to-round switch
- Nothing writes `g_06098674` to literal 4. It only reaches 4 via the coin-accept-edge
  `g_06098674 += 1` in **func_022045f8** (attract_credit_join_service) when the byte is already 3.
- The byte reaches 3 via **func_02204868** / **func_02204afa** on a START edge. `deliver_coin`
  (rt 0x060065f0) accepts a coin only when the byte is 0 or 3 — so the 3→4 "2nd credit" is a coin
  inserted during the press-START / continue countdown.
- At ==4: func_022045f8 calls **func_0220ab34** (2P resident coord registrar); func_02204afa's
  byte==4 branch morphs back to attract_handler and calls **func_0228168(0)**, whose COMMIT branch
  runs **func_02227bc4** (the real explicit-id player-actor spawn) then resets 4→2. So **byte==4 is
  the 2nd-credit/continue COMMIT that spawns the join actor — not a round scene switch.**

## Scene director round-selection (func_02205b20, called from boot-state 4 of func_022016ac)
Per-frame (gated on director-entity unk3c bit0) it selects the next scene from the current scene's
signed selector `cur[43]` over the table @0x06072660 (stride 0x50):
- `-1`→idx2 · `0`→cur[44] · `1`→cur[44+(g&1)] · `2`→cur[44+(g&3)]  (g = g_0609856c)
- `3`→ (g_06098674==2 && g_06098675==2) ? cur[45] : cur[44]  — credit test, **scene 73 only**
- `4`→ (g_06098670 != 0) ? cur[45] : cur[44]  — 2P-armed test, **scene 1 only (boot-reachable)**
- If `task->unk13 (s8 @+0xd) < 0`: force g_06098668=81 / g_0609866c=0x06073fb0 (reset-timeout "round lever").
- idx≥76: resets join bytes, installs scene_run_handler func_02205812 into *g_06098678. idx==81: game scene built.

**The ROUND is scene 81** (descriptor 0x06073fb0, game SM = func_02206876). Scene 73's credit==2
test only picks a SUB-PATH (both-joined→74 vs →79); both converge 74/79→76-80→81, so it is **NOT a
round-vs-no-round gate**. The character/stage-intro, stage LOAD, and player spawn-script live in
setup **scenes 76-80**.

## Scene 81 game SM (func_02206876)
START gate needs `0x06098798` bits 4/5 (P1/P2 start-enable — BIOS/system-synthesized, never set by
cart code; harness knobs STV_SCENEGATE/STV_STARTENABLE stand in) + panel START (0x060987DB==2).
States 0→4→1→2→3→5; state 4 = scene81_stage_setup (loads stage gfx = RBG1); state 5 COMMITS
(teardown + director hand-off); the player actor is installed DOWNSTREAM of the hand-off.

## The precise remaining gate (two coupled deep chains, not one flag)
1. **Faithful attract→round walk (scene 3→…→73):** each scene must complete its content
   (director unk3c bit0). Scene 3 / the fades complete only when f_2463c8 runs to end-of-script
   (gated on 0x06098564, set by f_22a8d8 only when 0x06098670==0 — STV_ATTRACTARM restores this).
   Forcing the director forward then dies on **unrecompiled-module walls**, first at
   **cart 0x0224627E (rt 0x0604A27E)** @ scene 8 — a RECOMP COVERAGE gap, not a logic bug. ~70
   scenes' content functions are not all in the recompiled module set; each needs a seed + re-recomp
   (the autolearn slog) before scenes 76-80 run the spawn-script that creates the 0x0602Dxxx
   player-join actor whose FSM (func_02229c00, sole setter of start flag g_06098688) commits.
2. **Scene 81 itself:** needs its START gate opened (0x06098798 bits4/5) + the RBG1 stage load.

## Furthest FAITHFUL state proven (one coin, no forcing)
Scene walks 0→2→3, parks ~4100 frames, then the director reset-timeout (unk13<0) force-builds
scene 81 @VB5169; scene81_game_sm + player_join_commit_fsm run ~310 frames; then reboot→scene2.
A cycle. No stepped player actor (direct-to-81 skips 76-80 setup); VDP1 player-sprite path is a
separate gap. Confirms credit_join_trigger.md + scene_director_status.md; does not advance past them.

## Concrete next step toward a real round
Add **cart 0x0224627E (rt 0x0604A27E)** to port/seeds_extra.txt and re-recomp — the first
unrecompiled scene-content wall past scene 3 on the forced-walk path. Expect it to expose the next
wall(s) one rebuild at a time (the multi-scene recomp-seed slog). Alternatively, find the
undecompiled faithful trigger that sets director `task->unk13 < 0` on a real coin+START. Plus RBG1.

## SLOG RESULT (2026-10-21): scene-content coverage is SOLVED in 7 seeds
The ~70-scene estimate was pessimistic - shared content handlers cover most scenes, so 7 recomp
seeds cleared the ENTIRE attract->game-enter reel (0->81) on BOTH scene-graph branches, zero
coverage walls left. Rebuild ~8s each. Walls (rt / cart / scene): 0604A27E/0224627E/8,
060464C4/022604C4/15, 06020368/0221C368/17, 0604074A/0223C74A/32, 0600B8AA/022078AA/34,
0601ED06/0221AD06/67, 06043700/0223F700/69. One coin walks 0..73->79->80->81; STV_SCENEJOIN=600
walks the character branch 73->74->75->76->79->80->81. Scene 81 enters then soft-resets ~27 frames
later (no stage/no START), as predicted. NOT a playable round.

REMAINING (not coverage - no seed fixes these):
1. Scene 81 START gate: 0x06098798 bits 4/5 (BIOS/system-synthesized; harness knobs
   STV_SCENEGATE/STV_STARTENABLE stand in) + the RBG1 stage-graphics load (unimplemented -> black playfield).
2. Player-actor spawn + join FSM (0x0602Dxxx actor; start-flag 0x06098688 commit) downstream of
   scene-81 state 5.
Next concrete targets: (a) open the START gate via harness knob + implement RBG1 stage load, then
(b) confirm the player actor spawns and persists.

## SCENE-81 R&D RESULT (2026-10-21): stage tiles render (forced); root blocker found
Reached a first on-screen frame of REAL stage tiles via a forced diagnostic camera - proof the
renderer + data work. Faithful path still black. Three corrections to the model:
- START gate is NOT the stage blocker: scene-81 SM (func_02206876) reaches state 4 (stage setup)
  on its own with no START. Opening the gate (STV_SCENEGATE) instead FORCES state-5 commit (skips
  setup) -> commits then soft-resets ~22f later.
- Stage uses RBG0, not RBG1 (BGON=0x0018: R0ON=1,R1ON=0). RBG0 already implemented + correct.
  The gap is DATA not code: stage char data IS uploaded (~123KB) but the RBG0 plane map points
  floor rows at EMPTY planes 0x38/0x2E and the rotation camera (My=1640) points off into them -
  never finalized because the round is not live. Forced camera (STV_RBG_YOFF=1248 +RBG_FLATK) frames
  the loaded plane -> real stage tiles render. Evidence: dh-portA/port/run/fr_y1248/shot-5300.png.
- Player CHARACTER actor never spawns; only the coin-join coordinator FSM (0x0602Dxxx: func_02229c00)
  runs. Its start-flag commit (0x06098688 @VB3885) is the credit/continue JOIN, not a round start.

ROOT BLOCKER (unifies all three): the scene-81 intro SM never completes state-5s commit/director
hand-off sustainably. Natural path stalls in intro states 1->2->3 waiting on entity->unk3c bit1
(busy) which never clears (spawned intro objects never signal completion); the director reset-timeout
then kills scene 81 (~288f). Stage-camera/floor finalization AND player spawn both live downstream of
this gate, so render vs actor are NOT yet separable. NEXT: find what clears entity->unk3c bit1 (the
intro-object completion signal) OR the real play-enter path. Once cleared, split into (R) RBG0 stage
camera/floor finalize + (A) player-character spawn/FSM.

Runtime edit added: vdp2.cpp read_rot_param() env-gated STV_RBG_YOFF/XOFF diagnostics (default off;
byte-identical regression when unset). Captured in the runtime patch.

## CORRECTION (2026-10-21, later): unk3c bit1 is NOT the blocker - it clears faithfully
The scene-81 intro SM runs 0->4->1->2->3->5 and commits state 5 with NO forcing. unk3c bit1 is SET
by the recoil-ramp intro objects func_022100f8/func_022101ce (phase 0: entity->unk3c|=2) and CLEARED
by the same handlers on ramp completion (phase 1/2: &=~2); both are recompiled and run, the ramp
completes, the bit clears. The earlier "never clears" note was STALE (those handlers were seeded
after it was written). The ~310-frame "reset-timeout" is just the intro SMs natural runtime (108f
timer + 3 recoil ramps) handing back at state 5. Verified faithfully on both the director-lever and
the 72->80 setup-walk paths; start flag 0x06098688 stays 0, no 0x0602Dxxx actor in the active list.

REAL REMAINING GATE (two, both downstream of the working state-5 handoff):
(A) PLAYER-CHARACTER ACTOR SPAWN - scenes 76-80 / the state-5 handoff must install an EXECUTING
    player spawn-script so a 0x0602Dxxx actor spawns and its FSM raises start flag 0x06098688 (the
    known spawn gap in round_spawn_experiment.md / player_spawn_scene_status.md). START HERE.
(B) Post-81 director target + RBG0 finalize - give the director a real gameplay scene to advance
    into after state 5 (else the boot executive falls back to attract ~310f), and finalize the RBG0
    stage camera (My off-map)/floor planes so the playfield is not black.
Runtime: bios.cpp stv_scene_probe_tick gained a default-off STV_SM81DIAG active-list probe
(byte-identical when unset).

## PLAYER-SPAWN RESULT (2026-10-21): player + start flag commit FAITHFULLY; crux = director decoupling
Faithful (one coin, no edits): a P1 player-family actor spawns (ent 060E1AD0, unk34=1) via the
credit-join path f_22045f8->f_2228168->func_02227bc4, and its FSM func_02229c00 raises 0x06098688=1
@VB3885 (commit state 0x20), consumed by attract_handler one frame later. A SECOND coin during the
continue countdown advances PAST the "Z gate" (credit byte 3->4 @VB3953 sets g_0609868a[0]=1 round-
enter commit; func_02229df4 consumes it + publishes slot g_06099590; f_2205232/f_2204afa run) and
SUSTAINS the 2P continue/join HUD past the single-coin reboot (frame jobF/shot-6200 @VB6200:
portraits + health bars live).
BUT the round does not sustain as GAMEPLAY: the coin spawns the actor on the attract scene (scene-3
park, cause 0x06098670=1), DECOUPLED from the scene director; the director reaches scene 81 only via
reset-timeout, which tears down the coin-spawned actor; scene-81 state 5 (func_02206c40) runs but
spawns no player (lacks the credit-join published context the timeout-jump skipped).
CRUX = the harness decouples the coin credit-join from the scene director. Two faithful sub-problems:
(a) DIRECTOR->81 COUPLING: advance the scene director off the scene-3 attract park onto the credit
    scene (73) on a credit so it walks 73->80->81 faithfully (the 7 recomp seeds already clear the
    walk); missing wire = credit-driven director advance + clearing the 0x06098670 park.
(b) IN-SCENE-81 SPAWN: reach 81 AFTER a coin credit-join published its context (g_06099590/
    g_06099764) + STARTENABLE, and check whether state-5 func_02206c40 finds it + spawns the player.
No runtime edits (all faithful via existing default-off knobs).

## DIRECTOR-COUPLING RESULT (2026-10-21): round gated on BIOS-synthesized signals, not cart code
Deep analysis, no edits. The 90-entry scene reel (cart 0x26E660 stride 0x50) is ONE linear chain
0->2->3->...->72->73->(79 if 1P / 74 if 2P)->...->80->81; reaching 73 is NOT a credit-jump - the
reel gets there on its own IF it does not park. A live CREDIT FREEZES the reel by design (the
gameplay-channel transition-timeline f_2463c8 stalls at cursor row-limit g_060973E0=20; unk3C bit3
never sets; director parks - at scene 3 AND setup scene 79). The ONLY director->81 motion with a
credit is the reset-lever (director task->unk13<0, pulled at credit-resolution VB5165), which jumps
STRAIGHT to 81 skipping setup 76-80, after the credit-join actor is torn down.
The whole decompiled credit-join chain (022045f8->02228168->02229c00->02229df4) spawns the actor +
raises 0x06098688 + publishes context but NEVER writes the scene-director index or the unk13 lever:
the credit-join is architecturally DECOUPLED from the director in every decompiled path. The coupling
signal is BIOS/system-synthesized (like START-enable 0x06098798 bits4/5, which a full image scan
confirms NO cart code sets) and ABSENT from cart code; the harness has no real ST-V BIOS to supply it.
Scene-81 state 5 (func_02206c40) does NOT spawn the player (only sets director-advance bit, blanks
VDP2, kill_objects_above_id(0)). Forced in-scene-81 player (STV_SPAWNPLAYER, ent 060E1620) persisted
through intro but was torn down at the state-5 handoff -> same ~310f reboot, FSM stuck at state 0.
PINNED GAP (3 coupled links, all BIOS/system-synthesized, NO seed/decomp fixes them):
1. Credit->director coupling (real path START-driven: coin->credit->START gated by BIOS start-enable
   0x06098798 bit4/5, absent from cart code).
2. Player must spawn in the scene the director advances INTO after 81 (not at/before 81, where it is
   pruned); scene 81.next44=0 -> attract. Candidate home: scene-81 off16 handler hdl 0600AC88 (cart
   0x02206C88), runs states 0->1->2->3 then stalls at 3 - the plausible gameplay/stage loop.
3. RBG0 stage finalize (camera My off-map / floor planes) for a visible playfield.
A FAITHFUL live round needs the ST-V BIOS start-enable + attract-exit behavior implemented in the
runtime (BIOS emulation), OR a clearly-labeled forced demo (synthesize START + pull the director
lever early + spawn the player in the POST-81 scene, not before it). NEXT: decompile cart 0x02206C88.

## STRUCTURAL CORRECTION (2026-10-21): scene 81 = stage-INTRO; gameplay = stage scenes 2..72
No separate "post-81 gameplay scene". Scene 81 is a FIXED POINT (func_02205b20 early-returns when
g_06098668==81); next44=0 is the real static value: 81 -> scene 0 -> (scene-0 selector -1) -> scene 2
= the FIRST gameplay stage. "Attract fallback" and "first stage" are the SAME scene index (2); only
CONTEXT differs (credit bytes / g_06098670 / human-vs-demo input). Gameplay scenes = STAGE scenes
2..72 (setup 74..78), off14 handlers func_0221b080/02218038/02232610/0221ce1c/0221d2e0/0221eac0/
0221f240 - ALL already decompiled. Scene 0 handler = func_02205c96 scene0_attract_select (shipped).
PLAYER SPAWN installs via scene DATA descriptor +0x1C (3-ptr spawn-list: unk0=1P-even/unk4=1P-odd/
unk8=2P-demo): scene_value_select func_0222a878 picks by g_06098670+parity -> process_spawn_list
func_0222a5d0 -> type-1 entry -> func_02227bc4(0) (slot id 1 = P1) -> player FSM func_0222a300->a418.
ROM-verified: scenes 2/3/4/6 1P spawn-lists begin with a type=1 entry; scenes 79/80/81 have +0x1C=0
(no player there). CREDIT->DIRECTOR: coin-op boot func_022058a8 sets g_06098668=1 -> scene 1
(credit-wait, func_0220600e); scene-1 selector-4 advances to scene 2 IFF g_06098670 armed; all gated
by 0x06098798 bit4/5 (start-enable, BIOS-synthesized, no cart writer).
THE CLEAN TEST (never run before): hold start-enable + keep the director on a real STAGE scene (2/3),
NOT jump to 81, then confirm process_spawn_list reaches a type-1 entry and the human player
(func_0222a300) spawns + persists. Prior waves parked before the content task OR jumped past the
stages. Faithful round path = scene 1 (credit) --[g_06098670 armed + start-enable]--> scene 2 (first
stage) with the player spawning from the stage spawn-list.

## RBG0 RENDER RESULT (2026-10-21): render path PROVEN; black floor = missing gameplay camera anchor
The RBG0 renderer + plane-map + tile upload FINALIZE FAITHFULLY (not a port bug): scene81_stage_setup
(func_02206e2e) runs without early-return; the scene-81 descriptors own default stage-config satisfies
the gate; real cart-ROM stage assets (bundle 0x06072350: pattern 0x22653C88 / palette 0x22653C90 /
cells 0x22653C98) are DMAd to RBG0 page 0 (plane[0..3]=0, map Y 0-511). Lower planes (0x38 @plane4-7,
0x2E @plane8-15) are DELIBERATELY zeroed by clear_vec4_array. The BLACK floor is the DYNAMIC rotation
camera: My drifts 1576->1640 (aimed at the cleared lower region, not page 0) and KAst=31872
(coefficient-table start outside any real table -> garbage Kx/Ky). These per-frame rotation params come
from the live stage-camera driver, which runs WITHOUT a valid anchor because the round never goes live
(no player actor / stage-scroll ref) = the SAME BIOS/director-coupling gap. FORCED camera
(STV_RBG_MY~200 + STV_RBG_FLATK, or STV_RBG_YOFF=1248 + FLATK) renders real stage tile FRAGMENTS
(renderer + plane + tile-data all confirmed). Cheapest forced DEMO floor: find the per-frame
rotation-param writer (stores My/Xst/Yst/KAst into the VDP2 rotation-param table in VRAM) and feed a
fixed valid stage camera (My~256, in-table KAst + perspective ramp). Runtime diagnostics added
(default-off): vdp2.cpp STV_RBG_DIAG/MY/MX/KAST, bios.cpp SM81DIAG stage markers.

## FAITHFUL-BIOS RESULT (2026-10-21): player SPAWNS + PERSISTS (no reboot); gap = coin->director routing
Implemented a faithful ST-V system-BIOS layer in dh-portB (bios.cpp + main_stv.cpp): synthesizes
0x06098798 |= 0x34 (panel-poll + P1/P2 start-enable) every frame post-boot-clear (the BIOS SMPC panel
scan), banks BIOS credits (0x06000664 consumer), serves 0x06000660/64/6C/40, and faithful power-on
cold-boot (0x060987CC=1) + panel-config words (0x06000798=0x60, 0x06000654=0x37F = 2P panel |
credit-required | idle). A/B VERIFIED (STV_NOSYSBIOS toggles it): with coin+START the P1 player actor
(ent 060E1AD0 unk34=1) spawns via func_02227bc4->func_0222a300/a418 and PERSISTS VB1500->6500+ with
NO reset-timeout teardown, NO reboot (old warm path jumped to scene 81 @VB4902 -> reboot). This passes
the coordinators clean test (human player spawns + persists at a stage scene). NOT yet a rendered live
round: frame = combat HUD + "CONTINUE? 4" over the attract skyline, player alive in the object list.
REFINED GAP = coin->director ROUTING: the faithful path is scene 1 (credit-wait func_0220600e) --
[g_06098670 armed + start-enable]--> scene 2 (stage). But (1) boot controller func_022016ac starts the
director on scene 2 attract-demo, never holding on scene-1 credit-wait; (2) on a coin the fade
sequencer func_0201c9c resets index to 0 -> attract demo loop -> FREEZES at first fade (scene 3)
because 0x06098670=1 stalls the transition-timeline (func_2463c8 gated on 0x06098564, set only when
0x06098670==0) - the same flag that arms the human spawn-list freezes fades; (3) no cart code writes
the director index to route a coin to scene 1 / a live stage = the BIOS/system coupling, genuinely
absent. NEXT: make the director SIT on credit-wait scene 1 when a credit is present (coin->director
routing), reconciling the 0x06098670 arm-vs-fade-freeze conflict. Build note: dh-portB CMakeLists
hardcodes SATURN_RUNTIME to dh-portA; reconfigure with -DSATURN_RUNTIME=<dh-portB runtime> (done in
dh-portB build cache). Round-runtime (dh-portB) captured at port/runtime-patches/round-runtime-dh-portB.patch.

## ROUND REACHED (2026-10-21): coin+START -> live sustained combat STAGE with the player
The coin->director routing is CLOSED. Faithful mechanism (traced end to end): scene_bootstrap
func_022058a8 is the credit->director router - with test-mode clear + credit-required set it writes
g_06098668=1 (scene 1 credit-wait, func_0220600e); scene-1 selector-4 advances to scene 2 (stage,
func_0221f080) when g_06098670 armed; scene 3 = combat STAGE (func_02236610). The stage spawn-list
(+0x1C) via func_0222a878/a8d8 picks the human 1P list (join byte 0x06098674[i]==2) whose type-1
entry calls func_02227bc4(0) -> P1 player (ent 060E1AD0 unk34=1, handler func_022863e). The
arm-vs-fade-freeze conflict is a NON-issue on this path: credit-wait commits the director advance
directly (its own state 3), not through the attract fade timeline that g_06098670 freezes.
TWO FAITHFUL BIOS-WORD BUGS FIXED (main_stv.cpp): (A) 0x06000654 was st32 but both readers read it
16-bit -> big-endian high halfword=0 -> credit-required(0x100)/2P(0x200) bits invisible -> FIX st16
0x037F; (B) 0x06000798 bit5 = TEST/SERVICE mode (not "proceed") - normal coin-op needs it CLEAR;
credit route uses STV_CFG798=40.
RESULT (STV_BOOTMODE=0 STV_CFG798=40 STV_FORCEARM=2): scene 0->1(VB9)->2(VB11)->3 combat STAGE
(VB846), SUSTAINED through VB2600 no reboot. P1 player (hdl 0602C63E func_022863e, ent 060E1AD0
unk34=1) present on scene 3 VB1200+ from the REAL stage spawn-list. Frames: run/FINAL/
scene3_HUD_player_blackfloor.png (combat HUD + portraits/health over rendered skyline; floor black),
scene3_forcedcam_stagetiles.png (real RBG0 stage tiles render w/ forced camera).
FAITHFUL: config fixes, scene_bootstrap->1 routing, 1->2->3 walk, stage spawn-list, P1 spawn+persist,
combat HUD, RBG0 render path. FORCED (labeled): STV_FORCEARM (asserts g_06098670=1 + g_06098674[0]=1
= the arm+join the real credit-wait START commit would set) + STV_CFG798=40 + forced RBG0 camera.
REMAINING (2 clean items): (1) recomp wall cart 0x0220686A (rt 0x0600A86A) - the aux object scene-1
handler func_0220600e spawns; the ONLY wall on the faithful scene-1 path. Seed it + re-recomp ->
credit-wait runs its real START commit -> STV_FORCEARM no longer needed (fully faithful). Added to
seeds_extra.txt. (2) RBG0 floor black: stage-camera My off-map needs a valid anchor fed from the live
player/stage-scroll ref (now reached live on scene 3 with the player present; camera-finalize and
player-spawn are no longer coupled).

## FULLY FAITHFUL ROUND (2026-10-21): coin+START, NO force knobs
Env STV_BOOTMODE=0 STV_CFG798=40 + a real coin+START1: scene 0 -> 1 (credit-wait DWELLS, VB9) -> 2
(VB216) -> 3 (combat stage, VB1051), sustained to VB4000, no reboot/FATAL. STV_FORCEARM is GONE -
func_0220600e sets the 2P-arm g_06098670=1 itself (store at cart 0x0220618E); the REAL coin drives
the credit-join to set join byte 0x06098674 0->1->2 (VB215); with NO coin the director holds on scene
1 (coin genuinely required). P1 player (func_022863e, ent 060E1AA0) spawns from the real stage
spawn-list + persists. ONLY seed needed: 0600A86A (cart 0x0220686A), 1 re-recomp, no further walls.
port/relearn_portB.sh = dh-portB recomp wiring. Non-default config = STV_CFG798=40 (correct coin-op
DIP, test-mode bit clear) + the 2 faithful BIOS-word bug fixes. Frame:
run/EVIDENCE/faithful_scene3_coin+START_noforce.png.
PART 2 FLOOR (remaining, re-scoped - NOT just a camera anchor): on the live faithful scene 3 the
stage-camera driver STILL writes My~1990 off-map + KAst garbage every frame, AND page 0 floor
char-data is largely EMPTY - the stage-floor upload (a scene81_stage_setup-equivalent) does NOT run
on the credit-route scene 3. Faithful floor needs BOTH on gameplay scene 3: (a) run the stage-floor
char-data upload, (b) a valid stage camera (My->page0, in-table KAst). Forced camera (STV_RBG_MY=256
STV_RBG_KAST=0 STV_RBG_FLATK) shows the sparse real tiles = renderer proven. Separate: the VDP1
player-sprite gap (actor in object list, sprite not drawn). Rotation-param writer = SH-2 stage-camera
driver DMAs the VDP2 rotation-param table to VRAM 0x5F000 (RPTAU/RPTAL 0xBC/0xBE), assembled
dynamically. Runtime diagnostics added to vdp2.cpp (STV_RBG_DIAG/MY/MX/KAST, STV_LAYERDIAG),
default-off, byte-identical when unset.

## FLOOR RESULT (2026-10-21): partial upload + stale camera; faithful floor = stage-anchor init gap
On the fully-faithful scene 3 (RBG0 on; off14 handler func_02232610 ran): the floor upload is PARTIAL
not absent - plane 0x38 (VRAM 0x070000) got ~65% of the real street-floor texture; plane 0x2E
(0x05C000, far rows) got NOTHING; page 0 is non-floor data. The off14 handler streams only around the
current stage-scroll window, and that scroll/anchor ref is UNINITIALIZED on the credit-route scene 3
(setup scenes 74-78 feed the scene-81 intro branch, not scene 3 - scene 3 loads its own floor via
func_02232610). Camera: My=1991 (off-map, points at the EMPTY far planes), KAst=31872 (off-table ->
garbage coeff); STATIC across 1400 frames (does NOT track the player) = stale/default anchor. Root =
the stage-scroll/player-position reference is uninitialized (scene 3 reached in a reduced/attract-demo
context = the system/BIOS stage-coupling gap). FORCED floor (STV_RBG_PLANE=070000 + STV_RBG_MY=256 +
STV_RBG_FLATK) renders a coherent tiled street floor under the combat HUD (white lines = the 35%
unfilled cells) = renderer + floor tiles PROVEN. Faithful floor needs (1) char upload to plane 0x2E,
(2) a valid rotation coeff table, (3) a live camera anchor - all driven by the uninitialized
stage-scroll/player ref. Going further needs naming the CPU writer of VRAM 0x5F048 (My) + its
scroll-ref input = requires the Ghidra per-function decomp (port/recomp is raw-asm C, not
dataflow-traceable) - the same wall. New default-off knob STV_RBG_PLANE added to vdp2.cpp.

## UNIFIED COUPLING POINT (2026-10-21, Ghidra): floor + player-sprite = ONE missing wire
Ghidra dataflow (guest = Ghidra_addr + 0x03E04000): BOTH remaining visual gaps share one root - the
camera/sprite FOCUS reference *0x0609882c (= object 0x060922A4, sprite_transform_setup DAT_0222268c),
read by BOTH the RBG0 rotation camera AND the P1 player-sprite transform. On credit-route scene 3 it is
a STATIC boot-time object (set once at VB8 by fn 0x0220B0A4, position ~unchanged), NOT wired to the
live spawned player actor 060E1AA0 (dynamic pool).
- FLOOR: RBG0 My reads this objects position -> 1972.8 off-map (page 0 = 512px) -> black. Rot-param
  table @VRAM 0x5F000 is an SGL work buffer (edit 0x060B89D0, My @ edit+0x48 = 0x060B8A18) copied
  per-frame (FUN_0221660c) -> set-A shadow -> DMA -> VRAM; written from the focus object by
  sprite_transform_setup (Ghidra 0x022225f6) via frame_sync_loop (FUN_022013c4) draw calls.
- SPRITE: the P1 sprite IS built + emitted every frame (gate *0x060bacb0 bit0 OPEN on scene 3; VDP1
  draws 130 textured sprites, cells uploaded) but transformed from the SAME stale focus object -> it is
  MIS-POSITIONED off the visible play area, not missing.
BOTH gaps = the live-gameplay->focus wiring (player position fed into 0x060922A4 / 0x0609882c pointed
at 060E1AA0), which the cart performs only under the real BIOS/director live-stage path. A runtime shim
driving 0x060922A4 from the live player actor would render BOTH the floor (camera anchor) AND the
player sprite = a labeled-forced but visually-complete round. Read-only Ghidra helper scripts added in
scripts/ (RotRecon/ScanConst/DecompList/Probe/DisasmFn).

## FOCUS-WIRE RESULT (2026-10-21): mechanism CONFIRMED but necessary-INSUFFICIENT; real gap = live-combat activation
STV_FOCUSTRACK (bios.cpp, vblank-in) copies the live P1 player (ent 0x060E1AA0, via slot table
*(0x06099584+0x2C+4)->0x06092524->unk8) world X/Y into the focus entity (*0x0609882c=0x060922A4 ->
fent *(P+8)=0x060E13B0 +8/+0xC). Z(+0x10) + angles(+0x24/26) deliberately NOT copied (player Z triggers
depth-cull -> all sprites suppressed; player angles are FACING not camera -> collapse RBG0 matrix).
CONFIRMED the shared-source mechanism: the wire moves BOTH consumers - floor camera My 1998->1908,
sprite view matrix +0x44 X 88->96 / +0x60 Y +408->-208. BUT INSUFFICIENT:
- FLOOR still black: My only shifts ~90, dominated by the STALE attract-context camera object
  0x060755D4 (cam.z=0x788000=7.9M vs player.z=98305); the wire fixes src not the stale cam. STV_CAMTRACK
  does not help. Plus partial floor upload (plane 0x38 ~65%, 0x2E empty).
- PLAYER SPRITE: VDP1 emits 179 sprites but they collapse to a point / dont render in BOTH baseline and
  shimmed - characters are PARKED because scene 3 on the credit route is a reduced ATTRACT/COIN-WAIT
  context ("INSERT COIN(S)" on screen), NOT live combat.
REFRAME: reaching the scene-3 STAGE != activating LIVE COMBAT. Both remaining visual gaps (floor camera
init + un-park character sprites + full floor upload) funnel to ONE deep gap: the BIOS/director
LIVE-STAGE ACTIVATION handshake, genuinely absent on the credit route = a substantial HLE/RE effort,
not a shim. Every PIECE is proven working (renderer, sprite emission, player spawn+persist, camera
mechanism); they await the live-stage context. Runtime: STV_FOCUSTRACK + STV_VDP1DIAG = keepers;
STV_CAMTRACK/CAMZ/CAMY = negative-result experiment knobs (drop in a lean pass). All default-off,
byte-identical when unset.

## LIVE-COMBAT ACTIVATION RESULT (2026-10-21, Ghidra+live): executive morph PINNED; fires forced, clears coin-wait
Deep Ghidra read + live COV/peek on the fully-faithful credit route (STV_BOOTMODE=0 STV_CFG798=40,
real coin+START1). NO runtime source edits this session (used existing default-off knobs only:
STV_STARTFLAG/STV_FOCUSTRACK/STV_COV/STV_SCENETRACE). Added read-only Ghidra helper scripts/RawFn.java
(creates a fn in-memory in -readOnly to decompile undefined-boundary addrs; nothing saved).

THE ACTIVATION HANDSHAKE (what the real BIOS/director does), traced end to end:
The "attract vs live-combat" distinction is NOT the director scene index - it is the EXECUTIVE task
handler in *g_06098678 (object 0x06092264). Scene index picks WHICH stage; the executive picks the
MODE. The morph chain = attract_handler(0x02204868) -> round_controller_dual(0x02205232) ->
scene_run_handler(0x02205812 == ENTER PLAY):
 1. attract_handler every frame: credits-absent branch draws the INSERT-COIN countdown; credits-present
    (else) branch CONSUMES start flag g_06098688 (pool DAT_02204a08) -> start bring-up (arm continue
    timer task+0x40 = ~0x27f, join byte ->3, notify slot g_06000688) -> morphs executive to round_controller.
 2. round_controller_dual counts the continue timer (task+0x40 P1 / +0x44 P2) down; on EXPIRY installs
    scene_run_handler. BOTH expiry-install sites resolve to 0x02205812 (verified via pool:
    DAT_02205738=0x17a +0x2205698 = 0x02205812; DAT_022054f8=0x33a +0x22054d8 = 0x02205812). Tail gate
    LAB_02205748 needs join byte 0x06098674==2 or 0x06098675==2 (pool DAT_02205850=0x06098674,
    DAT_02205854=0x06098675); its non-expiry installs resolve to attract(0x02204868)/join(0x02204afa).
 3. scene_run_handler (0x02205812, a jumptable dispatch) runs the live scene / advances the
    director into the scene-81 game bring-up.

LIVE GROUND TRUTH (STV_COV, faithful route, no force): executive STAYS attract_handler forever -
round_controller_join/dual=0, scene_run_handler=0, scene81_game_sm=0, player_join_commit_fsm=0.
Start flag g_06098688 is NEVER set (watch: only the VB0 init-to-0; coin@VB200 + START@VB340 register
but never set it). Yet process_spawn_list=1 and the P1 player spawns+persists (join byte 0x06098674[0]=2,
g_06098670=1). So on the credit route the player is live but the EXECUTIVE never goes to play mode -
INSERT-COIN drawn + characters parked.

WHY FAITHFULLY ABSENT: g_06098688 sole setter = join-coordinator FSM at cart 0x02229cac (inside
func_02229c00), which runs only once the credit-join COORDINATOR actor is spawned (player_join_spawn
0x02228168), driven by the BIOS/SMPC START-enable handshake (0x06098798 bits4/5) that prior full-image
scans + this session COV confirm NO cart code produces. Genuinely BIOS-synthesized, consistent with
every prior round.

FORCED DEMONSTRATION (labeled; existing STV_STARTFLAG knob, no new code): forcing g_06098688 once on
the faithful route drives the REAL cart chain end to end:
 - executive morphs attract_handler -> round_controller_dual (COV: round_controller_dual=1).
 - INSERT-COIN is CLEARED and replaced by the game own round-enter countdown "CONTINUE? 2/1/0"
   (frames run/act/shot-1200/1400/1600.png).
 - at CONTINUE?0 the continue timer expires -> scene_run_handler=1 AND scene81_game_sm=1 (ENTER PLAY),
   director advances scene 3 -> scene 81 @VB1626 -> soft-reset (the known post-81 fallback gap).
So the activation handshake + coin-wait clear WORK; it routes through the scene-81 bring-up (not
sustained scene-3 combat) and soft-resets.

CHARACTERS STILL PARKED even under the morphed executive: during VB1051-1622 the executive was
round_controller_dual ON scene 3, yet act-1400 shows only skyline+HUD+"CONTINUE?1" - no fighters on a
floor. So the executive morph alone does NOT un-park actors. The per-actor live gate is SEPARATE:
actor_main_state_step (0x02229074) runs the actor main state + animation_update "gated by unk4C" (two-
level switch on unk88, keyed by HP unk56). The scene-3 reduced context never sets the actors live gate
(unk4C / the focus-wire / stage-scroll anchor), so VDP1 emits their sprites with collapsed geometry.

PINNED REMAINING GAPS (all downstream of the same absent BIOS live-stage handshake):
 (1) FAITHFUL executive morph: set g_06098688 from the real credit-join coordinator (needs the BIOS
     START-enable 0x06098798 bits4/5 -> join actor spawn -> func_02229c00 state reaching cart 0x2229cac).
 (2) SUSTAIN on a stage: ENTER PLAY routes to scene 81 (bring-up) which soft-resets (post-81 director
     target + RBG0 stage finalize - the long-standing gap).
 (3) UN-PARK actors: set the per-actor live gate unk4C (actor_main_state_step 0x02229074) + the live
     focus/stage-scroll anchor so character transforms produce real geometry.
Furthest state captured: run/act/shot-1600.png (forced ENTER-PLAY countdown "CONTINUE?0", coin-wait
cleared). Reproduce: STV_BOOTMODE=0 STV_CFG798=40 STV_STARTFLAG=1 STV_COV=1 STV_SCENETRACE=1
  ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 3000 --out run/act
  --stvinput "200:COIN,340:START1" --shots 1200,1400,1600

## JOIN-COORDINATOR SPAWN (2026-10-22): faithful trigger traced; gap = spawn CONTEXT/ORDERING
player_join_spawn (0x02228168) is called by func_022045f8 ONLY when join byte 0x06098674==1 AND
per-scene gate g_0609868E==1. g_0609868E = scene_descriptor[+42] (table @cart 0x26E660): +42==1 ONLY on
attract FADE scenes 3/5/7/9; scene 1 (credit-wait) + scene 2 (demo stage) are +42==0. The SMPC/BIOS
bits (0x06098798 bit4/5 + panel START 0x060987DB==2) are read by scene-1 credit_render (func_0220600e)
for the credit commit, NOT the spawn gate directly. WHY IT NEVER FIRES (proven): the byte is consumed
0->1->2 on a gate=0 scene (credit-wait/demo/attract auto-credit) BEFORE any fade scene; no frame has
byte==1 AND gate==1. Forced onto a fade scene (STV_JOINTEST) the coordinator COLLIDES with the already-
spawned stage player (ent 060E1AA0): re-stamps it but unk43=0x0C (normal state, not join 0x02) ->
func_022863e runs normal logic, never the join FSM -> g_06098688 never commits. NOT a trigger gap (fully
traced), NOT coverage (join chain all recompiled). ROOT: the coordinator is designed to run in the
ATTRACT/credit phase (empty P1 slot + join spawn-list + fade gate) BEFORE the stage player; the direct
credit route spawns the stage player first. FAITHFUL FIX = (a) run the real attract demo reel during
credit-wait (fix BIOS countdown 0x06000654 & 0x7F synthesized static 0x7F -> scene 1 FREEZES; with a
countdown STV_CFG654=0300 the reel runs, but attract auto-credit still locks byte=2 on scene 2 before
the first fade) + route a coin to a fade scene with slot empty, OR (b) HLE the full BIOS/director
credit-join->executive->stage SEQUENCING. Substantial RE/HLE, not a shim. Runtime (default-off):
STV_SCENETRACE extended (gate/cpend/cready) keeper; STV_JOINTEST diagnostic. STV_STARTFLAG remains the
only lever for the faithful downstream morph.

## FAITHFUL START-FLAG COMMIT (2026-10-22): g_06098688 set by REAL cart code; ENTER PLAY reached
HEADLINE: g_06098688 is now set FAITHFULLY (NO STV_STARTFLAG) by func_02229c00 (commit @cart
0x02229c86, COV player_join_commit_fsm ran=1) @VB1604, consumed by attract_handler. Two faithful
sub-fixes (full detail: notes/port/faithful_credit_join_sequencing.md):
1. FAITHFUL COUNTDOWN (now DEFAULT): bios.cpp stv_system_bios_tick decrements the 0x06000654 low-7
   insert-coin/attract timer each frame (real BIOS); credit-wait scene 1 auto-commits at 0 + the
   attract reel runs (no freeze). No-coin reel: 0->1->2->3 fade (VB978, gate=1) slot empty.
2. COIN SEQUENCING (STV_COINSEQ, default off): delivers a real coin on the first gate==1 (fade) frame
   with byte==0 + no pending -> the game own func_022045f8 runs coin-accept (byte 0->1) then
   join-confirm (gate==1) -> player_join_spawn -> func_02229c00 commits. All downstream real code.
With these: executive morphs attract_handler -> round_controller (INSERT-COIN cleared, CONTINUE?
countdown, live HUD - run/final/shot-2000.png). PIN (sole remaining faithful gap to dual->scene_run->
scene81 ENTER PLAY): attract_handler picks dual vs join via (g_06099594+g_06099598)==0; at commit
g_06099594=0 but g_06099598=2 (the attract 2P DEMO fighter slot, never cleared) -> picks 1P-join.
Forcing g_06099598=0 (STV_CLEARSLOT2) routes dual->scene_run->scene81 (all real cart code). FAITHFUL
fix = "coin exits the attract demo": tear down both demo fighters (process_spawn_list teardown fn
0x0602E5D0 clears both slots at scene transitions) before func_02229c00 commits, so id[2] is clear.
Scene 81 then still soft-resets (separate post-81 gap). Tradeoff: sub-fix #1 changes default scene-1
(no longer dwells for a coin) - the faithful credit-join now needs the coin timed to a fade (STV_COINSEQ
or make it default for scripted coins). Runtime: bios.cpp countdown (faithful default) + STV_COINSEQ +
STV_CLEARSLOT2 (default-off).
