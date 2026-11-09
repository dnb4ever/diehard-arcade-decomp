# Die Hard Arcade (ST-V) RECOMP — the cooked-input START gate (0x06098798 bits 4/5): DECODER HUNT

Date: 2026-10-19. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Work done in isolated copy ~/decomp/dh-portA/. Builds on player_spawn_scene_status.md + stv_panel_input_status.md.
Address map: runtime(HWRAM) = cart SH-2 addr + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.

## TL;DR — the decoder does NOT exist in the cart (premise corrected)

The task premise was: "the cooked word 0x06098798 is produced each frame by a panel->cooked DECODER
that reads the raw panel buffer 0x060987D8 and sets bits 4/5; that decoder isn't yet identified."

**That decoder does not exist in the cart.** A complete static dataflow scan of the whole 2 MB program
proves 0x06098798 is NEVER set to a nonzero value by any cart code. The field is a SYSTEM/BIOS-provided
"start-enable" status the game only CONSUMES; with no real ST-V BIOS the harness must synthesize it
(exactly as it already synthesizes the 0x06000660 panel service / 0x060987D8 buffer).

Evidence (reproducible, see scripts in scratchpad):
- 0x06098798 has **ZERO** direct pool literals in the image. It is reached ONLY as [0x06098794 + 4]
  (struct base 0x06098794 = 48 pool refs = 24 unique + the +0x100000 mirror).
- Tracing every one of the 24 unique 0x06098794 loaders (follow reg copies + add#imm) finds exactly
  ONE store to +4 in the whole image: **ST.W @ cart 0x201580 (rt 0x06005580)**.
- That store writes r0, and r0 = r14, and r14 = 0 (set by `mov #0,r14` at cart 0x22013c6, the prologue
  of the enclosing fn at cart 0x022013c4 = rt 0x060071c4 — a boot/HW-ready routine that bumps the
  boot-stage marker 0x06098814 and polls the sub-system ready bit 0x80 @ 0x060e0dd8). So the sole
  "writer" is a **clear-to-0**, not a bit-4/5 setter.
- Exhaustive multi-base re-scan (every pool literal in 0x06098758..0x0609879c, incl. the panel-buffer
  base 0x060987D8 with add#-0x40, incl. r0-indexed stores mov.X Rm,@(R0,Rn)) finds **no other store**
  to 0x06098798/0x06098799. Confirmed: no setter anywhere.
- The per-frame CLEARER named in the task (fn 0x0601328E) is confirmed live: watch shows
  `store16 06098798 = 0000 in fn 0601328E` every VBlank interrupt (pr 0xFFFFFFE8 = IRQ context).

Conclusion: bits 4/5 (P1/P2 start-enable) of 0x06098798 are an external system signal. On real HW the
ST-V BIOS/system raises them (the same layer that fills 0x060987D8). The game's own panel buffer
decode (buf[3]/buf[7] action code, 2=START) is a SEPARATE path and is already modelled in the harness.

## 1. The scene-81 START gate — exact disasm (cart 0x02206876 = rt 0x0600A876)

    r14 = 0x06098794 ; r12 = task ; r11 = task->unk8 (entity) ; r8 = 5
    r0 = word @(4,r14)              ; cooked word 0x06098798
    tst #0x30,r0 ; bt/s dispatch    ; bits 4/5 CLEAR -> SKIP the panel poll entirely
    jsr *0x06000660 (r4=0x060987D8) ; poll panel (only when gate open)
    if (*0x060987E8 & 4) {          ; 2-player panel present
        if (bit4 0x10 of 0x06098798 && byte[0x060987DB]==2) task->state(+0x10)=5   ; P1 START
        if (bit5 0x20 of 0x06098798 && byte[0x060987DF]==2) task->state=5          ; P2 START
    } else if ((0x06098798 & 0x30) && byte[0x060987DB]==2) task->state=5
    dispatch task->state: 0 init .. 5 -> 0x2206c40 (rt 0x0600AC40) STAGE/PLAYER SPAWN

Bit map (authoritative): 0x06098798 bit4 (0x10) = P1 start-enable, bit5 (0x20) = P2 start-enable.
These are NOT derived from panel-buffer bits — they gate WHETHER the panel is even read. The actual
START press is the panel action byte 0x060987DB==2 (P1) / 0x060987DF==2 (P2), already synthesized.

## 2. Faithful implementation (tools/saturn-recomp/runtime/src/machine/bios.cpp)

Added knob **STV_STARTENABLE** inside stv_scene_probe_tick() (default OFF => byte-identical baseline;
added to the early-return guard). When set, each frame it does:
    if (vb >= threshold && ld8(0x06098674) > 0)   // credit present
        st16(0x06098798, ld16(0x06098798) | 0x30); // raise P1+P2 start-enable
This is the faithful BIOS-start-enable substitution (credit-gated, mirroring real-arcade start-enable),
not the blind unconditional STV_SCENEGATE force. ST-V-path only; Saturn disc path untouched.
Credit presence verified live (watch: 0x06098674 = 2 throughout the scene-81 window via STV_SCENEJOIN).

## 3. Honest per-item results (the bar)

| item | result |
|---|---|
| panel->cooked decoder found? | **Resolved: it does not exist in-cart.** Sole writer of 0x06098798 is a clear-to-0 (rt 0x06005580). Bits 4/5 are BIOS/system-provided. |
| bits 4/5 settable faithfully? | **YES.** STV_STARTENABLE synthesizes them credit-gated; credit=2 confirmed so they are raised each frame. |
| scene 81 state machine runs? | 0x0600A876 (SM entry) + 0x0600AC88 (state-region) appear in coverage. |
| scene 81 state 5 reached via START? | **NOT demonstrated.** Baseline (no gate) and STARTENABLE coverage are IDENTICAL (0600A876/AC88/AE2E both). The panel is NOT polled during scene 81's window (START only delivered post-reset at VB>=1641). So opening the gate does not change the outcome. |
| player actor spawned (0x0602Dxxx)? | **NO** (0 coverage lines under 0602d). |
| start-flag 0x06098688 raised? | NO. |
| attract_handler morph fired? | NO. |
| round began / stage rendered? | NO. Scene 81 soft-resets ~VB1622 (pr 0x06005BAC), ~180 frames after entry at VB1440 — same timeout as before. |
| new gameplay walls? | NONE. All runs exited clean (exit 0); no "call to ADDR, not an entry" faults. 0602d never ran, so no player-actor-region walls to autolearn. No new seeds. |

## 4. The revised / real blocker (precise)

The start-enable gate is now fully understood and faithfully synthesizable, but **it is not the active
blocker**. Opening it changes nothing because **scene 81's GAME state machine is not being driven as a
live, panel-polling task** by the forced director walk (STV_SCENESEED/JOIN/CHAR/ADV just set the
director INDEX g_06098668=81; they do not instantiate + tick the scene-81 GAME task). Proof: with the
gate forced open AND dense real START across 1440..1622, the panel service 0x06000660 is never polled
from scene-81 context in that window (no START is consumed until the post-reset attract at VB1641+).

So the next concrete blocker is UPSTREAM: the real attract->play transition that INSTALLS and runs the
scene-81 GAME task (so its state machine actually executes the gate, polls the panel, and advances
state 0->5). This overlaps play_transition_status.md (another agent's area). Only once scene 81 ticks
live will STV_STARTENABLE + a real START drive state 0->5; and state 5 (0x0600AC40) additionally needs
valid character/stage-selection state from the real scenes 75/76 branch (flagged in
player_spawn_scene_status.md as the state-5 precondition).

Revised estimate: the cooked-input gate itself is CLOSED OUT (understood + faithfully implemented).
The remaining path to a player round is the live scene-81 task instantiation (play_transition) + the
state-5 character/stage preconditions — i.e. the work tracked in play_transition_status.md /
player_spawn_scene_status.md, not an input-layer gap.

## 5. Files touched (isolated copy ~/decomp/dh-portA/) — EXACT list for reconciliation

MODIFIED (source — reconcile into main tree):
- tools/saturn-recomp/runtime/src/machine/bios.cpp
  (ONE addition: STV_STARTENABLE knob in stv_scene_probe_tick() + the finding comment block; added to
   the default-off early-return guard. Nothing else changed. Default off => byte-identical.)

SYNCED FROM MAIN (not my edit, per task setup):
- tools/saturn-recomp/runtime/src/video/vdp2.cpp (refreshed from main for RBG1; unchanged by me)

REGENERATED (build artifact; no manual edit): port/recomp-build/*

NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp, port/seeds_extra.txt
(no new seeds — no new walls). video.cpp unchanged (stv_scene_probe_tick already called each frame).

NEW SEEDS for merge into main port/seeds_extra.txt: NONE this session.

## 6. Reproduce (p620, in the copy)

    cd ~/decomp/dh-portA/port && cmake --build recomp-build --target saturn_stv -j"$(nproc)"
    STARTS=$(python3 -c "print(','.join(f'{v}:START1' for v in range(1440,1625,2)))")
    # faithful chain + credit-gated start-enable + real START; coverage dump:
    STV_BOOTMODE=1 STV_STARTENABLE=1 STV_SCENESEED="500:72" STV_SCENEJOIN=600 STV_SCENECHAR=0 \
      STV_SCENEADV="540:7:180" STV_COVDUMP=1 ./recomp-build/saturn_stv --cart diehard_main.bin \
      --cartrom ../cart.bin --vblanks 1650 --stvinput "100:COIN,160:COIN,$STARTS" --out run/senable
    grep -icE '0602d' run/senable/cov.txt    # 0 = no player actor
    # baseline comparison (no STARTENABLE) -> identical scene-81 coverage:
    (drop STV_STARTENABLE, --out run/base) ; diff <(sort run/base/cov.txt) <(sort run/senable/cov.txt)
    # static proof scripts (Mac scratchpad): trace794.py, trace2.py, traceall.py, traced8.py, discln.py
    #   run via ~/decomp/diehard-stv/tools/decomp/venv/bin/python

## 7. Frames
No new round/character/stage frames — no round started (honest; nothing faked). Prior scene-81 /
attract-demo frames in player_spawn_scene_status.md remain the current visual state.
