# Die Hard Arcade (ST-V) RECOMP - the scene content-task / off14 completion mechanism (attract reel)

Date: 2026-10-20. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Work in isolated copy ~/decomp/dh-portA/. Reference C read-only from main project/src_functional/.
Address map: runtime(HWRAM) = cart SH-2 + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.
Builds on: attract_to_credit_trigger.md (the scene-3 PARK), scene_director_status.md, drive_real_flow_status.md.

## TL;DR - the off14 content-task + completion mechanism is FULLY MAPPED and WORKS. The attract reel
## advances PAST scene 3: in steady attract it cascades 3 -> 4 -> 5 -> 6 -> 7 -> 8 (fades + demos,
## faithful per-scene durations) to a NEW wall at scene 8 (content seed-chain). The scene-3 "PARK" was
## NOT a broken completion mechanism - it is (a) a cold-boot transient the game`s own watchdog recovers
## from, and (b) with a CREDIT present, the "2P-armed" flag 0x06098670 stays =1 and gates the transition
## timeline OFF, which makes the park PERMANENT (what attract_to_credit_trigger.md saw - it used coins).

## 1. The mechanism, end-to-end (all decompiled / --watch-confirmed)

### 1a. off14 IS installed as a per-frame content task - by f_220fece after all.
scene_director_build (cart 0x02205b20) tail-calls f_220fece(task, *(cur+20)) where *(cur+20)=off14.
The shipped decomp project/src_functional/220fece.c says f_220fece "ignores off14" - that decomp is
WRONG/incomplete. The raw asm (m2c_fn 0x20fece) shows f_220fece forwards its INCOMING r4=task, r5=off14
to the allocator func_06013c3c(task, off14) BEFORE overwriting r5/r4 (the r5=1,r6=0x6c,r4=alloc happen
only for the SECOND call, the zero-init). So:
    f_220fece(task, off14): node = func_06013c3c(task, off14);  // installs a task node, handler=off14,
                                                                // node->unk14 = director task
                            func_06018bb8(node,1,0x6c);          // zero-init 0x6c body
Confirmed live (--watch on the task heap): at scene-3 entry (VB1038) a task node at 0x060923C4 gets
its handler (+0) = 0x06036610 (scene-3 off14) and its unk14 (+0x14) = 0x06092304 (the director task),
written from the f_220fece alloc chain (pr 06013ED8). Scene 2 uses the same path (node 0x060923A4,
handler 0x0601F080). So f_220fece IS the "off14-script -> install+tick content task" installer.
=> ACTION FOR MAIN DECOMP: fix 220fece.c - the two params are forwarded to func_06013c3c, not ignored.

### 1b. The content task is a per-scene state machine; completion = set director_entity unk3c bit0.
off14 0x06036610 = f_232610 (cart 0x232610) is the content handler for ALL 21 fade/transition scenes
(3,5,7,9,15,17,19,21,23,25,32,34,...). It is a task handler (arg0=task; task->unk8=content entity;
task->unk10=state byte; task->unk14=director task). States 0..5. In its completion states (4/5) it
does:  task->unk14->unk8->unk3C |= 1;   i.e. sets the DIRECTOR entity`s unk3c bit0 - the exact
"advance now" bit scene_director_build waits on. Confirmed: f_232610 RUNS during scene 3 (COVDUMP:
rt 0x06036610 ran=1), reaches state 2, and STALLS there (task state byte 0x060923D4 = 2 at VB1040,
never changes).

### 1c. Why state 2 stalls - it waits for the content entity`s "animation complete" bit3.
f_232610 state 2 (block at cart 0x2328d0) each frame calls three steppers (0x22a8d8 join/mode check,
0x216d58 fade-sprite draw, 0x243628 graphics) then tests  content_entity->unk3C & 8 (bit3). It leaves
state 2 only when EITHER bit3 is set (-> state 3 -> a unk50 frame-timer -> state 4 -> director bit0,
the NORMAL attract fade-out), OR the resident handler *g_06098678 has morphed to round_controller_dual
(0x06009232) / scene_run_handler (0x06009812) (the PLAY-enter path - never in attract). Confirmed live:
content entity (0x060E1620) unk3c (0x060E165C) is set to 0 at scene start and NEVER gets bit3.

### 1d. Who sets content-entity bit3 = the transition-timeline interpreter f_2463c8.
There is exactly ONE bit3 setter in the whole image: cart 0x246658 (rt 0x0604a658), inside f_2463c8
(cart 0x243628 subtree, entry cart 0x2463c8 / rt 0x0604a3c8). f_2463c8 is the scene`s TRANSITION
TIMELINE interpreter: it walks a scripted spawn/timeline (cursor 0x060973E0..F0, driven by entity
unk40 frame-timers), and when the script reaches its end it does  (record->unk14)->unk8->unk3C |= 8
(set content-entity bit3) and frees the record. So the completion CHAIN is:
    timeline f_2463c8 runs to end-of-script -> content_entity unk3c bit3 -> f_232610 state2->3->4
    -> director_entity unk3c bit0 -> scene_director_build advances to the next scene.

## 2. The scene-3 gap - the EXACT unmet condition (pinned with --watch)

f_2463c8 begins with  switch (0x06098564) { case 0: return; ... }  - if the word 0x06098564 == 0 it
early-returns and the timeline NEVER advances. 0x06098564 is written EVERY FRAME by f_22a8d8 (the
first state-2 stepper, cart 0x22a8d8): it sets 0x06098564 = 0, then ONLY if the "2 players armed"
flag 0x06098670 == 0 does it set 0x06098564 |= 3 (both player channels = pure attract); otherwise it
uses the join bytes 0x06098674/75 (nonzero only if a join==2). 
--watch proof (scene 3): 0x06098564 is held at 0 every frame (writer fn 0602E8D8 = f_22a8d8), because
0x06098670 == 1 and the join bytes are not 2. The fade level 0x060B3D58 is set once (=6, scene-3
descriptor +0x24) and never decrements. => content-entity bit3 never set => f_232610 parks in state 2.

### Root of 0x06098670 == 1
0x06098670 is semantically "2 players armed" (scene_director_build case-4 selector). It is set to 1 at
VB201 by f_201cae (cart 0x201cae), in a block guarded by (0x06000798 >> 5) & bit  AND 0x0609880c==1.
0x06000798 is the ST-V DIP/config word; the harness logs it as "0x06000798 = 0x60 [cold-boot default]"
(bit5 proceed). So the cold-boot config makes f_201cae ARM the flag during attract. In PURE attract
(no credit) the game`s own watchdog clears 0x06098670 to 0 at ~VB5167 (see sec 3), after which the
reel runs. With a CREDIT inserted the flag stays armed -> PERMANENT park at scene 3 (the earlier note).

## 3. Result - the reel ADVANCES past scene 3 (A), with two caveats

CONTROL (no diagnostics, no coins, STV_BOOTMODE=1), scene index 0x06098668 to 20000 VB:
    0 -> 2 (VB203) -> 3 (VB1038)  [cold-boot PARK]
    -> 81 (VB5169)  [watchdog] -> reset -> 1 -> 2 (VB5479)   [0x06098670 cleared to 0 at VB5167]
    -> 3 (VB7329) -> 4 (VB7830) -> 5 (VB9083) -> 6 (VB9662) -> 7 (VB11512) -> 8 (VB12013)
    -> FATAL "call to 060372D4, not an entry" (scene-8 content handler not seeded)
Durations are faithful: fades (odd, off14 0x06036610) ~500 frames, demos (even, off14 0x0601f080)
~1250-1850 frames, correctly alternating. The 4 cascade frames (scenes 4-7) differ (distinct md5) -
real visual progression, RBG1 backdrop + scene content.

Caveat 1 (cold-boot transient): the FIRST scene-3 (loop 1) parks regardless of 0x06098670 and is only
recovered by the watchdog cycle into steady attract. A separate cold-boot condition, not yet isolated.
Caveat 2 (credit-armed gate): WITH coins the park is PERMANENT (0x06098670 stays 1). The faithful fix
is upstream - 0x06098670 must be 0 during attract even with a credit; that traces into f_201cae /
the DIP word 0x06000798 bit5 / 0x0609880c (credit state). NOT yet resolved to a one-line fix.

### Scene-8+ is a content seed-chain (new walls, onion-peeled this session)
Seeding the scene-8 content handlers one at a time (relearn_portA.sh) advances scene-8 setup each time:
    060372D4 (f_2332d4, scene-8 content) -> next wall 0604A282 -> next 060144F6 -> next 060407BA ...
All at VB12013 (inside scene-8 setup). Scene 8 is a demo scene whose content spawns many sub-objects,
each a handler to seed - the "deep content chain" attract_to_credit_trigger.md predicted. Three seeds
added this session (see sec 5); the current wall is 060407BA (cart 0x02240 7ba) from pr 06034716.

## 4. HONEST verdict: (A) with two named further pieces.
(A) The off14/content-task/completion mechanism WORKS and the attract reel advances past scene 3,
    cascading 3->4->5->6->7->8 in steady attract. No completion bit was forced (STV_ATTRACTARM only
    clears the arm-flag; and the control run cascades WITHOUT it).
Further piece 1 (to make it robust / unblock the COIN path): make 0x06098670 correctly 0 during attract
    with a credit present - decompile f_201cae / the credit state around 0x0609880c / DIP 0x06000798.
Further piece 2 (to continue toward scene 73 / credit UI): seed the scene-8+ demo content handler chain
    (starts 060372D4, 0604A282, 060144F6, 060407BA, ...) - routine but numerous.

## 5. Files changed this session (isolated copy ~/decomp/dh-portA/) - EXACT list
MODIFIED (source):
 - tools/saturn-recomp/runtime/src/machine/bios.cpp  - NEW flag-gated diagnostic STV_ATTRACTARM
   (default OFF): when set, holds 0x06098670 = 0 each frame (the faithful attract "2P-not-armed" value).
   Confirms the timeline gate and unblocks the COIN-case permanent park (park -> watchdog recovery ->
   reel cascade). Default-off => baseline byte-identical. It is NOT a force of the director completion
   bit (unk3c bit0); it only restores the arm-flag the real completion path reads.
 - port/seeds_extra.txt  - 3 NEW learned seeds for the scene-8 content chain:
     060372D4 (f_2332d4, scene-8 content handler), 0604A282, 060144F6  (+ dated comment).
REGENERATED (build artifacts, no manual edit): port/recomp/* (relearn_portA.sh), port/recomp-build/*.
NEW (this report): notes/port/scene_content_completion.md.
NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp, the other runtime sources.
CURRENT OPEN WALL (next seed): 060407BA (cart 0x022407ba), reached during scene-8 content setup.

## 6. Frames (honest; attract reel ADVANCING, RBG1 backdrop + scene content; nothing faked)
 - port/run/s3frames/shot-{8400,9400,10500,11800}.png  = scenes 4,5,6,7 (distinct md5 = real progression)
 - Mac scratchpad: attract_reel_scene{4,5,6,7}_vb{8400,9400,10500,11800}.png
 - port/run/s3casc/ , run/s8/ , run/s8b/ , run/s8c/ (logs); run/s3coin20k (coin permanent-park control)

## 7. Reproduce (p620, in the copy)
```
cd ~/decomp/dh-portA/port
# steady-attract reel advances past scene 3 (control, NO diagnostics, NO coins):
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 20000 --out run/chk --watch 06098668:0609866c
grep "06098668 =" run/chk/log.txt   # 3(1038)->81(5169)->reset->3(7329)->4->5->6->7->8(12013)->wall 060372D4
# WITH coins = permanent park at scene 3 (0x06098670 stays armed):
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 20000 \
  --out run/coin --watch 06098668:0609866c --stvinput "200:COIN,260:COIN"   # scene stuck at 3
# diagnostic: clear the 2P-armed flag so the coin case also recovers + cascades:
STV_BOOTMODE=1 STV_ATTRACTARM=1 ./recomp-build/saturn_stv ... --stvinput "200:COIN,260:COIN"
```
