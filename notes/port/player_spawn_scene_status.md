# Die Hard Arcade (ST-V) RECOMP — locating the player-actor spawn (setup scenes 73-80 -> scene 81)

Date: 2026-10-19. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Builds on: scene_director_status.md, spawn_diagnosis.md, stv_panel_input_status.md.
Work done in isolated copy ~/decomp/dh-portA/ (reconcile into main tree).
Address map: runtime(HWRAM) = cart SH-2 addr + 0x03E04000; cart SH-2 addr = cart.bin offset + 0x02000000.

## TL;DR

The player-actor spawn is **NOT installed by any of the setup scenes 73-80.** Walking the
scene-director faithfully through the full game-enter sub-sequence **72->73->74->75->76->79->80->81**
runs CLEAN (after 3 new PIC seeds) but **never spawns the player actor (0x0602Dxxx = 0), never raises
the start-flag 0x06098688, and never runs process_spawn_list with a non-null script.** The scenes do
credit/character/stage-intro setup only.

The spawn lives in **scene 81 (GAME), state 5** (code at cart 0x02206c40 = rt 0x0600AC40), reached only
when the scene-81 state machine (cart 0x02206876 = rt 0x0600A876) accepts a **START** through its start
gate. That gate is **closed in the harness**: it requires the cooked control word **0x06098798 bits 4/5**
to be set (they never are) PLUS a START byte in the panel buffer (0x060987DB==2). With the gate forced
open I confirmed scene 81 *does* poll the panel from its own state machine (svc 0x06000660 from pr
0x0600A8A2) and *does* start into state-5 code, but it then **soft-resets** instead of spawning — the
forced path reaches state 5 without the real preconditions (proper character/stage state + a legitimate
start-enable). The 0x0602Dxxx functions that execute in the long run are **attract-demo actors after the
reset, NOT a spawned player** (verified: 0x0602Dxxx=0 through VB1610; they only run later in the demo
fight). No spawn was faked.

Honest verdict: spawn located (scene 81 state 5); the remaining faithful requirement is the
**start-enable flag 0x06098798 bits 4/5 + a live panel START** — the same ST-V panel-format gap already
flagged in spawn_diagnosis/stv_panel_input_status, now pinned to the exact scene-81 gate.

## 1. New diagnostic tooling (flag-gated, default off => byte-identical baseline)

The previous driver (STV_SCENEJUMP) lives in stv_panel_read, but **the ST-V panel service 0x06000660 is
NOT scheduled every frame once the game-enter scenes run** (measured: it stops being called ~VB542 in the
forced walk). So a panel-read hook cannot drive the chain. I added an **always-on per-frame driver**
called from the runtime VBlank tick:

- `tools/saturn-recomp/runtime/src/video/video.cpp` — `vblank_in()` now calls `stv_scene_probe_tick(g_vblanks)`
  right after `++g_vblanks` (runs EVERY frame, independent of the game state).
- `tools/saturn-recomp/runtime/src/machine/bios.cpp` — new `stv_scene_probe_tick(uint64_t vb)` with knobs:
  - `STV_SCENESEED="vb:idx"`  ONCE at VB>=vb: g_06098668=idx, g_0609866c=0x06072660+idx*0x50 (place current scene)
  - `STV_SCENEADV="vb:count:intv"`  from VB>=vb, every intv frames, force 1 director advance (set director-entity->unk3c bit0), count times
  - `STV_SCENEPIN="idx"`  every frame: pin g_06098668/g_0609866c to idx
  - `STV_SCENEJOIN="startvb"`  from VB>=startvb: g_06098674=g_06098675=2 (feed the scene-73 credit-branch -> 74). MUST be gated past boot; forcing it from VB0 diverges early boot.
  - `STV_SCENECHAR="v"`  every frame: g_0609856c=v (feed the scene-75 branch -> 76+(v&3))
  - `STV_SCENEGATE="startvb"`  from VB>=startvb: 0x06098798 |= 0x30 (force the scene-81 START gate open) [DIAGNOSTIC]
  (The old STV_SCENESEED/etc. block was moved out of stv_panel_read into this always-on function.)
- Isolation build helpers (dh-portA only; the repo's relearn.sh/autolearn.sh point at the MAIN tree):
  `port/relearn_portA.sh` (regen recomp from notes/symbols.txt + port/seeds_extra.txt into dh-portA),
  `port/autolearn_scene.sh` (inherits STV_* env, grinds "call to ADDR, not an entry" walls into dh-portA).

## 2. The scene graph (verified from the table @0x06072660, stride 0x50)

    72: sel 0  -> 73
    73: sel 3  -> (g_06098674==2 && g_06098675==2) ? 74 : 79    (credit/join branch)
    74: sel 0  -> 75
    75: sel 2  -> 76 + (g_0609856c & 3)                         (character branch; 76/77/78)
    76/77/78: sel 0 -> 79
    79: sel 0  -> 80
    80: sel 0  -> 81
    81: GAME (handled specially; its state machine = cart 0x02206876)
  Scene off14 "scripts": 73/74=0x06022AC0, 75=0x06036BC4, 76/77/78=0x06023240, 79=0x060232BC,
  80=0x06023532, 81=0x0600A876. NOTE: the real f_220fece (tail of scene_director_build) allocates a
  generic 0x6C object and IGNORES *(cur+20) (verified by disasm) -- scene behaviour is driven by handlers
  that read g_06098668/g_0609866c, not by an off14 call. The port decomp is faithful here.

## 3. The faithful walk runs clean; 3 new PIC seeds

Walking 72->81 with per-frame forced advances (STV_SCENESEED=500:72 STV_SCENEJOIN=600 STV_SCENECHAR=0
STV_SCENEADV=540:7:180) and autolearn surfaced exactly **3** new walls, all genuine game functions:
  - **06032B84**, **06032BA8** — scene 73's credit/join setup (called from the spawn family, pr 0x0602E9E4)
  - **060246AE** — scene 76's character/stage-intro setup (via task_list_run, pr 0x06013E0E)
After seeding, the full chain runs clean to scene 81 with no further walls.
(The per-scene "jump straight to scene N" probes faulted on 060189B6 / 06023952 etc., but those are
ARTIFACTS of skipping the prerequisite scenes -- they vanish on the faithful chain. Do NOT seed them.)

## 4. Per-item results (faithful chain, no input)

| scene active | 0x0602Dxxx exec | process_spawn_list non-null | start-flag 0x06098688 | new walls |
|---|---|---|---|---|
| 73 | no | no | no | 06032B84, 06032BA8 |
| 74 | no | no | no | - |
| 75 | no | no | no | - |
| 76 | no | no | no | 060246AE |
| 79 | no | no | no | - |
| 80 | no | no | no | - |
| 81 (state 0) | no | no (runs, exits on null) | no | - (soft-resets ~180 frames after entry) |

So no setup scene installs an executing player spawn-script. process_spawn_list (0x0602E5D0) runs but
always finds a null script (ctx+0x30) and exits -- throughout the chain.

## 5. Scene-81 state machine (cart 0x02206876) -- the start gate, disassembled

    r14 = 0x06098794 (control struct);  [r14+4] = word 0x06098798 (cooked control flags)
    r12 = task;  r11 = *(task+8) = entity;  r8 = 5
    tst (0x06098798 & 0x30); bt/s -> dispatch              ; if bits 4/5 CLEAR, SKIP the panel poll
    (gate open) jsr *(0x06000660)(r4=0x060987D8)           ; poll panel into the buffer
    if (0x060987E8 & 4) {                                   ; (bit2 selects sub-path)
        if (0x06098798 & 0x10 && byte[0x060987DB]==2) task[0x10]=5   ; P1 START -> STATE 5
        if (0x06098798 & 0x20 && byte[0x060987DF]==2) task[0x10]=5   ; P2 START -> STATE 5
    } else if (0x06098798 & 0x30 && byte[0x060987DB]==2) task[0x10]=5
    dispatch on task[0x10]: 0->init  1->0x2206b1e  2->0x2206b6e  3->0x2206bae  4->0x2206ae0
                            5->0x2206c40 (STAGE/PLAYER SPAWN)  default->0x2206c6a

So **state 5 (spawn) is entered only when 0x06098798 bits 4/5 are set AND a START byte is in the panel
buffer.** In the harness 0x06098798 is cleared to 0 every frame (fn 0x0601328E) and bits 4/5 are never
set, so the gate never opens, the panel is never polled, state 0 times out, and the boot machine
soft-resets (pr 0x06005BAC).

## 6. Forcing the gate (diagnostic) -- reaches state 5 but resets, does NOT spawn

STV_SCENEGATE=1400 (force 0x06098798|=0x30) + dense START (every 2 VB across the scene-81 window):
- CONFIRMED scene 81 polls the panel from its OWN state machine: `STVBIOS svc 06000660 ... from pr 0600A8A2 VB 1462`.
- scene 81 starts into state-5 region but then the **boot machine soft-resets at VB1465** (pr 0x06005BAC) instead of spawning.
- The 0x0602Dxxx functions that run afterwards (0602D34A/3B8/644/688) are **attract-demo actors** from the
  post-reset attract (scenes 3/4 at VB1620/2121), NOT a player spawn -- proven: 0x0602Dxxx=0 at VB<=1610,
  >0 only once the demo fight runs. State-5 spawn code 0x0600AC40 was never actually executed.
Why it resets: the forced path forces the gate bits (racing fn 0x0601328E which clears them) and skips the
real character/stage selection state, so state 5 sees invalid preconditions and bails. This is a forcing
artifact, not a faithful spawn -- flagged, not faked.

## 7. The remaining faithful requirement (X) and revised estimate

To spawn the player faithfully:
1. **Set 0x06098798 bits 4/5 legitimately** (the "start-enable" flag). It is field +4 of the control
   struct at 0x06098794 (48 code refs). The game's panel decoder derives these cooked bits from the raw
   ST-V panel buffer 0x060987D8; in the harness the raw->cooked mapping that yields bits 4/5 is unresolved
   (the ST-V panel-format gap from stv_panel_input_status / spawn_diagnosis). Find what sets them (credit-
   present / attract-complete) and reproduce it, OR resolve the panel decoder so a START press sets them.
2. **Deliver START through a live panel poll** while the gate is open (scene 81 does call 0x06000660 when
   the gate is open, so a real --stvinput START will be consumed once bits 4/5 are genuinely set).
3. Then scene 81 -> state 5 (0x0600AC40) should install/run the player spawn-script -> 0x0602Dxxx spawns
   -> start-flag 0x06098688 (set by the player-actor FSM 0x0602DCAC) -> morph -> round.

Revised estimate: the spawn location is now pinned (scene 81 state 5). The blocker is the start-enable
flag 0x06098798 bits 4/5, i.e. the panel-decode / credit-ready path -- a focused mid-single-digit effort
to resolve that one cooked-flag source. A visible round additionally needs VDP2 RBG1 (known large chunk).

## 8. Files touched (isolated copy ~/decomp/dh-portA/) -- EXACT list for reconciliation

MODIFIED (source -- reconcile into main tree):
- tools/saturn-recomp/runtime/src/machine/bios.cpp  (new stv_scene_probe_tick() + STV_SCENESEED/SCENEADV/SCENEPIN/SCENEJOIN/SCENECHAR/SCENEGATE; moved out of stv_panel_read)
- tools/saturn-recomp/runtime/src/video/video.cpp  (vblank_in calls stv_scene_probe_tick every frame)
- port/seeds_extra.txt  (+3 seeds: 06032B84, 06032BA8, 060246AE)

NEW (isolation helpers; main tree already has its own relearn.sh/autolearn.sh):
- port/relearn_portA.sh, port/autolearn_scene.sh

REGENERATED (build artifact from the +3 seeds; produced by relearn_portA.sh, no manual edit):
- port/recomp/*  (regenerate in main with its relearn.sh after merging the 3 seeds)

NEW (this report):
- notes/port/player_spawn_scene_status.md

NEW SEEDS for merge into main port/seeds_extra.txt:
    06032B84   # scene 73 credit/join setup (spawn family)
    06032BA8   # scene 73 credit/join setup
    060246AE   # scene 76 character/stage-intro setup

NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp.

## 9. Reproduce (p620, in the copy)

    cd ~/decomp/dh-portA/port && cmake --build recomp-build --target saturn_stv -j"$(nproc)"
    # faithful game-enter walk 72->81 (clean, no spawn):
    STV_BOOTMODE=1 STV_SCENESEED="500:72" STV_SCENEJOIN=600 STV_SCENECHAR=0 STV_SCENEADV="540:7:180" \
      STV_COVDUMP=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
      --vblanks 3500 --stvinput "100:COIN,160:COIN" --watch 06098668:06098690 --out run/chain
    grep -ic "^0602d" run/chain/cov.txt        # 0 = no player actor on the faithful chain
    # force the scene-81 start gate + START (reaches state 5, then resets -- NOT a faithful spawn):
    STARTS=$(python3 -c "print(,.join(%d:START1%v for v in range(1445,1621,2)))")
    STV_BOOTMODE=1 STV_SCENESEED="500:72" STV_SCENEJOIN=600 STV_SCENECHAR=0 STV_SCENEADV="540:7:180" \
      STV_SCENEGATE=1400 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
      --vblanks 2400 --stvinput "100:COIN,160:COIN,$STARTS" --out run/gate
    # grind new walls on a probe env (isolated): export the STV_* env then ./autolearn_scene.sh

## 10. Frames (honest; RBG1 unimplemented so playfield is black)
- port/run/frames/scene81_faithful_displayinit_vb1500.png -- faithful chain, scene 81 active (black display-init, no actors).
- port/run/frames/attract_demo_actors_0602D_vb2200.png -- post-reset ATTRACT DEMO where 0x0602Dxxx actors run (demo, NOT a spawned round).
- Mac scratchpad: same two files (scene81_faithful_displayinit_vb1500.png, attract_demo_actors_0602D_vb2200.png).
