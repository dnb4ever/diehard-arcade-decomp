# Die Hard Arcade (ST-V) RECOMP - the attract -> credit/join scene trigger (and whether a coin fires it)

Date: 2026-10-20. Port side only (read-only analysis + live --watch; NO source edits this session).
Work in isolated copy ~/decomp/dh-portA/. Reference C read-only from main project/src_functional/.
Address map: runtime(HWRAM) = cart SH-2 + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.

## ANSWER: (B) + a reframing. The attract->scene-73 advance is NOT coin-driven, and scene 73 is
## not the "coin -> play" gate at all. The port advances 0->2->3 then PARKS at scene 3, far
## upstream of scene 73. A coin never writes the scene index nor the director-advance bit.

### Headline corrections to the mission premise
1. The scene index does NOT "stay at 2". Live (clean boot, real coins) it walks 0 -> 2 (VB203)
   -> 3 (VB1038) and then PARKS at 3 through VB5000 (re-confirmed; matches scene_director_status.md).
2. The credit/join STATE MACHINE (0220600e scene_credit_join_sm) is scene **index 1**, NOT 73.
   Proof: scene-table off14 (handler ptr) @ idx1 = 0x0600A00E (= rt of cart 0x0220600e). Scene 73
   is only the credit-BRANCH scene (table selector 3); it has off14 0x06022AC0, a different handler.
   Older notes called 73 "the credit scene" because of its selector; the real credit UI SM is idx 1.
3. Scene 1 (the credit SM) is UNREACHABLE from the attract demo. Full scene-table graph dump (all 90
   entries, cart.bin @0x26E660, stride 0x50): the ONLY edge into idx1 is idx1 itself (self, sel 4);
   the ONLY edge into idx0 is idx0 (sel -1 -> forces idx=2). Scene 1 is entered ONLY at BOOT, via
   0201b92 (region/controller gate) or scene_bootstrap else-branch (auto-start dip g_06000654 & 0x100).

### What actually advances the scene director (the real mechanism)
scene_director_build (cart 0x02205b20 = rt 0x06009b20), called every frame from boot_scene_director
state 4 (022016ac), runs its advance body **only when director_entity->unk3c bit0 is set**. On advance
it selects the next scene from the current scene`s selector cur[43] (scene table @ rt 0x06072660):
  -1: idx=2 | 0: idx=cur[44] | 1: cur[44+(g&1)] | 2: cur[44+(g&3)]  (g = g_0609856c)
  3: (g_06098674==2 && g_06098675==2) ? cur[45] : cur[44]        <- the credit-count test (idx 73 ONLY)
  4: (g_06098670 != 0) ? cur[45] : cur[44]                       <- the 2P-armed test (idx 1 ONLY)

The attract reel is a LINEAR walk 0 -> 2 -> 3 -> 4 -> ... -> 72 -> 73, every edge selector 0/1/2
(unconditional or g_0609856c-indexed). **No scene in 2..72 reads the credit count.** The credit count
is consulted for the FIRST time at scene 73 (selector 3), deep in the game-enter sub-sequence.

Each hop requires that scene`s CONTENT task to emit the completion signal (director_entity->unk3c
bit0). The scene-content sequencers that set that bit in a terminal state are decompiled
(0201f20 vdp2_update_b state 2: `task->unk14->unk8->unk3C |= 1`; 02202138; 022027e4; and the two SM
handlers 0220600e / 02206876). f_220fece (the scene_director_build tail) is decompiled and merely
allocates a generic 0x6C object (220fece.c) - it does NOT interpret the per-scene script (off14).

### Does a coin fire it? NO. (decisive live + static proof)
- attract_credit_join_service (022045f8), run every frame by attract_handler (02204868), is the ONLY
  coin consumer during attract. It bumps g_06098674 (0->1 coin-accept, 1->2 join-confirm). It writes
  NEITHER g_06098668 NOR any director-advance bit. Confirmed by reading it end-to-end.
- All 13 writers of g_06098668 enumerated (grep): boot fade/init (0201b92, 0201c9c, 02201958),
  scene_bootstrap, scene_director_build, and gameplay fade/sound/HUD readers (0220340c, 02215fac,
  0221603e, 0221ad58, 0221af30, 0222a300/5d0/91a). **None is triggered by a coin/credit edge.**
- LIVE (STV_BOOTMODE=1, --stvinput "200:COIN,260:COIN", NO forcing knobs):
    g_06098674: ->1 ->2 @VB232, ->3 @VB262  (coins accrue through attract_credit_join_service)
    g_06098668: 0 @VB203 (boot scene0 sel-1 -> 2) ; 2 -> 3 @VB1038 (scene_director_build, pr 06009B46)
  The 2->3 advance is ~835 frames after scene 2 began - a demo timer, completely independent of the
  coins at VB200/260. Then scene index is static at 3 to VB5000.

### The precise unmet gate (Z), upstream of 73
The port parks at **scene 3** (first fade/transition scene; off14 0x06036610, b42=1). Scene 2 (demo
movie, off14 0x0601F080) completes at ~835 frames and advances; scene 3`s fade/transition content
never emits director_entity->unk3c bit0, so the linear demo reel stalls. This is the SAME wall
scene_director_status.md S5 flagged ("attract PARKS at scene 3"). To reach scene 73 faithfully the
ENTIRE ~70-scene attract reel (demo movies, demo fights, fades) must each run to completion - this is
a deep content chain, not a single coin gate.

B/C seam on scene 3: the completion-bit setters are decompiled, but HOW a given scene`s off14 script
(0x06036610 for the fades) selects/drives/ticks its content task is NOT pinned - f_220fece ignores
off14, so the off14-script -> content-task install path runs through still-unmapped code. The exact
scene-3 completion condition (frame timer vs event) is therefore the next thing to decompile.

### Convergence verdict (honest)
- Scene 73 is NOT the "coin -> play" gate. The faithful coin->play path is attract_handler`s self-morph
  to the round controller, gated on the START flag g_06098688 - which (per drive_real_flow_status.md)
  needs the player-join actor FSM (0x0602DCAC), which needs the player-actor spawn, the known cycle.
- The scene-director path to 73 is a SEPARATE, demo-reel-driven route and is blocked much earlier, at
  scene 3. Neither path is one gate from play.
- This reveals another downstream layer, not a single trigger: the attract reel`s per-scene content
  completion (scene 3 first). The port is converging in understanding but the remaining faithful work
  is substantial (either play the whole attract reel, or resolve the START-flag/actor-spawn cycle).

## Files changed this session
SOURCE: NONE. No edits to port/ or tools/saturn-recomp/. Pure read-only analysis + live runs.
NEW (this report): notes/port/attract_to_credit_trigger.md
RUN ARTIFACTS (not source): run/q_watch/, run/q2/, run/s3park/ (+ shots 700/1500/3000).
NEW SEEDS: none (all runs exit clean; no new "call to ADDR" walls).

## Frames (honest; attract only, RBG1 night-skyline backdrop)
- run/s3park/shot-700.png   - scene 2 (demo) active
- run/s3park/shot-3000.png  - scene 3 PARKED (index static at 3)
- Mac scratchpad: attract_scene2_demo_vb700.png, attract_scene3_park_vb3000.png

## Reproduce (p620, in the copy)
    cd ~/decomp/dh-portA/port
    STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
      --vblanks 5000 --out run/s3park --watch 06098668:0609866c --stvinput "200:COIN,260:COIN"
    # -> 06098668: 0(VB203)->2(VB203)->3(VB1038), static at 3 thereafter; coins only move 06098674.
