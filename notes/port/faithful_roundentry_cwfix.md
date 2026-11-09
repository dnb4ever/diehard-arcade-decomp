# Faithful round entry — the start-enable control-word fix (2026-10-23)

## TL;DR
The game now enters a **real gameplay stage from a faithful coin + START**, with **zero bridge
knobs** (STV_LIVEROUND / STV_SYNTHDEF / STV_STAGEDMA / STV_RBG_PLANE are no longer needed to reach
a stage). The wall that six-plus prior passes hit was a **single stale control bit**.

## Root cause (proven by recompiled-asm trace + runtime instrumentation)
The credit/join scene handler `func_0220600e` (cart 0x0020600e), in its state-2 START-join guard,
reads the ST-V BIOS control-panel word at **0x06098798** and requires bit 4 (`cw & 0x10`,
start-enable) set before it calls the coin-consume service `*0x06000664`. If the consume returns
nonzero (a banked credit) it latches the join (0x0609880C=1) -> state-3 commit -> installs
attract_handler -> executive morph -> scene director advances to a stage scene (2..72).

The HLE re-asserted 0x06098798 bits 4/5 (kStvPanelCtrl=0x34) only in `stv_system_bios_tick` at
**vblank-start**. The game clears 0x06098798 each frame *after* that, so by the time `credit_render`
read it, `cw & 0x10 == 0` -> the START-join branch was skipped -> `0x06000664` was **never called**
(verified: 0 calls across the run) -> no join -> no round. Every other input was correct: coin
banked (g_stv_credits=1), START action byte 0x060987DB=2 delivered to the handler's buffer, latch
clear. Only the control bit was stale.

Note: the functional-decomp reconstruction `0220600e.c` guessed the guard used a fixed 0x06098794
pointer for `cw`; the real asm confirms `cw = *(u16*)([sp+4]+4)` where [sp+4]=0x06098794, i.e.
*(0x06098798) — same address, so the HLE target was right; the bug was purely the timing of the
re-assert vs the game's per-frame clear.

## Fix (faithful, always-on, one line)
`tools/saturn-recomp/runtime/src/machine/bios.cpp`, in `stv_panel_read` (the 0x06000660 / 0x0600066C
panel service the handler calls immediately before the guard):

    st16(0x06098798u, (uint16_t)(ld16(0x06098798u) | kStvPanelCtrl));

The real ST-V BIOS/SMPC owns these panel/start-enable bits and keeps them live; re-asserting them
in the panel service (right before the handler reads them) matches that and closes the timing gap.

## Verification
- Faithful PAID flow (default --stvinput "200:COIN,340:START1", STV_BOOTMODE=0 STV_CFG798=40, no
  other knobs): SCENETRACE walks scene 0 -> 1 -> 2 (first stage) -> 3; credit(0x674)=2 (joined);
  the stage spawn-list builds entities E3..E6; renders the real opening (POLICE helicopter + char
  models over the skyline). ~1500+ entity-present frames (was 0).
- Regression — true no-coin attract (coin pushed out of window): stays at scene 1 (credit-wait),
  credit=0, no entities, NO spurious start. The fix cannot bank a credit, so a paid start still
  requires a real coin.

## Credit
The faithful credit/BIOS coin-layer in bios.cpp (and the func_0220600e behavioral map) was drafted
by a Gemini-4 "Argon" agent (via the Antigravity `agy` CLI). The root-cause diagnosis (SCENETRACE /
PLAYERPEEK instrumentation + m2c_fn.py --asm trace of the real handler) and the cwfix completion +
regression verification were done by Claude (Opus 4.8).

## NOT done-done (next)
- The scene-2 entities (E3..E6, unk34=3..6) are opening-stage actors with HP=0, and the P1/P2
  *player* slots aren't populated yet — so **controllable combat with a damageable enemy is not yet
  confirmed**. This is faithful round *entry*, not verified playable combat.
- A leftover "INSERT COIN(S)" overlay needs clearing once the round is live.
- The bridge-knob stack (LIVEROUND/SYNTHDEF/STAGEDMA/RBG_PLANE) is now obsolete for round entry and
  can be retired.
- Next step: run further and verify progression into controllable combat (player actor spawn into
  P1/P2, enemy AI acting + taking damage, win/lose).
