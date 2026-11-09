# P4.1 — faithful player spawn, control and enemy damage

2026-10-30, Codex. **Milestone behavior verified in this workspace:** P1 spawns,
scripted physical controls move him, and scripted attacks damage E3. No
LIVEROUND/SYNTHDEF/STAGEDMA or other game-state bridge was used. No commit made.

Host caveat: this session's `hostname` is the build host, not p620. Everything
below was written/run inside `~/decomp/diehard-stv` here. No remote
p620 execution or synchronization is claimed. No GPU or prohibited directory was used.

## What was actually wrong

There were two distinct issues, neither requiring a new forced spawn gate.

1. **The local executable/runtime/generated tree was stale relative to the
   handoff.** The existing executable was dated Oct 19. The nested runtime lacked
   `kStvPanelCtrl`, the documented service-time control-word fix, system-BIOS tick,
   SCENETRACE and PLAYERPEEK. Current `main_stv.cpp` could not build against it:
   `sound_stv_autostart` was undeclared. The recorded
   `port/runtime-patches/round-runtime-dh-portB.patch` contains those changes,
   including the exact cwfix. Reconstructing that patch against upstream
   `b22688d0bef52fba31cda79fd23150e8905cf3c2` restores the documented baseline.
   Regenerating with only the note symbols and extras lost existing generated
   entries (`0602A144`); using the **union** of the old generated entry table,
   note symbols and extras retains them. `0600A86A` was already in extras but
   absent from the stale generated table. Neither missing entry required a game
   logic change. Initial failures and builds are preserved under `port/run/P41/`.
2. **The input harness discarded physical button/direction identity.**
   `stv_input_script` reduced A/B/C/directions to kinds 6/7. With PANELBTN unset,
   those events only logged; no raw input reached `06000708/06000710`. Thus a
   present player received zero physical input. The new fix preserves the raw
   mask and publishes held controls into those BIOS peripheral words each VBlank.

The initial short-run diagnosis that P1/P2 never populate is disproved by the
rebuilt trace: both slots first populate at VB535 in scene 2 (P1 HP171, P2 HP0).
Those actors are transient opening content. The persistent combat P1 appears at
**VB1181**, task `06092524`, entity `060E1AA0`, ID1, HP171. E3 appears alongside him
at task `06092644`, entity `060E1900`, HP64. P2's persistent slot remains empty,
as expected for a P1-only paid start.

## Gate trace and game logic

Read the functional C rather than inferring a BIOS routing requirement:

| VB | Scene | 0609979A | 06098670 | 06098830 | P1 join byte |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0 | 0 | 0 | 0 |
| 9 | 1 | 0 | 0 | 0 | 0 |
| 12 | 1 | 0 | 1 | 0 | 0 |
| 356 | 2 | 0 | 1 | 0 | 2 |
| 1179 | 3 | 0 | 1 | 0 | 2 |
| 1181 | 3 | 0 | 1 | 3 | 2 |
| 1212 | 3 | 0 | 1 | 10 | 2 |

Of the three suggested globals, **0609979A never becomes 1** over the 6000-VBlank
reference. That is not a necessary normal-combat gate: `02232bc4.c:137` sets it
to 1 for scene 75's two-fighter intro, and line 203 clears it. The ordinary
spawn path explicitly supports value 0; `0222c220.c` even requires 0 for its
special move. Setting it to 1 would alter game mode rather than repair hardware.

The direct join coordinator `022045f8.c` calls `player_join_spawn` only when
join==1 and `0609868E==1`. The director (`02205b20.c`) writes that gate from
descriptor byte +0x2A. During scene-1 join acceptance the gate is 0, so the
coordinator advances join to 2 without spawning. This is the code's intended
branch, not proof that the slot can never populate.

Actual persistent spawn chain, confirmed by read-only generated entry probes:

* `02232610` (runtime `06036610`) enters state 0 at VB1178, state 1 at VB1180,
  state 2 at VB1182. Gate `0609868E` is now 1.
* Its state 1 creates the `0222a120` actor-spawn-list task. That task runs at
  VB1180 and selects the P1 list because join bytes are 2/0. Its fixed-ID spawn
  creates the persistent P1; by VB1182 its list task is state 3 (complete).
* `0222a120.c` seeds HP from `06099788[id-1]` when 0609979A is 0. This is a
  separate route from `player_join_spawn`. `process_spawn_list` (`0222a5d0`)
  also runs earlier, at VB363, for scene-2 opening content.
* `0220b1c0.c` writes progress 10 and can immediately reduce it to 3 while armed;
  the trace subsequently reaches 10. Progress is not the missing spawn gate.
* `0222a8d8.c` produces channels=1 for join=2/0 and arm=1. The old claim that
  arm=1 necessarily freezes the timeline at channels=0 does not apply here.

`P41CALL` evidence is in `final-input.log` / `raw-control.log`; the temporary
generated call probes were discarded by the final reproducible regeneration.
The persistent runtime `STV_P41TRACE` diagnostics remain available and read only
game memory (apart from the separate physical-input publication).

## Exact changes

* Restored the existing recorded `round-runtime-dh-portB.patch` baseline into
  the nested runtime. Its nine file diffs are the exact baseline restoration;
  these are **previously documented fixes**, not nine new P4.1 fixes. The earlier
  nested changes were saved as `port/run/P41/nested-before-restoration.patch`.
* New behavioral diff: [p41-raw-panel-input.patch](../../port/runtime-patches/p41-raw-panel-input.patch),
  applied after that baseline. File: `tools/saturn-recomp/runtime/src/machine/bios.cpp`.
  Events retain a `raw` mask and optional duration; `VB:TOKEN:FRAMES` defaults to
  two VBlanks. Each tick ORs active controls per player and writes P1/P2 BIOS raw
  words. A/B/C=1/2/4, UP/DOWN/LEFT/RIGHT=0x40/0x80/0x100/0x200, per the cart's
  `0220f9d4.c` decoder and `0220e1e2.c` consumers. Release writes 0. Simultaneous
  tokens combine. Coin/START keep their existing two-VBlank service semantics.
* Read-only diagnostic diff: [p41-player-trace.patch](../../port/runtime-patches/p41-player-trace.patch).
  `STV_P41TRACE=1` records all three suggested globals and all six actor slots
  every VBlank, plus join gate/channel/executive/camera/timeline context.
* `port/main_stv.cpp`: updated `--stvinput` grammar comment.
* `port/p41_build.py`: reproducible isolated regeneration/build, retaining old
  entries and stripping inline seed comments correctly. Generated output is
  `port/run/P41/recomp`; it builds against the actual **nested runtime**, not the
  temporary reconstructed copy.

Physical input publication emulates missing peripheral/BIOS input behavior.
It never writes an actor, HP, join state, scene index, mode, or progress global.
The cart computes cooked input, edges, movement, attacks, collisions and damage.
Legacy X/Y/Z and SERVICE retain their earlier diagnostic behavior; verified
physical gameplay tokens are A/B/C and directions.

## Reproduce

From the repo root, with no inherited `STV_*` bridge variables (none were set in
the session environment):

```bash
python3 port/p41_build.py
mkdir -p port/run/P41/repro
STV_BOOTMODE=0 STV_CFG798=40 STV_P41TRACE=1 STV_SCENETRACE=1 STV_PLAYERPEEK=30 \
  port/run/P41/build-generated/saturn_stv \
  --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2100 \
  --stvinput '200:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2,1500:P1A:2,1550:P1A:2,1600:P1A:2,1650:P1A:2,1700:P1A:2,1750:P1A:2,1800:P1A:2,1850:P1A:2' \
  --shots 1320,1440,1560,1800,2000 --out port/run/P41/repro \
  > port/run/P41/repro.log 2>&1
```

Reference: same invocation using only `--stvinput '200:COIN,340:START1'`.
No-coin regression: use
`--stvinput '99999:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2'`,
`--vblanks 1800`, `--shots 1700`, and a separate output directory. The out-of-window
coin prevents the harness's empty-script default from inserting a coin.

No LIVEROUND, SYNTHDEF, STAGEDMA, RAWINPUT, PANELBTN, VDP1ERASE, RBG_PLANE,
SCENESEED, FORCEARM or STARTFLAG was used in the final verification runs.

## Evidence and results

Final logs: `port/run/P41/{final-reference,final-input,final-nocoin}.log`.
Machine-readable paired samples: `port/run/P41/comparison.csv`.
All final runs reached their VBlank limits with no FATAL. Final build and the
tested build helper passed: `final-build.log`, `repro-build.log`. The diagnostics-off
smoke reached VB1600; all three PNGs are byte-identical to their traced-run
counterparts. Applying the input patch then trace patch to the recorded baseline
reconstructs the installed nested `bios.cpp` byte-for-byte.

| Observation | No added physical input | RIGHT + scripted A presses |
|---|---|---|
| VB1320 P1 X/Z, before input | 0012966A / 00018001 | identical |
| VB1440 P1 X/Z, after held RIGHT | 0012966A / 00018001 | 00102D4C / 000182C2 |
| E3 HP at VB1440 | 64 | 64 |
| E3 HP at VB1462 | 64 | 54 |
| E3 HP at VB1508 | 64 | 48 |
| E3 HP at VB1858 | 64 | 42 |

The same E3 task/entity stays allocated throughout those HP transitions, so this
is damage rather than comparing different actors. Cooked input reports 0200
during the RIGHT hold, then releases to 0; A is delivered as a separate edge.
The paired reference and input runs are identical before input starts.

PNG evidence (all paths relative to repo root):

* Before physical input: `port/run/P41/final-input/shot-1320.png`.
* After RIGHT: `port/run/P41/final-input/shot-1440.png`.
* After attacks/E3 damage: `port/run/P41/final-input/shot-1560.png`.
* Later combat: `port/run/P41/final-input/shot-1800.png`, `shot-2000.png`.
* Paired reference: `port/run/P41/final-reference/shot-{1320,1440,1560,1700,1800,2000}.png`.
* No-coin regression: `port/run/P41/final-nocoin/shot-1700.png`.
* Diagnostics-off rebuilt smoke: `port/run/P41/diagnostics-off/shot-{1320,1440,1560}.png`.

No-coin + START + movement/attack regression **PASS**, 1800 VBlanks: scene stays
1, join=0/0, all six actor slots empty, no spurious start. With no physical
events, the patched reference's VB1700 PNG is byte-identical to the reconstructed
pre-input-fix run (SHA256 `5a75a76d49f67160fa841b1edc73d6ce555ce2590f36a973dc693463a4434a3e`).

## What is next / limits

P4.1's requested spawn/control/enemy-damage behavior is verified, but the overall
port is not complete. INSERT COIN persists; VDP1 stale-image/erase artifacts and
background rendering remain visible without renderer knobs. In the no-input
6000-VBlank reference P1 eventually dies, the executive enters the countdown,
and the director reaches scene 81 at VB5716, followed by a black frame. Sustained
post-death/restart progression and overlay cleanup are separate next tasks.

The suggested gate chain required no new forced game-variable fix and no pair of
successive deeper-gate fixes: stop-rule forcing was unnecessary. Do not force
0609979A=1 or seed scenes 73–80 to reproduce this result. Re-run the commands on
p620 if that distinct machine is the required release target.
