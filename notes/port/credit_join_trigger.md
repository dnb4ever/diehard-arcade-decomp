# Die Hard Arcade (ST-V) RECOMP - the CREDIT-JOIN TRIGGER (coin -> join actor -> start flag -> morph)

Date: 2026-10-20. Port side only. Read-only analysis + live --watch + frame capture; NO source edits.
Work in isolated copy ~/decomp/dh-portA/. Reference C read-only from project/src_functional/.
Address map: runtime(HWRAM) = cart SH-2 + 0x03E04000 ; cart SH-2 = cart.bin file-offset + 0x02000000.
Builds on: round_spawn_experiment.md (the join is a player-actor STATE unk43, dispatched by func_022863e
via table 0x06091100: 0x1D=arm 0x2229b60, 0x20=commit 0x2229c00), coin_arm_and_reel.md (deliver_coin
gate; 0x06098670 arm; attract reel), attract_to_credit_trigger.md (scene-director graph; scene-3 park).

## VERDICT (honest): the credit-join trigger is FOUND, fully DECOMPILED, and REACHABLE. A single coin
## drives the game's own credit-join chain end-to-end to the point of raising the start flag and morphing
## the attract task into the round-enter sequencer - visibly changing the screen from "INSERT COIN(S) /
## CREDIT 0" to the combat-HUD + "CONTINUE? N" countdown. It does NOT land in a playable round: the chain
## stops at the credit/continue countdown (credit byte tops at 3; the round-enter sequencer f_2204afa wants
## byte 4), and the scene director resets at scene 81. No player-character sprite renders (known VDP1 gap).
## This is the furthest any session has driven the real play-enter path - prior notes had g_06098688 stuck
## at 0; the coin-arm fix (coin_arm_and_reel.md) is what let the commit fire. Classification: reached
## through the start flag and the morph (beyond prior (B)); the final round entry needs condition Z
## (credit byte 4 in f_2204afa + scene-director coordination off the parked attract scene).

## 0. THE CHAIN, fully mapped (every function below now has clear C semantics)

  coin --deliver_coin/coindetect--> g_0609880C=1 (P1 coin-pending)
  credit-scene fade / UI          --> g_0609880E=1 (P1 companion / "fade-armed")
  [both ==1] -----------------------> attract_credit_join_service f_22045f8  (THE TRIGGER GATE)
       increments credit byte g_06098674[P1] 0->1, inits counter g_0609978c=g_06099796, then
       (credit byte==1) calls -------> f_2228168 / f_2228332  (THE CONTEXT BUILDER + ACTOR SPAWN)
             advances credit byte 1->2; locates the player entity in the actor list; sets its
             position/facing from record table g_06060814[g_060b3d54]; builds the FULL join context:
             counter g_0609978c[idx], g_0609975c[idx]=0x1E, g_06097420[idx]=8, paired object
             g_060995b0, unk56 join-window timer, the g_06098670/g_06098830 paired-object sync block;
             and SPAWNS the player join-actor via func_02227bc4 (the EXPLICIT-id registrar that installs
             func_0222a300 and sets entity->unk34=player idx) -- the SAME real spawn path the
             round_spawn_experiment synthesized by hand, here driven by the credit.
  spawned actor runs func_0222a300 -> a418 -> (-1 term) -> func_0228508 -> func_022863e (main dispatch)
  func_022863e dispatches on entity->unk43 via table 0x06091100:
       state 0x02 = func_022288e8 : when unk54&2 (ready) && unk56==0 (window expired) -> set unk43=0x1D
       state 0x1D = func_02229b60 (arm) : on unk54&2 -> unk c8=60, unk43=0x20
       state 0x20 = func_02229c00 (commit) : counts down unk c8, decrements counter g_0609978c[idx];
             when it hits 0 -> RAISE start flag g_06098688[idx]=1, publish 12-byte record
             g_06099764[idx], OR this player bits into g_0609977c, morph *task -> func_02229df4
       func_02229df4 (next) : on g_0609868a[idx-1]==1 publish active slot g_06099590[idx]=idx, update
             paired object g_060995b0, clear bits, morph *task -> func_022863e
  attract_handler f_2204868 (runs f_22045f8 every frame) : once g_06098688!=0 -> morph its OWN task
       *arg0 -> f_2204afa (slot set) or f_2205232 (slot clear) [the round-enter sequencers], set
       attract-entity unk40=0x27F, credit byte g_06098674=3, clear start flag, issue resident cmd 0x20.
  f_2204afa (round-enter sequencer) : gated on unk40!=0 (ok) AND credit byte==4; sets g_0609868a[0]=1
       (the sel==1 commit signal func_02229df4 reads) and morphs again toward play.

## 1. Addresses / functions (cart SH-2 ; runtime = +0x03E04000). "DEC"=has C in src_functional.

  022045f8  attract_credit_join_service  THE TRIGGER GATE (undec; decompiled here, see sec 2)
  02228168  join-actor setup P1 path     CONTEXT BUILDER + SPAWN (undec; decompiled here)
  02228332  join-actor setup alt (==4)   sibling of 228168, same region (undec; decompiled here)
  0220581a  per-player join reset        inits counter g_0609978c=g_06099796 (undec; decompiled here)
  02204868  attract_handler              start-flag consumer + task morph (undec; decompiled here)
  02204afa  round-enter sequencer        next layer Z (undec; partial decompile here)
  02205232  round-enter sequencer (alt)  alt morph target (undec)
  02227bc4  explicit-id registrar        REAL player spawn (undec; per round_spawn_experiment)
  022288e8  join state 0x02              unk54&2 && unk56==0 -> unk43=0x1D (DEC: 22288e8.c)
  02229b60  join_arm state 0x1D          unk54&2 -> unk43=0x20 (DEC: 2229b60.c)
  02229c00  join_commit state 0x20       raises g_06098688 (DEC: 2229c00.c)
  02229df4  join_next                    publishes slot/paired (DEC: 2229df4.c)
  0222863e  player main-state dispatch   table 0x06091100 (DEC: 222863e.c)

  Per-player join globals (idx=1 P1 / 2 P2 ; most arrays stride by (idx-1)):
   g_06098674/75 credit byte   g_0609880C/10 coin-pending   g_0609880E/12 companion/fade-armed
   g_06098688/89 start flag    g_0609868a/8b sel signal     g_0609978c/8e join counter (=g_06099796)
   g_06099764[12] commit record g_06099590[] active slot    g_060995b0 paired object
   g_0609975c[] =0x1E          g_06097420[] HUD =8          g_06099794/96 join count/window values
   g_06098670 2P-armed         g_06098830/0609882c paired-obj sync

## 2. attract_credit_join_service f_22045f8 (THE TRIGGER GATE) - decompiled (m2c, hand-read)

  P1 block (P2 symmetric with ...10/12/675/978e):
    if (g_0609880C==1 && g_0609880E==1) {        // coin-pending AND credit-scene companion both set
        g_0609880E=0; g_0609880C=0; g_06098680=1; g_060987D4.unk0=1;
        g_06098674[P1]++;                         // 0 -> 1
        g_0609978c[P1]=g_06099796; g_06099788[P1]=g_06099794; g_0609975c[P1]=0x1E;
        g_060997a4=3; g_060997a8=1; (*g_0609029C)(g_06050D84, &g_0609880C);
        if (g_06098674[P1]==1) { f_2228168(P1); g_060987f8=(*g_06000640)(0); }   // <-- BUILD + SPAWN
        else if (g_06098674[P1]==4) { f_2228168(...); g_060987f8=...; }
        (*g_0600066C)(&g_060987D4); (*g_06000688)(0x10);
    }
    ... then a lower block: if (g_06098674[P1]==1 && g_0609868e!=1) { g_06098674[P1]=2; ... }

## 3. f_2228168 (CONTEXT BUILDER + SPAWN) - decompiled (m2c, hand-read; shares region w/ f_2228332)

  f_2228168(idx):
    if (g_06098674[idx]==1) {
        g_06098674[idx]=2;                                   // credit byte 1 -> 2
        entity = scan actor list (g_0609866c.unk1C.unk8, stride 0x18) for node.unk0==idx+1
        rec = g_06060814 + g_060b3d54*0x2C
        entity.unk8=rec[0]; entity.unkC=rec.unk4; entity.unk10=rec.unk8      // position
        if (entity.unk34==1){unk26=0xC000; unk8-=0x10000; unk44=1} else {unk26=0x4000; unk8+=0x10000}
        entity.unk5E=node.unk1; entity.unkBC=-1; entity.unk4E=0x5A
        if (g_06098670==1 && g_06098830>=3){ paired-obj sync via g_0609882c; g_06098830=3; ... }
        g_0609975c[idx]=0x1E; g_06097420[idx]=8; g_0609978c[idx]=g_06099796;   // counter init
        entity.unk56 = g_0609979a==1 ? g_06099794*2 : g_06099794;              // join-window timer
        func_0221707c(entity, 0x5999, -0x4CCC, 0x4000, 0x14ccc); func_02227bc4(...);  // SPAWN
        ... func_0220ac5a/22a91a/22ca54/22da90/22dc2c(anim); g_060995b0[idx] paired
    }
    (the credit-byte==4 branch is f_2228332: same build, plus the g_060e0e38[idx] control-table test)
  NOTE: f_2228168 writes NO unk43 / unk54. The actor state (unk43=2) and ready bit (unk54&2) come
  from the spawn/anim path and the join-window expiry; the unk43=0x1D arm is set by func_022288e8.

## 4. f_220581a (per-player join reset) - decompiled; called by bsr from scene-director 0x205948/0x205a10

  f_220581a(idx): g_06098688[idx]=0; g_0609868a[idx]=0; g_06099790[idx*2]=0; g_0609868c-tbl[idx]=0;
    g_0609975c[idx*4]=0x1E; g_0609978c[idx*2]=g_06099796; g_06099788[idx*2]=g_06099794;

## 5. LIVE EVIDENCE - the chain fires on a single coin, NO forcing (STV_BOOTMODE=1, "200:COIN")

  credit byte g_06098674[P1]:   0 ->1 ->2  @VB232 (f_22045f8+f_2228168)  ->3 @VB3887 (attract_handler)
  join counter g_0609978c[P1]:  =1 @VB201 (f_220581a)  =1 @VB232 (f_2228168)  ->0 @VB3885 (commit)
  start flag  g_06098688[P1]:   ->1 @VB3885 in fn 0602863E, pr 0602DC86 (= func_02229c00 COMMIT, state
                                 0x20) ; consumed ->0 @VB3887 by attract_handler (pr 06008926)
  scene index g_06098668:       2(203) ->3(1038, park) ->0x51/81(5169, reset-timeout) ->reboot 2(5479)

  => func_02229c00 (the SOLE start-flag setter) RAN and raised g_06098688 through the real FSM, driven
     purely by the coin. attract_handler consumed it and morphed its task to f_2204afa. Confirmed clean
     (no FATAL, no new "call to ADDR" walls). A coin+START run is identical (START at VB340 does not
     change the commit timing - the join commits on the internal window countdown, not the START edge).

## 6. FRAMES (honest) - coin-driven state change, but NO player character sprite

  run/round_frames/shot-3900.png , shot-4200.png : combat HUD (two portraits + health bars) + a ticking
     "CONTINUE? 9" -> "CONTINUE? 7" countdown over the attract night-skyline (RBG1).
  run/nocoin_ctrl/shot-3900.png (control): "INSERT COIN(S) / CREDIT 0" on black - NO HUD, NO countdown.
  => the coin DID change the screen (control differs): the game accepted the credit and entered the
     credit/continue-join countdown. But it is the CONTINUE/credit screen, not a playable round, and no
     player-character sprite is drawn (the playfield VDP1 sprite path is the known separate rendering gap,
     per coin_arm_and_reel.md 5b / round_spawn_experiment.md 8). The milestone "a player character on
     screen" is NOT met. Nothing was faked.
  Mac scratchpad: round_commit_vb3900.png, round_morph_vb4200.png, nocoin_vb3900.png, credit_coin_vb232.png

## 7. THE NEXT GATE Z (what stands between here and a playable round)

  attract_handler leaves credit byte g_06098674[P1]=3 after consuming the start flag. The round-enter
  sequencer f_2204afa takes its play-enter branch only on credit byte ==4 (and attract-entity unk40!=0,
  which IS set to 0x27F). The single-P1-coin path never reaches byte 4 (a 2nd coin is correctly refused by
  the deliver_coin/coindetect gate), and the scene director - parked at scene 3 - fires its scene-81
  reset-timeout at VB5169, tearing the state down. So Z = (a) reach credit byte 4 (the 2P / second-credit /
  continue-confirm state - f_2204afa round trigger) AND (b) coordinate the scene director off the parked
  attract scene onto the round scene (it only consults the credit count at scene 73, selector 3, or scene 1,
  selector 4 - neither reached from the parked attract reel). Decompiling f_2204afa fully (and f_2205232)
  and resolving who writes credit byte 4 is the next concrete step. These are now the only unmapped links.

## 8. Files changed this session (isolated copy) - EXACT list
  SOURCE: NONE. No edits to port/ or tools/saturn-recomp/. Pure read-only analysis + live runs + m2c.
  NEW (this report): notes/port/credit_join_trigger.md
  RUN ARTIFACTS (not source): run/credtest1..3/, run/round1/, run/round_frames/, run/coinstart{,2}/,
    run/twocoin/, run/nocoin_ctrl/ (+ shots). Mac scratchpad frames (sec 6).
  NEW SEEDS: none - every run exited clean (no new "call to ADDR, not an entry" walls).
  DUAL-VALUE FOR MAIN DECOMP: new C semantics for 6 undecompiled functions ready to write as .c files:
    022045f8 (attract_credit_join_service), 02228168 + 02228332 (join setup/spawn), 0220581a (join reset),
    02204868 (attract_handler), 02204afa (round-enter sequencer, partial). All cross-checked vs m2c --asm.
  NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp, runtime sources.

## 9. Reproduce (p620, in the copy)
  cd ~/decomp/dh-portA/port
  # the chain fires on one coin: credit 0->1->2->3, start flag raised @VB3885, morph:
  STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 4000 \
    --out run/credtest2 --watch 06098688:0609868c --stvinput "200:COIN"   # -> 06098688=1 @VB3885 fn 0602863E
  # coin-driven CONTINUE screen vs no-coin control:
  STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 4600 \
    --out run/round_frames --shots 3900,4200 --stvinput "200:COIN"
  STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 4300 \
    --out run/nocoin_ctrl --shots 3900,4200 --stvinput "9000:SERVICE"
  # decompile any of the credit-join functions:
  source tools/decomp/venv/bin/activate ; python3 tools/decomp/m2c_fn.py 0x2045f8   # (or 0x228168 ...)
