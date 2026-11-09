# Die Hard Arcade (ST-V) RECOMP - the COIN-path arm fix (0x06098670) + pushing the attract reel

Date: 2026-10-20. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Work in isolated copy ~/decomp/dh-portA/. Reference C read-only from main project/src_functional/.
Address map: runtime(HWRAM) = cart SH-2 + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.
  => cart.bin offset = runtime - 0x05E04000  (e.g. f_201cae = runtime 0x06005cae = cart.bin 0x201cae).
Builds on: scene_content_completion.md (off14/content-completion mechanism; the scene-3 PARK).

## TL;DR - PIECE 1 SOLVED, FAITHFULLY (no force). The coin-path permanent park at scene 3 was NOT a
## game-logic gap - it was a HARNESS bug in deliver_coin (bios.cpp). deliver_coin latched the
## coin-pending flag 0x0609880C=1 unconditionally (and, separate bug, wrote 0x0609880C for BOTH
## players). The REAL coin-detect fn (cart 0x20faf8 / runtime 0x060065f0) only raises the pending
## flag when the per-slot credit byte (0x06098674 P1 / 0x06098675 P2) is 0 or 3 AND the flag is not
## already set. Without that gate a 2nd coin re-raised the pending flag in a credit state the real
## coin-detect never would, corrupting the credit/arm bookkeeping so the game's own scene-81
## reset-timeout (which clears 0x06098670) never fired -> permanent park. Restoring the gate makes
## the coin path self-recover: a credited run now cascades 2->3->4->5->6->7->8 exactly like the
## no-coin control. The 2P-armed flag 0x06098670 is set (VB201, f_201cae) and cleared (VB5167,
## scene-81 reset) entirely by the GAME's own code - the harness never touches it.

## 1. What 0x06098670 ("2P-armed") really is, and where it is set/cleared (all --watch confirmed)

f_201cae (cart 0x201cae / runtime 0x06005cae) is the boot/attract scene sequencer. Decompiled in full:
its tail block (the "arm block") runs every frame it is scheduled and does:

    if (((0x06000798 >> 5) & 1) && 0x06098668 != 0) {        // DIP bit5 set AND scene index != 0
        if (0x0609880C == 1) { director->unk3C |= 1; ...; 0x06098670 = 1; }   // P1 coin pending
        if (0x06098810 == 1) { director->unk3C |= 1; ...; 0x06098670 = 1; }   // P2 coin pending
    }

So 0x06098670 is armed iff a coin-pending flag (0x0609880C / 0x06098810) is 1 at the moment f_201cae
runs. 0x06098670 then gates the attract transition timeline OFF (via f_22a8d8, which only sets the
timeline word 0x06098564 when 0x06098670==0) -> the current scene's fade never completes -> PARK.

Live --watch, 2-coin run "200:COIN,260:COIN":
 - VB201: 0x06098670 = 1  (f_201cae, pr 06005EA8) because the VB200 coin set 0x0609880C=1.
 - no-coin run: 0x06098670 is NEVER set; reel cascades 2->3(1860)->4->5->6->7->8 with no park, no reset.

So the arm flag is purely a function of coin-pending state. It is NOT a cold-boot transient (the earlier
"cold-boot park" in scene_content_completion.md sec 3 was actually caused by the DEFAULT --stvinput
"200:COIN,340:START1" - an empty --stvinput falls back to that default in main_stv.cpp, so the "control"
there had a coin at VB200 after all). A genuinely coinless run has 0x06098670==0 throughout.

## 2. Who clears it - the game's own scene-81 reset-timeout (NOT a watchdog we add)

The only clear of 0x06098670 in attract is the scene-81 reset/reinit routine (cart ~0x232b32, inside
the subtree attributed to fn 0603426A). On a park timeout the director jumps to scene 0x51 (81), whose
reinit does  0x06098670=0; 0x0609880E=0; 0x06098812=0; 0x0609856c=0; ...  and reboots the reel. Live:
 - single coin (early VB200 OR late VB2500): park at scene 3 -> scene 81 (reset) ~4800 VB later ->
   reboot -> reel cascades 2->3->4->5->6->7->8. SELF-RECOVERS.
 - TWO coins (200,260): the reset-timeout NEVER fires -> permanent park. <- the bug the mission hit.

The difference between 1 coin (recovers) and 2 coins (permanent) is the credit bookkeeping, which the
harness was corrupting. See sec 3.

## 3. Root cause = HARNESS deliver_coin, and the real coin-detect's gate

The real coin-switch -> pending-flag step is fn f_coindetect @cart 0x20faf8 (runtime 0x060065f0). It is
NOT scheduled in the boot-shortcut state the harness lands in (the 0x664 BIOS coin service is unpolled),
so bios.cpp substitutes it with deliver_coin. Decompiled, the real coin-detect does (per slot):

    if (coin_slot_enabled(DIP)                              // P1: r13 & 0x10 ; P2: r13 & 0x20
        && (credit_byte == 0 || credit_byte == 3)          // 0x06098674 (P1) / 0x06098675 (P2)
        && pending_flag == 0                               // (P2 path explicitly; don't re-raise)
        && BIOS_coin_read(slot) != 0)                      // momentary switch (0x06000664)
        pending_flag = 1;                                  // 0x0609880C (P1) / 0x06098810 (P2)

The OLD deliver_coin ignored ALL of this: it did `st16(0x0609880C, 1)` unconditionally, for every coin,
for BOTH players (P2 coins wrongly wrote 0x0609880C too). So the 2nd coin (VB260), arriving when the
credit byte was already 2 (= neither 0 nor 3), re-latched the pending flag - a state the real coin-detect
would have refused. That extra latch corrupts the credit/arm state enough that the scene-81 reset-timeout
is suppressed, and 0x06098670 stays armed forever.

### THE FIX (bios.cpp, deliver_coin) - replicate the real coin-detect gate
deliver_coin now: pend = (player? 0x06098810 : 0x0609880C); credit = (player? 0x06098675 : 0x06098674);
raise pend=1 ONLY if (ld16(pend)==0 && (credit==0 || credit==3)). Otherwise log "ignored by coin-detect
gate". This is faithful - it is exactly the real coin-detect's own guard, not a new heuristic.

## 4. RESULT (PIECE 1) - the coin path now cascades, faithfully, no force

2-coin run "200:COIN,260:COIN", STV_BOOTMODE=1, NO diagnostics:
    coin1 VB200 accepted (credit byte 0) ; coin2 VB260 IGNORED by gate (credit byte 2)
    scene: 2->3(1038, armed park) -> 81(5169 reset) -> reboot -> 2(5479)->3(7329)->4(7830)
           ->5(9083)->6(9662)->7(11512)->8(12013)  == identical to the no-coin control.
    0x06098670: set 1 @VB201 (f_201cae), cleared 0 @VB5167 (scene-81 reset) - BOTH by game code.
No completion bit forced; STV_ATTRACTARM NOT used (it is now superseded for the coin path - the real
logic lives in deliver_coin's gate). The STV_ATTRACTARM diagnostic is left in place, default-off and
byte-identical, and its comment now points here.

HONEST NOTE on faithfulness: the game's response to a credit during attract is to (a) restart the reel
to scene 2 and (b) arm, which PARKS the reel at scene 3 for ~4800 VB (a "credit present / waiting"
hold) before the reset-timeout resumes the demo. Whether real DHA overlays "CREDIT / PRESS START" on a
*running* demo instead of this park-then-resume loop is not verifiable from the port; but the set/clear
of 0x06098670 and the park->reset->cascade are 100% the game's own code. The harness fix only stops the
spurious extra coin latch. ==> ACTION FOR MAIN DECOMP: none in-cart; this is a harness-faithfulness fix.

## 5. PIECE 2 - pushing the reel past scene 8 (autolearn seed chain) - FULL DEMO LOOP reached

autolearn_scene.sh (AL_VB=14000, inert timeline) seeded the scene-8 content chain to completion in
7 iterations, then STOPPED with "no learnable non-entry" (the run reaches 14000 VB with NO more
"call to ADDR, not an entry" walls). 7 NEW seeds (appended to port/seeds_extra.txt, for main merge):
    060407BA  06035336  06034E34  06021302  06022256  0601D984  06006DDA
  (scene-8+ content/sub-task handlers; cart = runtime - 0x05E04000.)

RESULT - scene-index trace (inert, no-coin; identical with coins after the sec-4 recovery):
    2(10) -> 3(1860) -> 4(2361) -> 5(3614) -> 6(4193) -> 7(6043) -> 8(6544) -> 9(7797) -> 10(8298)
    -> 2(8625) -> 3 -> 4 -> 5 -> 6 -> 7 -> 8(15159) -> ...
So the attract DEMO REEL is now a COMPLETE SELF-SUSTAINING LOOP: scenes 2..10 then back to 2,
repeating indefinitely. No walls, no park, with OR without coins. This is the whole attract demo.

HONEST on reach vs scene 73 / a round: the demo reel LOOPS at scene 10 -> 2 (scene 10's selector
returns to 2). It does NOT climb toward scene 73 (the credit branch) or into a round via the demo
path. That is expected and consistent with attract_to_credit_trigger.md: scene 73 and the round are
reached by the game-ENTER route (credit -> START -> player-actor spawn FSM 0x0602DCAC -> round),
NOT by the demo reel. Reaching a round therefore still requires the START/actor-spawn path (the known
open cycle), which is OUT of scope for this coin-arm/reel task. The milestone here is: the attract
reel is now whole and the coin path no longer breaks it.

RENDERING (noted, not chased - known separate gap): the advancing demo-fight scenes (9/10) render the
combat HUD correctly (two character portraits + green/magenta health bars) but the playfield sprites
are garbled (scanline noise). Backdrops/HUD OK; sprite/BG layer is the known separate rendering gap.

## 5b. Frames (honest; attract reel ADVANCING; HUD renders, sprite/BG layer garbled = known gap)
port/run/ (p620):
 - run/reelframes/shot-{2700,4000,4500,6300,7100,8050,8500}.png  = no-coin reel scenes 4,5,6,7,8,9,10
   (all distinct md5 = real progression; 7100/8050/8500 are the NEW scenes 8/9/10 from the seed chain)
 - run/coinframes/shot-{7500,11700,12300}.png  = 2-coin path: 7500==11700 (frozen scene-3 credit-wait),
   12300 distinct (scene 8, post-recovery cascade) -> proves the coin path recovers and cascades
Mac scratchpad (/private/tmp/.../scratchpad/):
 - reel_scene4_vb2700.png, reel_scene7_vb6300.png, reel_scene8_vb7100.png, reel_scene9_vb8050.png,
   reel_scene10_vb8500.png, coin_scene3_park_vb7500.png, coin_scene8_cascade_vb12300.png

## 6. Files changed this session (isolated copy ~/decomp/dh-portA/) - EXACT list
MODIFIED (source):
 - tools/saturn-recomp/runtime/src/machine/bios.cpp  - deliver_coin rewritten to apply the real
   coin-detect credit-byte gate (and correct per-player pending flag). (+ STV_ATTRACTARM comment note.)
 - port/seeds_extra.txt  - NEW scene-8+ content-chain seeds from autolearn (see sec 5; note for main merge).
REGENERATED (build artifacts, no manual edit): port/recomp/*, port/recomp-build/* (relearn_portA.sh).
NEW (this report): notes/port/coin_arm_and_reel.md.
NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp.

## 7. Reproduce (p620, in the copy)
```
cd ~/decomp/dh-portA/port
# coin path now cascades (NO diagnostics, 2 coins):
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 16000 --out run/fix2coin --watch 06098668:0609866a --stvinput "200:COIN,260:COIN"
grep "06098668 =" run/fix2coin/log.txt   # 3->81(reset)->3->4->5->6->7->8  (same as no-coin)
# no-coin control (byte-identical path, no deliver_coin): cascades cleanly, 0x06098670 never set:
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 14000 --out run/nc --watch 06098668:0609866a --stvinput "9000:SERVICE"
```
