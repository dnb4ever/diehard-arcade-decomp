# Post-scene81 sustain: the unified wall (2026-10-22)

## Result of the sustain investigation
Live gameplay NOT reached (honest). But both previously-separate "floors" (no live
player actor; RBG0 stage floor off-map/black) are now UNIFIED to ONE root cause.

## The mechanism (decomp-grounded)
- scene 81 is the ENTER-PLAY **intro**, not the round. scene81_game_sm (func_02206876)
  runs states 0->4->1->2->3->5; **state 5 DELIBERATELY blanks** the display
  (`*0x25F80000 = DISP.0 & 0x7FFF`), kills objects (kill_objects_above_id(0)), and
  sets the director-advance bit -- a hand-off.
- The scene director (func_02205b20) `g_06098668==81` branch is TERMINAL: it re-affirms
  81 and never selects a successor round scene.
- The boot SM (func_022016ac) state 4, after the director acks, parks at the undefined
  `task->state = 13` when DIP 0x06000798 bit5 is CLEAR -- which the credit route
  (STV_CFG798=40) makes it. (Confirmed polarity: 022016ac.c `if(!((0x798>>5)&1)) state=13`.
  NIT: the rationale comment in dh-portB/port/main_stv.cpp still says "state 13 when SET" --
  backwards; the decomp file is the correct reference.)

## The single faithful gap (both floors)
The **character-select / stage-setup scene chain, scenes 73-80**, is what BOTH:
  (1) builds the gameplay-context persistent player actor (FLOOR 1), AND
  (2) populates the stage-asset bundle (ctx->unk30 bit0 + ctx->unk3C: pattern table,
      palette, cell records) that scene81_stage_setup (func_02206e2e) DMAs to VDP2.
      That handler early-returns at `if(!(ctx->unk30 & 1)) return;` on the credit route.
The faithful credit route SKIPS 73-80 (attract -> round_controller_dual -> scene_run -> 81),
so neither the actor nor the stage floor is ever built -> scene 81 blanks over empty VRAM.
Scene table chain: 73(credit sel=3)->74->75(char-select sel=2)->76/77/78->79->80->81.
Setup handlers off14 = 06023240 / 060232BC / 06023532.

Force-seeding the director into 73-80 (STV_SCENESEED/STV_SCENEADV) makes them complete
INSTANTLY without populating the bundle -- their handlers no-op without the char-select/
stage-selection game-state + transition-timeline (0x06098564 gate) completion. That
sequencing is the absent BIOS live-stage-activation handshake every prior session converged on.

## Status / evidence
- Faithful ENTER-PLAY (scene 81) reached with ZERO force knobs (STV_COINSEQ=100 STV_DEMOSTOP=980):
  COV player_join_commit_fsm ran=1; frame = "CONTINUE?" + portraits + health bars over the
  night skyline, then blanks by design. PNG: run/TASK_EVIDENCE/FAITHFUL_enterplay_*.png.
- Sustained VISIBLE (non-blanking) frame achieved only on the scene-3 coin+START route
  (attract/coin-wait context, NOT a live round): HUD+skyline, md5-differs 1200+ frames.
- New runtime diagnostic (default-off, byte-identical when unset): **STV_HOLDDISP=<scene>**
  in vdp2.cpp vdp2_compose() -- keeps composing past scene81's TVMD blank to reveal the
  (empty/garbled) stage VRAM behind it, proving the blank is display-disable over an
  unpopulated stage, not a renderer failure.

## Next sub-project (bounded but substantial)
Faithfully satisfy scenes 73->80's completion gates: char/stage selection input into
scenes 73/75 (likely STV_PANELBTN-class) + the transition-timeline interpreter + the
0x06098564 gate, so scene81_stage_setup and the real player-spawn run in a populated
context. That is the single remaining wall to a live, sustained round.

## UPDATE 2026-10-22b: stage-bundle mechanism PROVEN; two walls pinned

Drove the scenes 73-80 chain first-hand. Findings:
- Per-gate map (complete = handler sets director-entity unk3C bit0 -> func_02205b20 advances):
  sc73/74 -> shared SM func_02218038 (gate: g_06098664==1 OR timer+cooked-button+g_06098670==1);
  sc75 char-select func_02232bc4 (loops state2<->4<->5 until BOTH char-select actors CONFIRM
  characters: entity unk56 set + func_0222a04e pass); sc76 -> func_02218038; sc79
  func_0221f2bc (state2->3 on g_06098664==1 or START shortcut); sc80 func_0221f532 (raw-asm) -> 81.
- PROVEN by peek: on the faithful credit route scene81 task node (0x06092524) has unk30=0 / unk3C=0,
  so scene81_stage_setup (func_02206e2e) early-returns -> NO stage DMA. The scene-81 DESCRIPTOR
  (g_0609866c=0x06073FB0) holds the real stage config (desc.3C=0x06072350 bundle) but nothing copies
  it into the node on the credit route -- that copy is the scene-75-80 stage-select side-effect.
- NEW default-off knob STV_STAGECFG (bios.cpp, byte-identical unset): copies desc stage-config
  (+0x30/34/38/3A/3C) into the scene81 node until unk30 bit0 set -> scene81_stage_setup RUNS and
  DMAs the REAL cart-ROM stage bundle. PROVEN: BGON 0x0119->0x0018, HOLDDISP frame goes all-black
  -> real stage-tile palette (greens/olive/browns). PNG run/TASK_EVIDENCE_stage_tiles_sc81.png
  (streaks under the off-map camera -- bundle DMA proven, NOT a coherent floor).
- Also added read-only SCENE81_DESC / SCENE81_CTX peeks (under STV_SM81DIAG) in bios.cpp.

### The two remaining walls (both pre-existing BIOS/input gaps; not closed this pass)
A) SCENE 75 char-select completion needs the full P1/P2 COOKED-INPUT DECODER (direction/button bits
   at 0x06098734 / 0x0609874c) so the two char-select actors confirm characters. Only a generic
   "pressed" bit0 (0x0609879A) is currently verified; START/P6/P7/PANELBTN do not satisfy it.
   This is a bounded RE task (decode the input-word format) -- the gating wall for the faithful path.
B) Even with the stage bundle, the visible floor needs the RBG0 live stage-camera anchor (My off-map
   / KAst off-table), AND scene 81 is an INTRO that commits in state 5 (blanks + kill_objects) and
   hands off to a director with NO successor (81 is terminal). The live round lives DOWNSTREAM of a
   hand-off the absent BIOS live-stage-activation coupling never completes. This is the fundamental
   HLE-BIOS gap every session converges on.

### ARCHITECTURAL FORK (for the owner)
The port HLE-STUBS the ST-V BIOS (the harness deliberately bypasses BIOS boot to dodge the security
hang). Walls A and B are both "absent BIOS coupling" -- things the REAL ST-V BIOS would drive.
Option 1: keep HLE-RE'ing each coupling (input decoder, live-stage activation, post-81 successor) --
  faithful but each is a substantial sub-project.
Option 2: run the REAL ST-V BIOS ROM (the owner has the BIOS romset) inside the recomp runtime instead of the
  HLE stubs -- lets the real BIOS handle the coupling natively. Cost: integrating real-BIOS boot into
  a runtime currently built to skip it (security CRC, full boot sequence). Potentially the shorter path
  to a live round, since it stops re-implementing the BIOS by hand.
This is a strategy decision, not a quick crack.

## UPDATE 2026-10-22c: "Wall A" DISSOLVED - input solved; scene 75 was a misread

CORRECTION to the Wall-A framing above: scene 75 (func_02232bc4) is NOT a char-select
confirm gate. It is a 2-FIGHTER ROUND INTRO that self-advances via func_0222a04e
(HP/signature compare of the two fighters g_060995b0[1]/[2], unk56) + spawn-progress/
timeout (unk5A, entity unk54) - no cooked input required to complete. Proven: seeded into
the chain, the director runs 74->75@VB921->76@VB934->79@VB1197 with ZERO force-advance.

### Cooked-input path FULLY MAPPED (input is no longer a blocker)
- Cook = func_0220f9d4 (0220f9d4.c), per-frame controller SM; reset = func_0220f990.
- Records: 5 x 0x18 at base 0x06098734 (P1 0x06098734, P2 0x0609874c, ... sys rec4 0x06098794).
  Fields: +0 cur, +2 inject, +4 press-edge, +6 release-edge, +0xC/+0xE dir timers, +0x10/+0x12
  button hold, +0x16 latched dir. Confirmed by func_0222c220 (PAD_RECS 0x06098734) + func_0220f990.
- BITS (raw == cooked, cook ORs raw straight in, no remap): bit0=A, bit1=B, bit2=C,
  bits6-9 (0x3C0)=direction. Consumers: func_0220e1e2 (mover 0x40/80/100/200), func_0222c220.
- Channel/present mask 0x06098564 written by func_0222a8d8.
- RAW side = BIOS: cook reads 0x06000708 (P1) / 0x06000710 (P2), filled by the ST-V SMPC
  peripheral handler on HW, ZERO in the HLE harness. New default-off knob STV_RAWINPUT
  ("VB:P:HEX,...") writes the raw words so the cart's own cook decodes them - PROVEN: inject
  P1 0x0141 / P2 0x0282 -> cooked 0x06098734=0x0141, 0x0609874c=0x0282. (Oracle not needed:
  cart-side cook determines the layout.)

### Recomp fix
Added seed 0602E0EC (func_0222a0ec, scene-75 state-6 cleanup) to seeds_extra.txt - was the
only wall breaking faithful 74->75 entry (FATAL "call to 0602E0EC, not an entry"). Regenerated.

### THE narrowed remaining wall (single, downstream - NOT input)
1) The fully-faithful credit route SKIPS 73-80: attract_handler -> round_controller_dual ->
   scene_run -> scene 81, never entering the setup chain. The attract->setup-scene-chain routing
   is the real HLE-BIOS live-stage-activation coupling (the chain only runs when director-seeded).
2) scene 79 (func_0221f2bc) parks on g_06098664==1 or a START shortcut; then 80->81.
Next step: use the runnable recompiled-BIOS oracle (~/decomp/bios_spike) to extract how a real
coin+START routes attract -> the 73-80 setup chain, and port that routing into HLE so the faithful
route enters the chain (-> populated stage + real fighters -> scene 81) instead of shortcutting.

## UPDATE 2026-10-22d: ROUTING MECHANISM PROVEN; faithful walk 73->81 reached; stage bundle populated

### (1) The 81-shortcut vs 73-chain routing decision (decomp-cited, empirically confirmed)
Two SEPARATE tasks are in play and the "routing" is the SCENE DIRECTOR, not the executive morph:
- DIRECTOR task func_022016ac state 4 -> func_02205b20 (scene_director_build) walks the scene table
  (g_06098668 index, g_0609866c desc) one scene per entity->unk3c bit0 ack.
- GAME/executive task (*g_06098678): attract_handler (func_02204868) -> on a start flag, morphs to
  round_controller_dual (func_02205232)/join (func_02204afa) -> scene_run_handler (func_02205812).
  scene_run just steps g_0609867c = whatever scene the DIRECTOR currently has installed.
THE DECISION: the director PARKS on scene 3 (the attract fade) because arm g_06098670=1. Cited:
func_0222a8d8 sets channel mask 0x06098564=0 unless g_06098670==0; func_022463c8 (transition
timeline) early-returns when 0x06098564==0 -> scene-3 fade never completes -> entity unk3c bit0 never
set -> director cannot advance. Parked, the director's reset-timeout lever fires: func_02205b20
`if ((s8)task->unk13 < 0) { g_06098668=81; g_0609866c=0x06073fb0; }` -> JUMP STRAIGHT TO 81, skipping
73-80. arm=1 is set by scene-1 credit-wait func_0220600e state-0 (`R32A(0x06098670)=1`). EMPIRICAL
(STV_SCENETRACE): faithful credit route sits scene=3 VB966..VB2875 then scene=81 @VB2877 the frame
after arm clears; NO coin needed to reproduce (pure attract parks 3 VB966..5021 then 81). So the
executive attract->dual->scene_run morph is NOT the router; it is PLAY-ENTER running on top of the
director's scene (81, from the timeout). The 73-chain is only reached by WALKING the reel (every scene
completing) or by SEEDING the director index. There is NO cart "jump to 73": 73 is the end of the
linear reel; the credit-select test is func_02205b20 case-3 (scene 73 only): both join bytes
g_06098674/75==2 ? cur[45](->74 2P setup) : cur[44](->79 1P).

### (2) Oracle (bios_spike) - cannot show attract->73 routing
Re-ran. The recompiled real BIOS still halts in LOW-LEVEL boot (WDT/cache-address-array init, last
log line "area W cache address array"), i.e. BEFORE the RAM-relocation blocker f_00003576 documented
in real_bios_spike.md, and FAR before any cart attract/director logic. The attract->73 routing is CART
code (the scene director) that runs only after full BIOS boot + cart handoff - downstream of the
oracle's permanent blocker. CONCLUSION: the oracle cannot observe coin+START->73 routing; that routing
is cart-determined, and the missing piece is the BIOS-synthesized credit/arm/start-enable SIGNALS the
cart director consumes (not routing code the BIOS runs). The signal gap is what the HLE bridges.

### (3) Faithful route made to ENTER the chain; walk 73->81 proven; scene 81 node POPULATED
- Added ONE recomp seed: 0602E04E (cart 0x0222a04e func_0222a04e, scene-75 2-fighter HP/signature
  compare; was 'call to 0602E04E, not an entry' on the 74->75 advance). seeds_extra.txt + re-recomp.
- STV_SCENESEED=200:73 + STV_SCENEJOIN=210 (existing default-off knobs): director walks
  73->74->75->78->79->80->81 with REAL cart code, ZERO force-advance, NO reset-timeout, NO FATAL
  (SCENETRACE: 73@201 74@960 75@1183 78@7594 79@7857 80@8770 81@10939).
- At scene 81 the walked node is EMPTY by itself: SCENE81_CTX unk30=0, unk3C=0 (chain did NOT copy
  desc->node; the stage/char-SELECT side-effect never ran on the seeded path). + STV_STAGECFG copies
  the desc stage-config into the walked node 060923E4 -> unk30=0x101 (bit0=1), unk3C=0x06072350 ->
  scene81_stage_setup (func_02206e2e) DMAs the REAL cart-ROM stage bundle. PROVEN by SM81DIAG peek +
  PNG run/TASK_EVIDENCE_routing/sc81_faithfulwalk_stagebundle_forcedcam.png (real stage-tile palette
  greens/olive/browns under STV_HOLDDISP=81 + forced flat cam STV_RBG_MY=256 STV_RBG_FLATK=1;
  fragments, NOT a coherent floor - camera/plane-map not finalized because the round is not live).
- FIGHTERS: id1=1/id2=2 spawn TRANSIENTLY during setup scenes 73/75/78 (SLOTTRACE) but are torn down
  (entity_system_reset per scene bootstrap) before scene 81 -> NO live fighters at 81. Live fighters
  belong to the post-81 round (81 is the stage INTRO, terminal in the director; the long-standing
  post-81 hand-off gap).

### (4) NEW default-off knob STV_PLAYROUTE (bios.cpp stv_scene_probe_tick; byte-identical unset)
Ties chain-ENTRY to the REAL credit-join instead of a fixed VB: on the frame the real join byte
0x06098674/75 reaches 2 (or start flag 0x06098688/89 set) while the director is on an attract fade
scene (3..72), it seeds g_06098668=73 + g_0609866c=desc(73) + join bytes=2 (scene-73 2P branch).
PROVEN to FIRE on the real coin: STV_COINSEQ=100 STV_PLAYROUTE=1 -> "seeded director to scene 73 at
VB980" (right after the real credit byte->2 @VB979) -> scene=73. BUT the walk STALLS at 73: the
concurrent executive credit-join (COV round_controller_join ran=1) competes and scene 73 never
completes its advance. So PLAYROUTE delivers faithful ENTRY-on-real-coin; the clean WALK still needs
the director unimpeded (STV_SCENESEED path, no live executive join). Two-task arbitration is the
precise remaining friction for a single fully-faithful invocation.
Runtime patch captured: port/runtime-patches/playroute-knob-bios.patch. Regression: with PLAYROUTE
unset the documented faithful route still morphs dual->scene_run->scene81 (COV dual=1 run=1 81=1).

### (5) HONEST STATUS / remaining blocker
REACHED: faithful director WALK 73->74->75->78->79->80->81 (real cart code, 1 added seed) + scene-81
node POPULATED with the real stage bundle (peeked unk30 bit0=1 + PNG tiles). NOT reached: a live
POPULATED ROUND at 81 (no live fighters at 81 - structural: 81 is the intro, fighters live post-81;
stage floor not coherent - camera/plane anchor needs the live round). Remaining blockers, unchanged
class (BIOS/system-synthesized couplings): (A) the scene-75..80 stage/char-SELECT side-effect that
copies desc->node AND keeps fighters alive does not run on a seeded/credit path (needs the real
selection game-state) -> stage-pop still needs STV_STAGECFG, fighters don't persist; (B) the post-81
live-round hand-off (81 terminal in the director) - the convergent HLE-BIOS live-stage-activation gap.
Repro (milestone): STV_BOOTMODE=0 STV_CFG798=40 STV_SCENESEED=200:73 STV_SCENEJOIN=210 STV_STAGECFG=1
STV_HOLDDISP=81 STV_RBG_MY=256 STV_RBG_FLATK=1 --stvinput 99999:COIN --shots 11000,11050 --vblanks 11120.

## UPDATE 2026-10-22e: post-81 activation map (premise corrected) + two-task arbitration

CORRECTION: there is NO post-81 round-controller that spawns persistent fighters. Scene 81
(scene81_game_sm func_02206876) is a TERMINAL ENTER-PLAY INTRO: state 5 (cart 0x02206c40)
blanks VDP2, sets the director-advance bit, kill_objects_above_id(0) -- it does NOT spawn the
player. The scene director (func_02205b20) g_06098668==81 branch is a FIXED POINT (no successor).
The boot SM (func_022016ac) then parks at state 13 (bit5 clear).

The real live round is NOT after 81. It lives on the gameplay STAGE scenes (2..72): the executive
*g_06098678 morphs attract_handler(0x02204868) -> round_controller_dual(0x02205232) ->
scene_run_handler(0x02205812), which tail-calls the CURRENT scene stepper (*0x0609867c). For a live
round the DIRECTOR must sit on a STAGE scene (2..72); that scene's DATA descriptor +0x1C spawn-list
(func_0222a878 -> process_spawn_list -> type-1 -> func_02227bc4 slot1=P1 -> player FSM func_0222a300)
spawns the PERSISTENT fighters. Scenes 73/79/80/81 have +0x1C=0 (no player there), which is why the
73->81 walk populates the stage GFX but never the fighters.

### Two-task arbitration IMPLEMENTED + PROVEN (new)
Root cause of the old PLAYROUTE-stall-at-73: an armed coin freezes the attract fade reel (arm
g_06098670=1 -> channel mask 0x06098564=0 -> func_022463c8 early-returns -> fades never complete ->
director entity unk3c bit0 never set), so the one-phase seed overwrote the scene index while the
director was inert. Fix (bios.cpp, default-off, byte-identical unset):
- two-phase STV_PLAYROUTE: phase1 arms on the real credit-join; phase2 (boot-SM state 4 on the reel)
  manufactures the clean boundary faithfully -- sets scene 73 + 2P join bytes, SETS director unk3c
  bit0 so func_02205b20 runs its selector + content spawn, and clears the reset-timeout lever
  (dir unk13=0) so it takes the selector branch not the force-jump-to-81.
- STV_JOINHOLD (the arbitration): across the walk keeps the reset-timeout disarmed; in [73,76)
  reverts any executive morph (join 0x02204afa / dual 0x02205232) back to attract_handler, zeroes the
  executive countdowns, consumes the start-flag edge, re-asserts the 2P join bytes; at >=76 stops
  touching the executive (director installs scene_run itself).
RESULT (real coin): director walks 73->74->75->78->79->80->81, stage bundle DMAs (unk30=0x101,
unk3C=0x06072350), round_controller_join AND dual both ran=0 (join cleanly suppressed). This single
faithful real-coin route did not exist before. Patch: port/runtime-patches/arbitration-joinhold-bios.patch.

### How close / the convergent remaining wall
On the plain faithful coin+START stage path, a PERSISTENT P1 fighter actor (id1=1, entity 060E1AA0)
lives + MOVES on stage scene 3 (VB968..3739, world pos oscillating). But it is NOT rendered: the
frame stays attract-context (HUD + skyline + INSERT COIN, no fighter sprite, skyline floor not a stage).
The two blockers are the SAME convergent absent-BIOS-synthesized couplings every pass hits:
  (A) the executive never faithfully morphs to scene_run ON A LIVE STAGE (the coin->director decoupling)
  (B) VDP1 sprite-emit (per-actor live gate unk4C, actor_main_state_step 0x02229074) + the RBG0
      stage-camera anchor need the live-stage activation signal.
Both are BIOS-synthesized and absent from cart code. The recompiled-BIOS oracle cannot reach them
(it halts in low-level boot before cart director logic).

### Strategic fork (unchanged, now tightly pinned)
Five port passes have each CONFIRMED rather than cracked this one coupling. Two routes remain, both
substantial: (1) keep HLE-RE'ing each absent signal (executive-PLAY-on-stage, sprite-emit gate,
camera anchor) one at a time; (2) boot the real ST-V BIOS inside the runtime so it drives them
natively -- open-ended (RAM-relocation + RAM-built thunks, per real_bios_spike.md). This is a genuine
plateau: the port boots/renders/sounds/reaches ENTER-PLAY + walks the setup chain on a real coin +
spawns a live moving actor, but a RENDERED playable round is gated on this convergent BIOS coupling.


## UPDATE 2026-10-22f: fighter-sprite gate pinned at the VDP1-COMMAND level (dump-proven)

Task: make the live scene-3 P1 fighter (ent 060E1AA0) render a sprite. Result: it does NOT
render, but the gate is now pinned far more precisely than "sprite emits but mispositioned".

TRACE (decomp + peek + VDP1 VRAM dump), faithful scene-3 route (STV_BOOTMODE=0 STV_CFG798=40,
coin@200 START1@340, P1 actor lives VB968..):
- The actor is LIVE and its handler runs: obj 06092524 handler = func_0222863e (player main step)
  -> g_06091100[unk43=1] = func_022286cc -> func_0222c39c (combat handler) + actor_anim_frame_update.
  Peeked: unk4C=0 (NOT busy -> actor_main_state_step does not bail), unk56=0x7F (full HP, live),
  unk34=1 (P1), unk30=0x6E (model id), unk88=0, world pos X=0x12640D (off-map) Y=0 Z=0x182F5.
- It EMITS its full model EVERY frame: textured-sprite count jumps 0 (scene 2) -> 169..179 the frame
  the actor spawns (VDP1DIAG). So the sprite path is NOT gated off; there is no missing emit and no
  live/draw flag blocking it. (Corrects the task premise "never emits a VDP1 sprite" - it emits ~179.)
- But ALL ~179 commands are comm=2 (distorted sprite) with ALL FOUR VERTICES = (0,0,0,0), drawn at
  VDP1 local-coord (352,120) = right-edge/mid-screen -> invisible. CMDSIZE (0x0108..) and CMDSRCA
  (texture) ARE real; only the GEOMETRY is zero. Proven by parsing the VDP1 VRAM command list from
  --dump 1500 (run/probe_base/dump-1500.bin): "textured 179 nonzero-vertex 0, local (352,120)".
- The collapse is INVARIANT to: the stage camera / focus wiring (STV_FOCUSTRACK/STV_CAMTRACK/STV_CAMZ
  sweep -> identical bounds), the executive-mode morph (STV_STARTFLAG -> round_controller_dual on
  scene 3 -> still all-zero), the actor WORLD POSITION (new knob STV_ACTORPOS forced to on-screen
  coords -> still all-zero), and the animation record index (new knob STV_ACTORANIM sweep of
  unk5A/unk5C -> still all-zero). So NONE of position/camera/focus/mode/anim-index is the cause.
- NOT the pose either: the per-actor pose working buffer (0x06096328 + (unk34*108)) is NON-ZERO
  (peek 0x06096394 = live pose data; rec_out 0x060963F4 = non-zero transformed root). The skeleton
  IS animated. The per-sprite projection scratch 0x06098B1C is also non-zero.
- PINNED GATE: the zero geometry is produced at the MODEL-PART distorted-sprite DRAW/transform stage
  (the 02215a2c submit_model_culled -> staged-record -> 022156da/0221582a emitter path), i.e. the
  matrix that transforms the character's loaded model parts (unk30=0x6E; part descriptors/textures
  ARE loaded) to screen space resolves to ZERO. Everything upstream (actor liveness, pose animation,
  per-sprite projection scratch) works; the model-part screen transform is the degenerate element and
  is NOT reachable from any harness lever tried. This is the same convergent absent-BIOS live-stage
  activation (the live view/model matrix setup), now localized to the VDP1 command geometry.

CONCLUSION: no visible fighter; the sprite path is genuinely gated by the absent live-stage-activation
matrix setup, PROVEN at the command level (full model emitted every frame with zero geometry, no
reachable lever de-zeroes it). Honest negative, unprecedented precision.

NEW default-off knobs (bios.cpp, byte-identical unset; verified default run unchanged = 179 tex at
(352,120)): STV_ACTORPOS="X:Y:Z" (force live P1 world pos), STV_ACTORANIM=idx (force unk5A/unk5C).
Patch note: port/runtime-patches/actorpos-actoranim-bios.patch. Next lever to try: peek/force the
actual per-model draw matrix staged by 02215a2c (matrix_copy_reverse_to) - the last un-probed input.

## UPDATE 2026-10-22g: CONVERGENCE RESOLVED — the "degenerate matrix" was a runtime DIVU bug; FIGHTERS RENDER

The pinned "live view/model matrix is degenerate / zero" hypothesis (UPDATE 22f) is WRONG.
No matrix/stack slot is zero. The real cause is a SH-2 on-chip division-unit emulation bug,
and fixing it renders the whole 3D pipeline — visible, animating fighters on the stage.

### What was actually traced (decomp + recomp + VDP1-dump, all empirical)
- The matrix stack (g_06098878 base 0x0609887c), the default matrix @0x0604ec64 (identity),
  the fwd/inv caches @0x06098aec/0x06098abc, the camera translation @0x06098b9c, and the
  reversed model->screen matrix @0x060da040 are ALL populated with valid, animated data every
  frame (STV_MTXPEEK). Nothing is degenerate.
- The distorted-sprite VERTICES come NOT from a live matrix multiply at emit time but from a
  pre-projected corner pool: func_02215cb4 (build_quad_drawlist) runs func_022135a0
  (vertex_project_dualmtx) to project the model verts through M=0x060da040 into pool
  0x060debc8 (master) / 0x060dfbc8 (slave), then func_02213d24 gathers 4 corners per sub-quad
  by index into the da0c8/dc648 draw lists, emitted by func_0221582a/func_022156da.
- Hooking func_022135a0: its INPUTS (matrix + model verts) are valid, and a faithful C
  re-emulation of its projection produced correct on-screen coords (x=-8..330, y=-111..125).
  But the POOL it writes was all-zero at gather time -> every gathered vertex 0 -> all VDP1
  distorted-sprite commands emitted with (0,0,0,0) vertices + real CMDSRCA/CMDSIZE (confirmed
  by parsing the VDP1 VRAM command list: 119/120 zero-vertex).
- The divergence: func_022135a0 stores depth to pool+8 (non-zero) BEFORE the perspective
  divide, then computes recip = (0x100<<32)/depth on the SH-2 DIVU and reads it back from
  DVDNTUL (0xFFFFFF1C). The runtime's onchip.cpp divide() wrote the quotient only to DVDNT
  (0x104) / DVDNTL (0x114) and the +0x20 block mirror — it NEVER populated DVDNTUH (0x118) /
  DVDNTUL (0x11C), the same-block aliases the hardware publishes the result to. So the read
  returned stale 0 -> recip=0 -> xp=yp=0x8000 -> out.x=out.y=0 for EVERY vertex (depth OK).

### The faithful fix (runtime, always-on, 1 line)
onchip.cpp divide(): after the 0x104/0x110/0x114 stores, also
  `wr(o, 0x118, (uint32_t)r, 4); wr(o, 0x11C, (uint32_t)q, 4);`
Patch: port/runtime-patches/divu-dvdntul-fix-onchip.patch. No knob — it is a correctness fix.

### Result (clean regenerated recomp + this fix, ZERO force/knobs)
- Faithful coin+START route (STV_BOOTMODE=0 STV_CFG798=40, coin@200 START1@340): at VB1100/
  1300/1500 the P1 fighter + enemy render as fully TEXTURED, ANIMATING 3D models on the
  bridge/city stage, with HUD health bars + character portraits. VDP1 command parse: 410
  commands at VB1100, ZERO zero-vertex (was 119/120 zero).
- Attract-mode 3D demo also renders now (376 non-zero sprites at VB400) — the DIVU bug had
  suppressed ALL VDP1 3D rendering game-wide (this is why render_status.md said "0 sprites").
- No regression: VDP2 background/text layers unaffected; the fix only publishes the correct
  DIVU result the hardware already aliases.
- Evidence: port/run/TASK_EVIDENCE_divu/fighter_vb1100|1300|1500.png + dump-1100.bin.
- New default-off diagnostic knob STV_MTXPEEK="startvb[:intv]" in bios.cpp (dumps the 3D
  matrix state; byte-identical when unset).

NB: the separate stage-camera/VDP1-local anchor is now the only cosmetic item — sprites
cluster toward screen-top in the attract/transition context; a fully live round centers them
(see VB1100: two fighters framed on the walkway). The model/projection pipeline is SOLVED.

## UPDATE 2026-10-23: post-DIVU re-assessment — INPUT DRIVES THE PLAYER (proven); overlay/PLAY gap pinned

Fresh-eyes pass now that the DIVU fix renders 3D. Reproduced the faithful coin+START route
(STV_BOOTMODE=0 STV_CFG798=40, coin@200 START1@340) byte-identical (shot-1100 md5 7987e477,
== TASK_EVIDENCE_divu/fighter_vb1100.png).

### What the DIVU fix CLEARED (confirmed this pass)
- 3D fighters render textured + animating on the bridge/skyline stage with HUD + portraits.
- The old "black stage floor" wall is GONE: the bridge walkway + night-city skyline render on
  every frame (plain faithful scene 3, no camera force). It was mostly the invisible-render bug.
- "Dead round / 0 sprites" gone. The renderer/plane/tile pipeline is solved.

### NEW: STV_RAWINPUT was DEAD on the faithful route (bug), now fixed -> input drives the player
- BUG: the STV_RAWINPUT write block sits AFTER the stv_scene_probe_tick early-return that bails when
  no director/stage knob is set. On the pure faithful route (no director knob) the raw words
  0x06000708/0x710 were never written -> cart cook func_0220f9d4 read 0 -> cooked input always 0.
  (The 22c "RAWINPUT proven" only worked because STV_SCENESEED was also set.) Fix: added
  STV_RAWINPUT to the guard so it self-enables. Default-off, byte-identical unset.
- PROVEN input drives player: on plain faithful scene 3, injecting raw P1 words makes the cook
  populate 0x06098734 and the player actor (ent 060E1AA0, unk34=1) RESPOND:
  * dir 0x200 (bit9) -> walk left: X 0x132666 -> holds 0x0CD99A (left rail); delta ~0x58000 vs
    the ~0xE000 demo oscillation band. PNG run/TASK_EVIDENCE_live + run/RT_input/shot-2000.png
    (player at far-left railing) vs run/RT_base/shot-2000.png (centered).
  * dir 0x80 (bit7) -> Z 0x18001 -> 0x31998 (depth). dir 0x100 (bit8) -> facing flip ang 0x4000 / 0xC000.
  * button A 0x01 -> forward lunge (X +0x19000); A+C 0x05 -> Y jump 0 -> 0x0D11F.
- NEW read-only knob STV_PLAYERPEEK=<intv> (bios.cpp): logs live P1/P2 world pos + facing + cooked
  input record each intv vblanks via the entity-slot table. Patch:
  port/runtime-patches/rawinput-guard-playerpeek-bios.patch.

### Honest state: it is ATTRACT-mode-on-a-stage, NOT a clean PLAY round
- COV on the plain faithful route: attract_handler ran=1 the WHOLE run; round_controller_dual/join,
  scene_run_handler, scene81_game_sm ALL ran=0. process_spawn_list ran=1 (that is how P1 spawns on
  scene 3). So the executive NEVER leaves attract_handler. The fighters are the stage-3 attract DEMO
  spawn-list actors; they are input-responsive only because arm g_06098670=1 makes the cook feed the
  P1/P2 records and the actor mover reads them.
- "INSERT COIN(S)" is faithful: attract_handler (func_02204868) prints the insert-coin countdown for
  each player whose join byte g_06098674/75 == 0. P1 joined (0x674=2) but P2 (0x675=0) has not -> the
  P2 insert-coin prompt persists -> "INSERT COIN(S)". The overlay clears ONLY when a START flag
  (g_06098688/89) is set, which MORPHS attract_handler into the round controller (and blanks the HUD).
- The faithful morph DOES work via STV_COINSEQ=100 STV_DEMOSTOP=980: player_join_commit_fsm ran=1,
  round_controller_dual ran=1, scene_run_handler ran=1, scene81_game_sm ran=1. BUT the director then
  routes enter-play to scene 81 @VB2727 (terminal intro, blanks by design), and before that the
  round controller sits in a CONTINUE? countdown (run/RT_play3/shot-1800.png: big McClane on the
  bridge + "CONTINUE? 7"). DEMOSTOP also despawns ent 060E1AA0 (the responsive demo P1) and installs
  a fresh idle P1 (ent 060E1900, frozen at 0x132666). So this path = enter-play morph -> terminal 81,
  not a sustained live stage round.

### PRECISE remaining gap (unchanged class, now sharper)
A clean live PLAY round needs the director to land/stay on a gameplay STAGE scene (2..72) in DUAL
mode with scene_run stepping it, instead of routing enter-play to terminal scene 81. Two faithful
couplings remain, both BIOS/director-synthesized and absent from cart code:
  (1) attract_handler morph gate: needs a START flag on the plain route (player_join_commit_fsm never
      commits there) with BOTH demo slots clear so it picks DUAL not JOIN.
  (2) post-morph director routing: round_controller_dual -> scene_run must step a LIVE stage scene,
      not advance the director to terminal 81.
The rendering unknown is solved and input is solved; the frontier is purely this executive/director
PLAY-on-a-live-stage coupling (the same plateau prior passes converged on).

## UPDATE 2026-10-23b: coupling #1 SOLVED (morph keeps responsive player); coupling #2 pinned to play-scene descriptor

Fresh pass on the two couplings from 2026-10-23. New default-off labeled bridge **STV_LIVEROUND="vb[:stage]"**
(bios.cpp stv_scene_probe_tick; byte-identical unset -- VERIFIED: plain faithful shot-1700 md5
979de894... reproduces with knob unset). Patch: port/runtime-patches/liveround-knob-bios.patch.
Evidence: port/run/TASK_EVIDENCE_liveround/.

### Coupling #1 (MORPH GATE) -- SOLVED, responsive player NOT lost
The prior COINSEQ+DEMOSTOP path reached DUAL but despawned the responsive demo P1 (060E1AA0 ->
idle 060E1900) and routed to terminal 81. STV_LIVEROUND instead presses START the way a player's
hand would -- sets the cart's own START edge flags across two frames:
- VB1200: set g_06098688 (P1 START). attract_handler picks round_controller_JOIN (demo entity
  slots g_06099594/98 are non-zero while the live demo P1 is alive, so its dual-vs-join gate can
  NOT pick DUAL without clearing them = despawning the player). So we go via JOIN deliberately.
- VB1201 (handler now join 0x06008AFA): set g_06098689 (P2 START). round_controller_join's
  "other player pressed start" branch (func_02204afa) morphs -> round_controller_DUAL (0x06009232).
- The demo actors are NEVER cleared -> responsive P1 (060E1AA0, unk34=1) survives.
- dual's countdown (entity unk40/unk44, armed 0x27f by the cart on the START edge) counts down and
  func_02205232 installs scene_run_handler (ENTER PLAY) at ~VB2481, all REAL cart code.
PROVEN COV (plain real-cart morph, held in the dual window): attract=1, join=1, DUAL=1,
scene81=0. Player stays input-responsive THROUGH the morph (arm g_06098670=1 keeps the cook
feeding P1/P2): dir 0x200 -> X 0x12966A->0x0D8CCD (walk left to rail); dir 0x80 -> Z 0x18001->
0x31998 (depth); dir 0x100 -> ang 0x4000->0xC000 + X right (facing flip). 5 shots 1300..2450 all
md5-different = live/animating. NB: handler slots hold RECOMP addrs (cart+0x03E04000): attract
0x06008868, join 0x06008AFA, dual 0x06009232, scene_run 0x06009812 (JOINHOLD's cart-addr compares
are stale on this port; LIVEROUND uses recomp addrs).

### Coupling #2 (POST-MORPH ROUTING) -- director held on stage; scene_run crash pinned
Holding arm=1 keeps the director (func_02205b20) parked on stage scene 3 (g_06098668=3,
g_0609866c=0x06072750): the attract fade never completes -> director entity unk3c bit0 never set ->
its main body (incl. the reset-timeout force-jump-to-81) never runs -> **scene 81 is never reached**
(scene81_game_sm COV=0). This is strictly better than COINSEQ+DEMOSTOP (which routed to terminal 81).

BUT scene 3 is an ATTRACT-DEMO descriptor, not a PLAY scene. When dual's countdown expires and
scene_run installs (VB2481), the game's setup-scene SM func_02218038 (cart 0x02218076 -> store fn
cart 0x02213A70) reinstalls the PLAY scene-object handler scene_obj_handler (func_0220f6f6, recomp
0x060136F6) into the scene node *g_0609882C (=060922A4) within the SAME frame. scene_obj_handler
case-0 "restart" reads scene 3's desc+0x10 (*(0x06072760)=0x0600F1C0 = cart CODE) as a play
DEFINITION and calls its garbage unk0 = 0x2FE62FD6 -> FATAL "call to 0FE62FD6" (pr 06013754) at
VB2483. Scene 3's descriptor has no play-definition layout; the install+call are mid-frame so a
vblank-in revert is too late (confirmed via a reverted STV_LRSTAB probe, removed). The reachable
crash-free sustained state is therefore the DUAL window.

### Honest state -- how clean a round
Reached: a SUSTAINED (VB1203..~2480, ~1280 frames) crash-free round with the executive morphed to
round_controller_DUAL stepping the live bridge stage, the input-responsive fighter 060E1AA0 moving
+ animating + rendering (textured 3D on the catwalk), 2 health bars + 2 portraits HUD. scene81
never reached; director held on a stage scene 2..72.
NOT clean: the "INSERT COIN(S)" (P2 unjoined) + "CONTINUE? N" overlay PERSISTS -- it is baked into
round_controller_dual's own per-frame coin/continue-countdown display (func_02238134/func_02238176
+ the g_06090304 string table), inherent to the coin/continue controller that PRECEDES true play.
A fully clean round (no overlay, play HUD with lives/score) lives only in scene_run on a REAL
PLAY-scene descriptor (play-definition layout at desc+0x10, both-joined, play HUD) -- which only
the absent setup chain 73-80 builds. This is the same convergent play-scene-context wall; this pass
narrows it to a precise, descriptor-level diagnosis and proves the executive morph + player
persistence are no longer the blocker. Overlay-gone is now the single remaining item, and it is
structurally downstream of the setup-chain play-scene (not an attract-scene property that can be
toggled off).

## UPDATE 2026-10-23c: scene_run FATAL SOLVED (route-b play-def synthesis); overlay-free scene_run proven

Bounded one-pass chase for a clean overlay-free play round. Outcome: the scene_run crash (coupling #2's
hard wall) is SOLVED, and scene_run now steps a real, overlay-free, input-responsive play scene with the
fighters rendered. New default-off knobs (byte-identical when unset; two consecutive plain-faithful runs
md5-identical, faithful VB1700 frame = bridge+fighters+HUD+INSERT COIN as documented).

### (1) THE play-definition descriptor layout (desc+0x10) -- decomp-cited + empirically dumped
scene_obj_handler (func_0220f6f6, recomp 0x060136F6) case-0 does, with cur-descriptor g_0609866c:
  d = *(g_0609866c + 0x10);            // the "def" pointer
  func_0220f816(scene, d);             // binds: scene->unk10=(u8)d->unkC; obj->unk6C=(s16)d->unkC>>8;
                                       //        obj->unkAE from d->unkE (0220f816.c)
  ((void(*)(void*,s32,s32))*(d+0))(obj, *(d+4), *(d+8));   // CALLS d->unk0 as a function
So a GENUINE play-def at desc+0x10 is a struct:
  +0x00 unk0 : the step-callback FUNCTION POINTER (cart code 0x06004000..0x0604A000). THE crux field.
  +0x04 unk4 : s32 arg1 -> into the per-scene data block (e.g. 0x06063E2C)
  +0x08 unk8 : s32 arg2 -> into the per-scene data block (e.g. 0x06063E44)
  +0x0C unkC : u16 substate seed (low byte = initial sub-state 0/1/2/3; high byte = obj->unk6C)
  +0x0E unkE : u16 countdown (obj->unkAE)
  +0x10      : next def (chain) or 0 (terminator); scene_obj_handler cases 2/3 advance to cur+0x10.

FULL 90-DESCRIPTOR DUMP (new knob STV_DESCDUMP; run/TASK_EVIDENCE_playdesc/descriptor_dump.txt) shows
TWO descriptor families, and desc+0x10 is DUAL-PURPOSE by family:
 - PLAY scenes (2,4,6,8,10-12,14,16,18,20,22,24,26-29,31,33,35,36,38-41,43,45-55,57-59,61,63,65,67,68,
   70,71,73,74,76-79,85-87): desc+0x10 -> a DEF STRUCT whose unk0 is a real cart-code callback
   (commonly 0x060125BC = cart func_0220e5bc; also 0x06012BDE/0601328E/060130EC/06012A06/0601315E/...).
   off14 (desc+0x14) in {0x0601F080,0601FEB8,06020E1C,060212E0,06022AC0,06023240,...} = the stage/setup
   handler fns (func_0221b080/0221ce1c/.../0221f240...). These are the scene_obj_handler scenes.
 - ATTRACT-FADE scenes (3,5,7,9,15,17,19,21,23,25,32,34,37,44,48,51,62,64,66,69,72,75): desc+0x10 is a
   DIRECT CODE POINTER (0x0600F1C0 / 0x0600F6B2 / 0x0600FC20 -- all real function entries the attract
   handler 0x06036610/BC4 CALLS directly). Read AS a play-def, unk0 = the first instruction word
   (0x2FE62FD6) = a non-fn-ptr -> FATAL. Scene 81's desc+0x10=0x0600AC88 is likewise direct code
   (scene81_game_sm path), unk0=0x2FE6E010.

=> The LIVEROUND scene_run FATAL was ONLY this family mismatch: the faithful route holds the director on
ATTRACT scene 3 (desc 0x06072750, +0x10=0x0600F1C0 direct-code), and round_controller_dual->scene_run->
func_02218038 reinstalls scene_obj_handler, whose case-0 misreads scene 3's direct-code +0x10 as a play
def and calls 0x2FE62FD6 -> FATAL. NOT a structural round-context wall; a descriptor-type mismatch.

### (2) ROUTE TAKEN -- (b) SYNTHESIS, plus (a)-lite confirm. Build: clean, ~8s rebuilds, no recomp regen.
New default-off knobs in bios.cpp stv_scene_probe_tick (patch port/runtime-patches/synthdef-descdump-bios.patch):
 - STV_DESCDUMP="vb"            one-shot dump of all 90 descriptors' +0x10 def (unk0..unkE) + off14/spawn.
 - STV_SYNTHDEF="dst[:src[:startvb]]"  from startvb, overwrite scene `dst`'s desc+0x10 with scene `src`'s
   def pointer (src default 2). The route-b synthesis.
Route (a)-lite: STV_LIVEROUND="1200:2" (hold a NATIVE play-def scene, scene 2): scene_run steps it with
   NO FATAL (ran clean to 6000 VB) -- but func_02218038's play-enter bootstrap blanks VDP2 + tears down
   the demo actors, so the frame goes BLACK (all shots md5-identical black). Clean round is downstream of
   that intro completing (unpopulated context). Proves no-crash but not a visible round.
Route (b): STV_LIVEROUND=1200 + STV_SYNTHDEF="3:SRC:1210" -- hold the FAITHFUL scene 3, and AFTER the
   morph (VB1210, attract no longer using scene 3; patching BEFORE the morph crashes attract since it
   calls +0x10 directly) overwrite scene 3's +0x10 with a real play-def from scene SRC. scene_run ->
   scene_obj_handler steps SRC's content over scene 3's STILL-RENDERING skyline/bridge bg.

### (3) RESULT -- scene_run steps a non-crashing, OVERLAY-FREE, input-responsive play scene
PROVEN (run/TASK_EVIDENCE_playdesc/):
 - NO FATAL: "executive in scene_run_handler at VB 2481 -- SUSTAINED LIVE ROUND on stage scene 3",
   ran clean to 3700+ VB. scene_obj_handler IS the live handler post-scene_run: peek node 060922A4
   hdl=060136F6 ent=060E13B0 st=2 (the "run" state), g_0609866c=0x06072750 (scene 3) +synth def.
 - OVERLAY GONE: no INSERT COIN / CONTINUE? -- that overlay is attract/dual coin-display; scene_run on a
   play-def doesn't draw it. Scenes rendered (SRC-dependent, all animating, all shots md5-different):
   SRC=2 -> the POLICE-helicopter intro over the night skyline; SRC=6/8 -> the TWO PLAYER FIGHTERS
   (blonde Kris + McClane in blue POLICE shirt) rendered as textured 3D models over the skyline floor.
 - PLAYER RESPONDS TO INPUT: two actors spawn post-scene_run (060E18B0 unk34=1, 060E1A50 unk34=2). With
   STV_RAWINPUT P1 injected vs a no-input reference, P1 cook reg + world pos change (Z 0x188DE->0x31998)
   while the reference stays static. PNGs run/TASK_EVIDENCE_playdesc/shot-{2850,3000,3300,3600}.png.

### (4) HONEST remaining gap (narrowed, same convergent class)
This is NOT yet a pixel-perfect combat round: (a) the FLOOR under the fighters is scene 3's night-skyline
backdrop (the held descriptor's stage-config/RBG0 camera is still scene 3's, not SRC's live stage), and
(b) VDP1 sprite OVERDRAW at screen-top -- the same unfinalized live-stage camera / VDP1-local anchor the
notes already track (the fighters render but aren't centered/clipped by a live play camera). Both are the
convergent live-stage-camera-anchor item, now the LAST cosmetic blocker. The crash and the overlay are
BOTH gone; what remains is pointing the held descriptor's stage-config at SRC's stage + finalizing the
play camera so the fighters stand on SRC's real floor centered -- a bounded next step, no longer a FATAL.

## UPDATE 2026-10-23d: OVERDRAW SOLVED (VDP1 manual-mode erase); FLOOR = stage DMA proven, perspective floor still camera-gated

Cosmetic-finish pass on the route-b scene_run round (repro: STV_BOOTMODE=0 STV_CFG798=40
STV_LIVEROUND=1200 STV_SYNTHDEF="3:8:1210" --stvinput "200:COIN,340:START1"). New default-off
knobs; two consecutive no-knob runs byte-IDENTICAL (shot-1700 md5 9b19b7f9...), so all additions
are inert when unset. Patches: port/runtime-patches/{vdp1erase-vdp1.patch,stagedma-descdump-bios.patch}.
Evidence: port/run/TASK_EVIDENCE_camerafloor/ (BEFORE_overdraw_3300.png vs shot-2800/3000/3300.png).

### (b) THE SCREEN-TOP OVERDRAW WAS A VDP1 FRAMEBUFFER-ERASE BUG (SOLVED, clean centered fighters)
Parsed the live VDP1 command list from --dump (TASK_EVIDENCE_camerafloor/dump-3300.bin): the CURRENT
frame emits ONE coherent group of 163 model-part distorted-sprites, ALL within x[304..386] y[114..160]
in VDP1's 704-wide 8bpp space (local-coord cmd = (352,120) = screen CENTER). i.e. the fighters were
ALREADY centered + correctly framed every frame. The "giants clustered at screen-top" were NOT in the
command list -- they are STALE accumulation in the display framebuffer.
Root cause (runtime): the game drives VDP1 in MANUAL frame-change mode (FBCR FCM=1) and triggers a
buffer CHANGE with FBCR=0x0003 (FCT=1) but no separate ERASE arm (FBCR=0x0002). vdp1.cpp vblank_out
only auto-erases in one-cycle mode (`if (one_cycle) g_erase_field=true`) or on an explicit erase arm,
so the new draw buffer kept 2-frames-ago sprites. Because the live draw (a compact fighter group over
a VDP2 background) covers only part of the screen, old close-up-camera frames (e.g. the VB2520 intro
giant) persist forever -> the top-cluster overdraw.
Fix: STV_VDP1ERASE=1 arms an erase on every manual change (mirrors one-cycle). RESULT: overdraw GONE
-- a single clean, centered Kris+McClane pair over the night skyline, no giants/cluster, no overlay,
sustained (shot-2800 large/close, 3000 medium, 3300 far -- the scene's own camera "breathes"/zooms;
faithful, not player-tracked). This is almost certainly a FAITHFUL correctness fix (manual-mode erase
is incomplete in the runtime); kept as a labeled default-off knob pending broader cross-game testing,
recommend promoting to always-on (like the DIVU fix) after that.
NB: STV_FOCUSTRACK had NO visible effect here (verified) -- the camera is scene 8's own content camera,
which already frames the fighters; player-tracking is not the lever. The overdraw, not the camera
anchor, was the real artifact.

### (a) FLOOR: stage-bundle DMA PROVEN for SRC's real stage; coherent perspective floor still gated
Full descriptor stage-config map dumped (new STV_DESCDUMP ext + run/rb_descdump): EVERY scene desc
(base 0x06072660 stride 0x50) carries +0x30 flags(bit0=DMA gate) /+0x34/+0x38/+0x3A/+0x3C(bundle).
SRC scene 8 = {30=0x21,34=0x30001,38=8,3A=8,3C=0x06072378}; scene 3(held)=3C=0x0607238C(skyline);
SPECIAL scene-81 desc 0x06073FB0 = {30=0x101,38=1,3A=1,3C=0x06072350}(real first stage, greens/olive).
New STV_STAGEDMA="vb[:src[:intv]]" builds a scratch ctx from SRC's stage-config and CALLS the game's
own scene81_stage_setup (func_02206e2e, recomp 0x0600AE2E) -- the real DMA+RBG0 plane/palette setup --
and mirrors SRC's config into the held descriptor (g_0609866c). PROVEN: DMAs SRC's bundle (log shows
bundle=06072378 for src=8; 06072350 for src=0x06073FB0), BGON keeps R0ON (bit4) at PRIR=4 > NBG0's 3.
BUT the RBG0 floor does NOT render coherently: its live camera is stale (My~1754 off-map, KAst=31872
off-table, coefficient table all-zero) -- written by the absent live-stage camera driver. Forcing the
camera (STV_RBG_MY + STV_RBG_FLATK, both scene-8 and scene-81 bundles) yields garbled tile grids /
white expanse (plane-map + coefficient perspective not finalized), NOT a coherent floor -- exactly the
convergent live-stage-camera wall every prior pass hit. So the FLOOR DMA machinery is proven and
pointable at SRC's real stage, but a faithful textured perspective floor remains gated on synthesizing
the live-stage coefficient table + plane-map (the absent BIOS live-stage camera). The cleanest round
frame therefore leaves RBG0 unforced: fighters on a dark stage floor band under the real night skyline.

### Honest state / remaining gap
Clean round delivered: two fighters CENTERED + correctly framed, NO screen-top overdraw, overlay-free,
input-responsive (raw-input injection moves the player vs a static no-input reference), sustained. The
ONE remaining cosmetic item is the TEXTURED perspective floor: stage tiles DMA into VRAM faithfully,
but rendering them as a coherent ground needs the live-stage RBG0 coefficient table/plane-map that the
absent BIOS live-stage camera fills (forcing it is garbled). Same convergent wall, now isolated to the
RBG0 coefficient/plane synthesis -- the crash, overlay, and overdraw are all gone.
Repro (clean): STV_BOOTMODE=0 STV_CFG798=40 STV_LIVEROUND=1200 STV_SYNTHDEF="3:8:1210" STV_VDP1ERASE=1
  --stvinput "200:COIN,340:START1" --shots 2800,3000,3300 --vblanks 3400

## UPDATE 2026-10-23e: FLOOR RENDERS -- coherent textured perspective ground (coefficient-address port BUG, faithful fix)

The "RBG0 floor is camera-gated / coefficient table absent" conclusion (23d and five prior passes)
was WRONG for the LIVEROUND context. The live-stage RBG0 camera driver IS running and DOES build a
valid per-line 1/z coefficient table every frame; the port was reading it from the WRONG VRAM
ADDRESS. Fixing that one address renders a coherent textured perspective floor under the fighters.

### (1) The RBG0 floor pipeline (decomp-cited; traced this pass)
- func_022225f6 (022225f6.c) = the per-frame camera-matrix compose. Reads focus object *(0x0609882C)->+8
  (world pos +8/+C/+10, yaw +0x24, pitch +0x26) and camera 0x060755D4; builds the 3x3 rotation from
  sin/cos (func_02213190/022131a8) into the work-RAM matrix block *(0x060B8B90)=0x060B89D0 (set A),
  then calls the coefficient builder. Called per frame from frame_sync_loop (FUN_022013c4 @0x022014aa/b0).
- func_02222c08 (02222c08.c) = the per-SCANLINE 1/z coefficient builder (240 entries, SH-2 DIVU, midpoint
  densified). Output buffer *(0x060BAC98)=0x060B9698.
- func_0221660c (0221660c.c) = per-VBlank DMA (via isr_vblank_in 02223cf0.c -> func_02216530): walks the
  4 descriptors at 0x06099514 (stride 0x18); desc0 matrix block -> VRAM 0x5F000 (0x1C longs), desc2 coeff
  buffer -> VRAM 0x5F200 (0xF0 longs = 240 x s32). Descriptors installed by func_02221f68(4,0x25e5f000)
  and func_02222052(6,0x25e5f200) during scene81_stage_setup (func_02206e2e) -> func_0221636c attr_dispatch.
- Gates: 0x06099578 (draw-active), 0x060BACAC/0x060BACB0 bit0 (per-camera enable), 0x060E0BC8 (vblank
  commit). PEEKED LIVE in our round: ALL = 1, focus 0x0609882C=0x060922A4->+8=0x060E13B0 (live). Gates OPEN.

### (2) The actual bug plus faithful fix (1 line, vdp2.cpp read_rot_param)
VRAM 0x5F200 in the live round holds a PERFECT 1/z ramp (K = 4.55 at y16 -> 2.24 -> 1.49 -> 1.11 -> ... ->
0.32 at y224, monotonic; peek run/TASK_EVIDENCE_floor/coef_ramp_0x5F200.txt). The port computed the
coefficient base as kt_base = KTAOF<<16 (=0x10000) + KAst*4 (=0x1F200) = 0x2F200 -> read unrelated tile
data -> coefficient garbage/mostly-transparent -> dark band. But the cart ENCODES the table address as
KTAOF = addr>>18 and KAst = addr<<14 (attr_dispatch cmd6: ins=(addr>>2)>>16, KAst=r14<<14). So the decode
MUST be KTAOF<<18: 1<<18=0x40000, +0x1F200 = 0x5F200 (exact). Fix: s.kt_base = (ktaof<<18) and 0x7FFFF.
FAITHFUL correctness fix (like the DIVU fix), RBG0/RBG1-only: NBG0 skyline / sprites / VDP1 untouched;
the documented clean round is preserved (run/REGCHK). Patch: port/runtime-patches/rbg0-coef-ktaof-shift18-vdp2.patch.

### (3) The one remaining forced knob (plane-map), and why
With the real coefficient read, the real matrix My~1810 anchors sampling into map rows 1024-2047 (plane
0x2E) and 0-511 (0x00) -- which scene81_stage_setup's hybrid DMA leaves EMPTY (it loads only floor page
0x38 @0x070000, map rows 512-1023). A truly-live round fills the full 4x4 plane set; our route-b hybrid
(held scene 3 + SYNTHDEF fighters + STAGEDMA floor bundle) does not. So the EXISTING default-off knob
STV_RBG_PLANE=070000 (force all 16 plane slots to the loaded floor page) is needed to put the floor
texture wherever the real camera+coefficient sample. The perspective itself is 100% the game's real
authored 1/z coefficient table -- not flat-K, not synthesized.

### (4) RESULT -- coherent textured perspective floor (PNG-proven)
Repro (clean, floor): STV_BOOTMODE=0 STV_CFG798=40 STV_LIVEROUND=1200 STV_SYNTHDEF="3:8:1210"
  STV_STAGEDMA="1210:0x06073FB0" STV_VDP1ERASE=1 STV_RBG_PLANE=070000
  --stvinput "200:COIN,340:START1" --shots 2800,3000,3300 --vblanks 3400
(0x06073FB0 = scene-81 real-first-stage descriptor, bundle 0x06072350 = the greens/olive GROUND texture;
scene-8 bundle 0x06072378 is a cityscape-ish RBG0 page, less ground-like -- the green bundle is a true floor.)
Evidence: port/run/TASK_EVIDENCE_floor/shot-{2800,3000,3300}.png -- two fighters (Kris + McClane) CENTERED
on a green checkered PERSPECTIVE ground (large tiles foreground -> compressed toward the horizon where it
meets the night skyline), overlay-free, overdraw-free, sustained, the scene camera "breathing"/zooming
(2800 close, 3000 medium, 3300 far) -- all three show a coherent receding floor, NOT a flat band, NOT a
garbled grid. coef_ramp_0x5F200.txt = the valid 1/z table the port now reads.

### (5) Remaining gap (small, precise)
Fully faithful (no STV_RBG_PLANE) needs the stage DMA to populate the full plane set (0x2E far plane plus
0x00) the way a true selection-chain stage build does, OR the camera My to frame the single loaded page --
i.e. the route-b hybrid camera/plane inconsistency, not a renderer limit. The renderer plus the real
coefficient/matrix are correct; the coefficient-address fix is the substantive unlock. The floor is no
longer the convergent "absent BIOS camera" wall -- that premise is retired.
