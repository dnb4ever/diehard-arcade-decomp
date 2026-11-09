# Die Hard Arcade (ST-V) RECOMP — the credited attract->play transition + P1-actor spawn

Date: 2026-10-19. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Builds on: credited_round_status.md, stv_panel_input_status.md, boottask_status.md.
Address map: runtime(HWRAM) = cart SH-2 addr + 0x03E04000; cart SH-2 addr = cart.bin offset + 0x02000000.
cart.bin is MIRRORED (0x220xxxx == 0x230xxxx); all cart addrs below use the 0x220xxxx bank.

## TL;DR — the knot is MAPPED end-to-end, and it is a genuine chicken-and-egg across the
## un-decompiled game-scene / player-actor subsystem. Verdict: (c), a real slog. Evidence below.

The attract->play transition is NOT a single missing flag. It is a dependency cycle in the
currently-reachable code, broken on real hardware by a "coin -> press START -> build game scene ->
spawn player actor" subsystem that the harness never enters and that is almost entirely
un-decompiled (the decomp has 364 of several thousand functions; the whole player-actor family is
absent). No input/flag injection I or the two prior port agents tried reaches it.

## 1. The dispatch table 0x06090304 — RESOLVED (it is NOT the credited game-start swap)

Who writes it: nobody writes individual entries at runtime. Every one of the 6 cart references to
0x06090304 (0x22045d4, 0x2204a38, 0x2204c70, 0x220502c, 0x2205308, 0x22065bc) is either a POOL
LITERAL (the 0x45d4 / 0x65bc ones are data: bytes `06 09 03 04`) or an indexed READ
(`mov.l @(r0,r4),r4`). It is a fixed table of TEXT-string pointers, read live:

    0x06090304 = { 0x06050ED8, 0x06050E9C, 0x06050EEC, 0x06050F00, 0x06050F14, 0x06050F28,
                   0x06050F3C, 0x06050F44, ... }   (peeked at runtime)

Index 2 = 0x06050EEC = the "PRESS START" string, dispatched to the text routine
g_0609029C = 0x06027080. It is the SAME table and the SAME contents whether or not a credit exists.
There is NO credited-state handler swap at index 2 — the hypothesis from the handoff is DISPROVEN.
attract_handler calls `0x06090304[buf[3]]` -> text-draw only, and ONLY while that player's
credit/join byte is 0 (see S2). So a panel START never installs a game-start handler this way.

## 2. The attract->play transition — LOCATED precisely (attract_handler 0x06008868 / cart 0x204868)

The front-end task attract_handler is the transition owner. Structure (disassembled in full):

    coin_accrual(0x2045f8);           // 0x6098674[p]++ per accepted coin; sets 0x6098680=1
    (*0x0609867C)();                  // credit/HUD draw hook (= 0x060084CC live)
    flag = *0x06098688 + *0x06098689; // P1 + P2 start-present bytes
    if (flag != 0) { ... MORPH ... }  // <=== THE TRANSITION
    else {                            // flag == 0 (always, in the harness)
        if (*0x06098674 /*P1 credit/join*/ == 0)  P1_panel_dispatch(0x06090304[buf[3]]);  // attract text
        if (*0x06098675 /*P2*/          != 0)  return;
        else                                   P2_panel_dispatch(0x06090304[buf[7]]);
    }

The MORPH (flag != 0 path, cart 0x2048ae): if the actor slot-pool is populated
(`*0x06099594 + *0x06099598 != 0`) it OVERWRITES its own task handler
(`task->unk0 = 0x204afa` or `= 0x205232`) — i.e. it turns the front-end task INTO the round/
continue controller — then clears coin-enable, sets 0x06098674 := 3, clears the start flag, and
calls the round/text dispatch g_0609029C. Those morph-target handlers (runtime 0x06008AFA /
0x06009232) are where the game scene descriptor g_0609866C gets set (writers at cart
0x2204df4..0x2205c12). i.e. the game scene is only built AFTER the morph.

GATING CONDITION, exactly: the morph fires iff `0x06098688 || 0x06098689` is nonzero at the top of
attract_handler. Nothing else. Everything hinges on that start-present flag.

Corollary (important, corrects a reading in credited_round_status.md): once P1's credit/join byte
0x06098674 goes nonzero, attract_handler STOPS reading the P1 panel entirely (jumps past
P1_panel_dispatch to the P2 check). So after a coin, a panel START on P1 is not even looked at by
this handler — confirming the START that matters is the FLAG, not a panel read.

## 3. The start-present flag 0x06098688 — set ONLY by the player-character actor, confirmed

Sole setter: cart 0x2229cac (runtime 0x0602DCAC), inside the join-commit state of a
player-character actor FSM whose state handler starts at cart 0x2229c00 (runtime 0x0602DC00). The
state does, keyed on entity[0x34] (= player index 1/2):
  - decrements a per-player counter (word table g_0609978C), gated;
  - `0x06098688[idx-1] = 1`          (raise the start-present flag)
  - `0x06099590[idx]   = r13`        (register the actor into the slot/player table that
                                      attract_handler's morph test reads at +4/+8)
  - morphs the entity handler to the next state (cart 0x2229c00 + 0x158*... = 0x2229DF4)
  - ORs flag bits into g_06099764 entry, writes g_0609868A[idx-1], etc.
Every other reference to 0x06098688 in the whole image is a CLEAR or a READ. So the flag is an
intra-round join signal raised by an ALREADY-SPAWNED player actor. No panel/attract path sets it.

## 4. The P1-actor spawn chain — and the ROOT BLOCKER (empirical, not inferred)

The player-character actor FSM family lives in runtime region 0x0602Dxxx (cart 0x2229xxx).
COVERAGE, via STV_COVDUMP on the cold-boot + 2-coin + repeated-START run:

  * 0x0602Dxxx region ran functions: NONE (empty).  <=== no player-character actor ever executes
  * demo-actor / pool / spawn-list region (0x0602Axxx/Bxxx/Exxx): MANY ran, incl.
    process_spawn_list 0x0602E5D0, pool writer 0x0602AEB8. These are the ATTRACT-DEMO fighters.

So the slot-pool 0x06099590 DOES fill (0x06099594=1, 0x06099598=2 during the combat demo), which is
why the morph's `+4 + +8 != 0` test would pass — but those are DEMO actors spawned from the demo's
spawn-script, using the 0x0602A/B/E handler family. The player-character handler family (0x0602Dxxx)
that contains the join-commit setter is never entered. Hence: start flag never set, morph never
fires, attract loops forever.

Who would spawn a 0x0602Dxxx player actor: the GAME scene's spawn-script, which process_spawn_list
(0x0602E5D0) reads from the scene context (ctx+0x30 -> spawn-script -> +0x14, exits if null). The
game scene descriptor g_0609866C is set only by (a) the morph-target round controllers
(0x2204df4..0x2205c12) — which require the start flag (S2/S3), or (b) boot-machine states 5-10
(writers at cart 0x2201d2c..0x2201da8) — which NEVER RUN (see S5).

=> THE CYCLE:  morph  <-  start flag  <-  player actor (0x0602Dxxx)  <-  game-scene spawn-script
               <-  g_0609866C game scene  <-  morph-target handlers  OR  boot states 5-10.
Every door into this ring is behind another door in the ring, within the code the harness can reach.

## 5. Why boot states 5-10 (the scene director) never run — and the boottask-note polarity fix

func_022016a0 (the 10-state boot machine) idles in STATE 4. Decoding state-4 (cart 0x2017ec):
    r3 = (*0x06000798 >> 5) & 1;            // DIP/config bit5
    if (r3 != 0) goto epilogue(0x2201946);  // bit5 SET  -> return WITHOUT advancing ctx[0x10]
    else { jsr *BIOS; ctx[0x10] = 13; }     // bit5 CLEAR -> terminal/idle (state 13)
*** CORRECTION to boottask_status.md S2: the polarity there is backwards. bit5 SET does NOT go
terminal; it returns to the per-frame epilogue leaving the state at 4. So with the DIP fix
0x06000798=0x60 (bit5 set) the machine sits in state 4 as a steady per-frame service state and
DELIBERATELY does not fall into states 5-10. credited_round_status.md had the correct polarity.
*** The DIP fix itself is correct and is NOT the problem. States 5-10 are a separately-triggered
sequence (reset-type / event driven), not the normal next step, and their trigger is un-decompiled.
So the attract/game flow is carried by the TASK-LIST handlers (attract_handler family), which are
exactly the ones stuck in the S4 cycle.

## 6. What I attempted (flag-gated / diagnostic; no faithful trigger force-writes 0x06098688)

| attempt | result |
|---|---|
| Watch slot/player table 0x06099590..A4 over cold-boot+2coin+START | written only by demo spawner 0x0602AEB8 (demo fighters cycle in slots 1/2); never by the 0x0602DCAC player setter. |
| Watch join/credit + start-flag bytes 0x06098674..90 | 0x06098674[0] goes 1->2 from coin_accrual 0x2045f8 (credit++/coin); start flag 0x06098688/89 only ever CLEARED (fns 0x06005958/0x0600981A/0x06008868), never set. g_06098670=1 (set by fade-ready func_0201c9c). |
| Peek dispatch table + fn ptrs | 0x06090304 = all text ptrs (S1); g_0609029C=0x06027080 (text); g_0609867C=0x060084CC. |
| COVDUMP region diff | player-actor region 0x0602Dxxx = never runs; scene-setup func_02202c5c (0x06006C5C) = never runs; morph targets 0x06008AFA/0x06009232 = never run. |
| **Item-4 faithful-ish trigger:** STV_PANELBTN=1 + cooked START bit (0x0609879A) + panel START + 2 credits, 2500 VB | start flag stays 0; 0x0602Dxxx still never runs. A real cooked/panel START with credit present does NOT spawn a player actor or raise the flag. |

I did NOT force-write 0x06098688 (prior runs proved that is downstream: the morph then enters a
round controller that immediately needs a populated game-scene spawn-script, finds none, and falls
back to attract — no sustained player round). I did not fabricate any round or frame. No new
missing-function walls were hit (the code is present; the blocker is game state, not recomp gaps).

## 7. THE VERDICT — (c) deep gameplay/front-end state-machine logic, a genuine slog

Evidence it is (c), not (b):
  - The transition is a closed dependency cycle (S4) inside the reachable code; the real-hardware
    break-in point (coin -> START -> game scene -> player spawn) is NOT a single flag and is NOT
    reached by any input/flag we can inject (S6).
  - The player-character actor family (0x0602Dxxx, hundreds of functions) is entirely un-executed
    AND essentially un-decompiled; the game-scene spawn-script and the scene-director trigger
    (boot states 5-10) are likewise un-decompiled.
  - Three independent port agents (two prior + this one) have forced the start flag, the join
    status, and real cooked START, and none produced a credited round. The failures are consistent
    and point at the SAME un-built subsystem, not at a tweak.
  - It is additionally coupled to the stage/level-data load (which overlaps the unimplemented VDP2
    RBG1 render), so even if the spawn fired, "a real round" needs that subsystem too.

Why NOT (b): there is no bounded 1-2-run fix. The single most promising thread — find what triggers
the scene director (boot states 5-10 at cart 0x2201d2c, or the post-coin front-end task that reads
the cooked block 0x06098794 and spawns the join actor) — is itself a multi-run reverse-engineering
dig across un-decompiled code, and it only unlocks the FIRST door of the ring. Realistic estimate to
a visible credited round: well into double-digit focused runs of RE + new runtime work (scene-setup
+ player-actor FSM family + spawn-script + stage load + RBG1), i.e. the kind of effort that warrants
PAUSING the port and/or prioritising the functional-decomp of the 0x0602Dxxx actor family and the
0x2201d2c / 0x2204df4-0x2205c12 scene-director region first.

What IS solid and reusable: the entire transition mechanism is now mapped exactly (S1-S5). If the
functional-decomp agent decompiles the player-character actor family (cart 0x2229xxx = runtime
0x0602Dxxx) and the scene-director writers of g_0609866C, this note gives the precise target: get a
0x0602Dxxx actor with entity[0x34]=1 instantiated and stepped to cart 0x2229c00; it will raise
0x06098688 and attract_handler will morph itself into the round on the next frame.

## 8. Concrete next steps (if the port is kept running)
1. Decomp dependency: ask the functional-decomp agent for the 0x0602Dxxx (cart 0x2229xxx) player-
   actor FSM family and the g_0609866C scene-director writers (cart 0x2201d2c, 0x2204df4-0x2205c12).
   This note is the map; those functions are the territory.
2. Find the scene-director trigger: what sets boot ctx[0x10] to 5 (states 5-10), OR which task reads
   0x06098794 START after a coin and installs the join-actor spawn. Watch g_0609866C writers live.
3. Only then: build the game-scene spawn-script path (process_spawn_list 0x0602E5D0 consumer) so a
   player actor spawns; then the stage-load + VDP2 RBG1 for a visible playfield.

## 9. Files touched / seeds / artifacts
- notes/port/play_transition_status.md  (this file; the deliverable).
- port/seeds_extra.txt — dated comment appended; NO new seeds (no new PIC-call walls were reached;
  the cold-boot+credit path is already code-complete per prior runs).
- No code changes to port/ or tools/saturn-recomp/ this run: the investigation showed the blocker is
  game-state/subsystem reconstruction, not a harness tweak, and the task forbids downstream
  force-writes. The existing STV_BOOTMODE / STV_CFG798 / STV_PANELBTN / STV_STARTFLAG diagnostics are
  sufficient to reproduce every finding here.
- No real-round frame was produced (none exists to capture; none was faked). The attract combat-HUD
  frames from prior runs (port/run/ diag_*.png) remain the honest current visual state.
- NOT modified: project/, tools/decomp/, notes/symbols.txt. run/watch12.log + run/diag_*.png intact.

## 10. Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
# transition mechanism: watch the start-flag + slot/player table; note nothing sets 0x06098688:
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 1500 --stvinput "200:COIN,260:COIN,340:START1,500:START1" \
  --watch 06099590:060995A4 --peek "06098688:4,06090304:8,0609029c:1" --out run_probe
# player-actor region never runs (empty grep) vs demo region (many):
STV_BOOTMODE=1 STV_COVDUMP=1 ./recomp-build/saturn_stv ... --out run_probe
grep -iE "^0602d" run_probe/cov.txt   # empty = no player-character actor ran
# item-4 attempt (real cooked START + credit still does not spawn a player / raise the flag):
STV_BOOTMODE=1 STV_PANELBTN=1 ./recomp-build/saturn_stv ... \
  --stvinput "200:COIN,260:COIN,340:START1,360:P1A,500:START1" --watch 06098688:0609868A
```
