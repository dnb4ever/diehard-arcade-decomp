# Die Hard Arcade (ST-V) RECOMP — the scene-director trigger (attract -> game scene)

Date: 2026-10-19. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Builds on: play_transition_status.md, boottask_status.md, credited_round_status.md.
Address map: runtime(HWRAM) = cart SH-2 addr + 0x03E04000; cart SH-2 addr = cart.bin offset + 0x02000000.
Work done in isolated copy ~/decomp/dh-portA/ (reconcile into main tree).

## TL;DR — the scene-director trigger is FOUND and FIRED. It is NOT boot states 5-10.

The game-scene director RUNS every frame and IS actively sequencing scenes. The trigger that routes it
to the GAME scene is the scene-director task's byte **unk13 (+0xd) going negative** — checked by
scene_director_build (cart 0x02205b20) at every scene advance. I reproduced it faithfully (new
flag-gated STV_SCENEJUMP): the game-scene descriptor g_0609866c=0x06073fb0 (scene 81) IS built and its
setup runs. BUT forcing that jump lands DIRECTLY on scene 81 and skips the stage-setup scenes (73-80),
so no player actor spawns and the scene times out / soft-resets after ~310 frames. The FAITHFUL path
(attract walks to the credit-branch scene 73) is blocked by a SEPARATE wall: the attract PARKS at scene
3 (the combat demo never signals completion). Honest verdict: trigger solved; two downstream walls named.

## 0. CORRECTION to prior notes (important)

- play_transition_status.md S4/S5 said "g_0609866c is set only by the morph-target handlers OR boot
  states 5-10, which NEVER RUN." **That is wrong.** g_0609866c is written **every scene advance** by
  scene_director_build (cart 0x02205b20 = rt 0x06009b20), which is **called from boot-machine state 4**
  (func_022016a0 @ cart 0x022017ec: `bsrf 0x432e` -> 0x02205b20). Observed live: g_0609866c walks
  060740A0 -> 06072660(scene0) -> 06072700(scene2) -> 06072750(scene3). The scene director is NOT
  "boot states 5-10"; it is state 4's per-frame call. (Boot states 5-10 ARE a real, separate sub-loop
  5->6->...->10->1 that nothing in the harness enters, because states 1-4 never set ctx[0x10]>4 and only
  an external writer of ctx[0x10]=5 would start it. They are not the scene director and are a dead end
  here.)

## 1. The scene director, exactly (scene_director_build, cart 0x02205b20)

Handler ABI (task): entity = task->unk8. It only does its body when **entity->unk3c bit0 is set**
(the per-scene "advance now" signal); otherwise it just counts entity->unk50 (>4 raises
0x06098812/0x0609880e). On advance it SELECTS the next scene and writes:
    cur = *g_0609866c
    if ((s8)task->unk13 < 0):  g_06098668 = 81;  *g_0609866c = 0x06073fb0;  g_060987d0 = 1;   // <== TRIGGER
    else: switch ((s8)cur[43]) { -1:idx=2; 0:idx=cur[44]; 1/2: g_0609856c-indexed; 3: credit-branch; 4: g_06098670-branch }
          *g_0609866c = 0x06072660 + (s16)(idx*0x50);
    if (g_06098668 >= 76): reset join bytes (0x06098674/75=0), install f_2205812 into *g_06098678   // game-enter
    g_0609868e = cur[42];  f_220fece(task, *(cur+20));   // (cur+20) = the scene's tokenized SCRIPT ptr

So progression is SCRIPT-DRIVEN: cur+20 (off14) is a tokenized scene script (e.g. 0x0601f080,
0x06036610, 0x06022ac0 ...), interpreted by f_220fece; the script decides scene duration and when to
set entity->unk3c bit0 to advance.

## 2. The scene graph (table @0x06072660, stride 0x50) — the game is scene 81

Parsed all 90 entries (fields: +0x14 script, +0x2b signed selector, +0x2c/+0x2d next candidates).
Verified against live transitions (idx0 sel -1 -> 2; idx2 sel 0 -> cur[44]=3; both observed).

- Attract/demo scenes: 0,2,3,4,... (even=demo script 0x0601f080, odd=fade script 0x06036610).
- Boot scene = idx 84 (0x060740a0) — the INITIAL g_0609866c value at power-on.
- **Credit-branch = idx 73 (0x06073d30), selector 3:** next = (0x06098674==2 && 0x06098675==2) ? 74 : 79.
  Both branches then converge 74->75->(76/77/78)->79->80->**81 (GAME)**. Scenes 76-80 are the
  character/stage-intro + stage-LOAD setup scenes. Scenes >=76 trigger the "game-enter" reset above.
- **GAME scene = idx 81 (descriptor 0x06073fb0):** off14 setup = 0x0600a876 (cart 0x02206876), off16 =
  0x0600ac88. 0x02206876 is the game scene's OWN state machine (states 0..5+): state 0 inits the game
  DISPLAY (writes 0x06098f08 struct, 0x06090294 service) and waits for a panel START (buf 0x060987db==2,
  gated on 0x06098798 bits4/5 and 0x060987e8 bit2) to advance to state 5 (stage/spawn).

## 3. The trigger, FIRED (flag-gated STV_SCENEJUMP; faithful lever, NOT a 0x06098688 force-write)

The scene-director task = *0x060987B8 = the boot-controller's task node (060922E4 at runtime), the exact
pointer func_022016a0 and scene_director_build receive. scene_director_build reads its unk13 (+0xd,
signed, cart 0x02205b78 cmp/pz) and, if negative, forces the game scene. Added a flag-gated diagnostic
(tools/saturn-recomp/runtime/src/machine/bios.cpp) that sets `(*0x060987B8)->unk13 |= 0x80` once at
VB=STV_SCENEJUMP. This is the SAME field/lever the round-start path pulls into the scene director — it is
upstream of the game-scene build, unlike the proven-useless 0x06098688 force-write.

Result (STV_BOOTMODE=1 STV_SCENEJUMP=500, coins VB100/160):
- VB500: unk13 |= 0x80 set on task 060922E4.
- VB938 (next NATURAL scene advance): scene_director_build took the unk13<0 branch:
  **g_06098668 = 0x51 (81), g_0609866c = 0x06073FB0** — the GAME scene descriptor was built. VERIFIED.
- scene-81 setup **0x0600a876 ran** (COV) and off16 0x0600ac88 ran; attract_handler 0x06008868 ran.
- process_spawn_list 0x0602E5D0 ran.

## 4. Honest per-item

| item | result | evidence |
|---|---|---|
| scene-director mapped? | **YES** | state-4 call to 0x02205b20; scene table + graph parsed & verified |
| scene-director triggered? | **YES** | STV_SCENEJUMP -> unk13<0 -> branch taken at VB938 |
| g_0609866c built (game)? | **YES** | g_0609866c=0x06073FB0, g_06098668=0x51(81) |
| spawn-script ran? | **PARTIAL** | process_spawn_list 0x0602E5D0 ran, but found no player spawn-script (exits on null) |
| player actor spawned (0x0602Dxxx)? | **NO** | COV: 0x0602Dxxx region = 0 functions (never executed) |
| start-flag 0x06098688 raised? | **NO** | join FSM (0x0602dc00/dcac) never ran |
| morph fired? | **NO** | morph targets 0x06008afa/0x06009232 never ran |
| round began? | **NO** | black frame; boot machine soft-resets at VB1248 (~310 frames after entry) |

Frames: port/run/sj_frame/shot-{1000,1150,1240}.png and Mac scratchpad
`scenedirector_scene81_entry_vb1150.png` — BLACK (display setup done, no actors/stage; honest: NOT a
round, nothing faked).

## 5. THE NEW BLOCKER (two coupled walls)

1. **Direct jump skips stage setup.** Forcing unk13<0 lands on scene 81 and SKIPS scenes 73-80 (the
   credit-branch + character/stage-intro + stage-LOAD). Scene 81's state machine inits only the display,
   then (START not consumed through its gates, no stage data) times out and soft-resets after ~310
   frames. The player-actor spawn almost certainly happens in the SKIPPED setup scenes (76-80) and/or
   scene-81 state 5 — neither reached by a direct jump.
2. **The faithful path is blocked by the attract PARK at scene 3.** The attract walks 0->2->3 then PARKS
   at scene 3 (the combat demo) indefinitely (verified to 12000 VB): scene 3's tokenized script never
   signals completion (entity->unk3c bit0), so the director never advances past scene 3 to reach the
   credit-branch scene 73. (Scene 2 DOES complete at ~835 frames; scene 3 does not.) Also the
   credit-branch needs 0x06098674==2 && 0x06098675==2, but coin accrual drives 0x06098674 to 3 (P2 stays
   0) — the join bytes must be exactly 2 at the branch (the real player-join FSM sets them, see
   play_transition S3 + the now-decompiled join chain).

## 6. Revised estimate / next steps

The scene-director trigger is SOLVED and reproducible (STV_SCENEJUMP). To a real spawned player actor:
- (best, faithful) Make the attract advance past scene 3: investigate scene-3's script-completion
  (f_220fece interpreter of script 0x06036610) — if it is a frame timer, ~1-3 runs; if it waits on the
  demo-fight end (combat sim), deeper. Then with a credit present, the natural walk reaches scene 73,
  the credit-branch routes 74->..->80->81 WITH each scene's setup (stage load + spawn-script), and scene
  81 should find a player spawn-script -> 0x0602Dxxx spawns -> (downstream now decompiled) start-flag ->
  morph -> round. Still needs join bytes==2 at the branch (deliver the real join, not coin-count).
- (shortcut) Drive the game-entry SUB-SEQUENCE from scene 73/79 rather than a direct jump to 81, so
  scenes 73-80 run their setup. Less faithful (forces g_06098668) but would reveal whether 76-80 carry
  the player spawn-script; a good 1-run diagnostic for the next agent.
- Then: player-actor FSM family (0x0602Dxxx, downstream decompiled per coordinator) must actually
  spawn+step; and VDP2 RBG1 for a visible stage.
Estimate to a spawned player actor on the faithful path: mid-single-digit focused runs (the trigger
itself is done). A visible playable round additionally needs RBG1 (the known large render chunk).

## 7. Files touched / seeds / artifacts (isolated copy ~/decomp/dh-portA/)
- `tools/saturn-recomp/runtime/src/machine/bios.cpp` — NEW flag-gated diagnostic **STV_SCENEJUMP=VB**
  in stv_panel_read: sets (*0x060987B8)->unk13 |= 0x80 once at the given VBlank (the faithful
  scene-director game lever). Default off => baseline byte-identical.
- `notes/port/scene_director_status.md` — this file.
- NEW SEEDS: **none.** The scene-director + scene-81-setup code ran clean to the reset (no
  "not implemented"/PIC walls). The player-actor region (0x0602Dxxx) never executed, so its walls remain
  latent (none to seed yet — they appear only once an actor spawns).
- Frames: port/run/sj_frame/*.png + Mac scratchpad scenedirector_scene81_entry_vb1150.png (black; honest).
- Infra fixes made in the copy (the `--exclude 'run*/'` rsync dropped tools/saturn-recomp/runtime/):
  copied runtime/ from main and reconfigured recomp-build fresh with
  -DSATURN_RUNTIME=$HOME/decomp/dh-portA/tools/saturn-recomp/runtime (isolation). These are restores/
  build-dir, not source edits.
- NOT modified: project/, tools/decomp/, port/main_stv.cpp, port/seeds_extra.txt, notes/symbols.txt.

## 8. Reproduce (p620, in the copy)
```
cd ~/decomp/dh-portA/port
cmake --build recomp-build --target saturn_stv -j"$(nproc)"
# fire the scene-director trigger -> game scene 81 built at the next advance (VB938):
STV_BOOTMODE=1 STV_SCENEJUMP=500 STV_COVDUMP=1 ./recomp-build/saturn_stv --cart diehard_main.bin \
  --cartrom ../cart.bin --vblanks 2500 --stvinput "100:COIN,160:COIN" \
  --watch 06098668:06098678 --out run/sj2
grep "06098668 = 00000051" run/sj2/*        # scene index 81
grep -c "^0602d" run/sj2/cov.txt            # 0 = player actor still not spawned (the new blocker)
# attract parks at scene 3 (no trigger): watch the scene index stall at 3
STV_BOOTMODE=1 ./recomp-build/saturn_stv ... --vblanks 12000 --watch 06098668:06098678
```
