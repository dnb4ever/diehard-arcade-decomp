# Die Hard Arcade (ST-V) RECOMP — real boot sequence + the 10-state boot machine (func_022016a0)

Date: 2026-10-19. Builds on notes/port/credit_playmode_status.md, stv_input_gameplay_status.md,
spawn_diagnosis.md. Port side only (port/ + tools/saturn-recomp/). project/ and tools/decomp/ read-only.

## TL;DR — the prior diagnosis was partly wrong; the real fix is ONE BYTE

- **The boot-task IS already installed under the harness.** frame_sync_loop installs the full
  initial task set (including the boot controller) every cold run; the boot controller **and**
  func_022016a0 (the 10-state machine) **already run** under the harness today — verified by
  coverage, in BOTH the default path and the boot-mode path. The spawn_diagnosis/credit notes'
  claim "main_init_entry never runs func_022016a0 / the boot task is never installed" is **wrong**.
- **The single missing boot-state the harness skips is the BOOT-MODE byte `0x060987CC`.** The real
  ST-V entry stub (which the harness jumps over) writes a reset-type value {0,1,2} there before
  entering main_init_entry. The boot controller reads it to choose func_022016a0's **start state**.
  The harness leaves it 0 → func_022016a0 starts at state 3 (warm/soft-reset path) and **skips the
  cold-boot init (state 1 = func_0201b92: sound + panel + controller-present + config)**.
- **Fix = write `0x060987CC = 1` (cold power-on) before entering main_init_entry** (flag-gated
  `STV_BOOTMODE`, default off). With that + 15 autolearn seeds the game now runs its **full
  cold-boot → title ("INSERT COIN(S)") → attract loop**, func_0201b92 runs, process_spawn_list
  runs, the spawn context `0x06099580` initialises, and the in-game **combat HUD (character
  portraits + health bars) renders**. Runs **clean to 6000 VBlanks — no missing-function walls.**
- **Still NOT reached:** a *credited* player round and a *visible* playfield. Coin-pending is set
  but credit never accrues on this path; the playfield (actors + stage) stays black. Both are named
  and localised below — neither is a mystery.

## Address mapping
runtime(HWRAM) = cart SH-2 addr + 0x03E04000. cart SH-2 addr = cart.bin file offset + 0x02000000.
diehard_main.bin (the loaded image) == cart.bin[0x200000:0x28de20], byte-identical (no relocation).

---

## 1. The real boot sequence (disassembled from the cart entry forward)

```
reset / header @0x02200100  (ST-V "SEGA ST-V ... DIEHARD ARCADE" header)
   braf -> region/area table @0x02200e00 (JP/USA/EU... bra chain)
   PIC boot stub @0x0220013c  (relocation-ish + SECURITY CRC @~0x02200748 -> self-branch HANG)
   three reset entries, selected by reset type (SMPC):
       0x02201000: r0=0  \
       0x02201004: r0=1   > mov.l r0,@0x060987CC ; jmp main_init_entry(0x06005020)
       0x02201008: r0=2  /         ^^^^ BOOT-MODE byte written here, then jump
main_init_entry @0x02201020 (= runtime 0x06005020; THIS is where the harness enters):
   bsr FUN_02201958            global-init (clears HWRAM state; does NOT touch 0x060987CC)
   install 3 ISRs via *0x06000300 (vectors 64/65/67)
   program VDP2, SCU timer; init task pool; set 0x06099578=1
   tail -> frame_sync_loop @0x022013c4
frame_sync_loop @0x022013c4 (the per-frame main loop):
   bsrf 0x0220fe48(handler_array @0x0604EBC0)   <-- INSTALLS THE INITIAL TASK SET (batch)
   per frame: sync on 0x060E0DD8 bit7; bsrf task_list_run(0x0220fde4); render; swap; repeat
```

**0x0220fe48 = task_install_batch(handler_array):** walks a NULL-terminated array of task-handler
pointers and links a node for each into the task-list control block `g_06092238`
(freelist@0, iter@4, cur@8, sentinel@0x0C — see project/src_functional/0220fde4.c). The array it is
called with (@cart 0x0224ABC0 / rt 0x0604EBC0) is:

| idx | handler (rt) | note |
|---|---|---|
| 0 | 0x06005B6C | boot/config sibling (near func_0201b92 @0x06005B92) |
| 1 | 0x0602A134 | task |
| 2 | 0x0600F0A4 | task |
| 3 | 0x0602B790 | task |
| 4 | **0x06005590** | **the BOOT CONTROLLER** (cart 0x02201590) |
| 5 | 0x00000000 | terminator |

So the boot controller is **installed by frame_sync_loop on every run, including the harness's.**

### The boot controller (handler 0x02201590) — a 3-state (0/11/12) morphing task
- state 0: creates a child task (via create-helper 0x02214BB8, 108 bytes), **reads BOOT-MODE
  `0x060987CC`**, indexes dispatch table @0x0604EBD8 → writes ctx[0x10] = its next state, stores its
  ctx ptr to 0x060987B8, then branches on boot-mode:
  - mode 0/1/3 → **reassigns its own handler to 0x022016AC (= func_022016a0)**; next frame
    task_list_run calls func_022016a0 instead, starting at the table-selected state.
  - mode 2 → stays itself, state 11.
- **Boot-mode dispatch table @0x0604EBD8:** `{0:3, 1:1, 2:11, 3:1}` (the func_022016a0 start state).

So **BOOT-MODE directly selects where func_022016a0 begins**:
mode 1 (cold power-on) → **state 1** (full cold init); mode 0 (what the harness leaves) → state 3.

## 2. func_022016a0 — the 10-state boot machine (cart 0x022016AC, rt 0x060056AC)

Dispatch: `state = ctx[0x10]; if (state-1) u< 10 { braf table[state-1] } else exit`. Jump table
@0x02201738 (10 BE-halfword offsets off base 0x0220171A). State → handler and what it does:

| state | addr | action (key calls / transition) |
|---|---|---|
| 1 | 0x0220174C | coin-meter `*0x06000640`; **func_0201b92** (sound+panel+controller-present+config, cart 0x02201B92); ctx[0x10]=2 |
| 2 | 0x022017A0 | calls 0x02201C78; if ctx[8]+0x3C bit set → ctx[0x10]=3 |
| 3 | 0x022017CC | set ctx[8]+0x3C bit; coin-meter 0x06000640; service 0x06000688; ctx[0x10]=4 |
| 4 | 0x022017EC | call 0x06009B20; elapsed-time calc (two 0x06000640 reads); timing fns 0x0600E960/0x0600E992; service 0x06000688; if cfg(0x06000798) bit5 → BIOS `*0x0600060C` + **ctx[0x10]=13 (terminal/idle)** |
| 5 | 0x022018A8 | stores + bsrf init; clears ctx[8]+0x3C bit; ctx[0x10]=6 |
| 6 | 0x022018D8 | if ctx[8]+0x3C bit1 set → exit; else ctx[0x10]=7 |
| 7 | 0x022018E6 | bsrf; clears ctx[8]+0x3C bit; … |
| 8 | 0x02201906 | (boot-sequencer continuation) |
| 9 | 0x02201914 | (boot-sequencer continuation) |
| 10 | 0x02201934 | (boot-sequencer continuation) |

Observed cold-boot chain (mode 1): **1 → 2 → 3 → 4 → 13 (terminal)**. State 13 (>10) falls through
the dispatch to the exit each frame, i.e. the boot machine goes idle after init and hands off the
attract/play flow to the OTHER installed tasks. func_022016a0 is the **boot/cold-init sequencer**,
not the per-round gameplay loop; the attract demo / coin-wait live in the other tasks (notably the
attract handler 0x06008868, which runs in the warm/default path).

## 3. Minimal faithful reproduction (found)

**Write `0x060987CC = 1` before entering main_init_entry. That is the whole boot-state gap.** No
task-install code needs reproducing (frame_sync_loop already does it); the security-CRC stay stubbed
(not run, as before). Implemented flag-gated in port/main_stv.cpp:

```
STV_BOOTMODE=N  ->  st32(0x060987CC, N) just after install_bios_workarea(), before the jump.
```

Default (unset) writes nothing → **default behaviour byte-unchanged** (verified: default still runs
the attract with attract_handler + process_spawn_list, func_0201b92 skipped). FUN_02201958 and
frame_sync_loop never touch 0x060987CC, so the value survives to the boot controller on frame 1.

Autolearn (port/autolearn_boot.sh, new) on the mode-1 path learned **15 PIC-call seeds** in three
passes (deeper START inputs reach deeper code), after which the run is **clean to 6000 VBlanks**:
```
0600B2CE 0601F330                                   (cold-init, VB<=800)
06008308 06018A60 0602C106 06008258                 (+coin+start, VB<=2500)
06049E58 0604A0C8 06014858 0603594A 06035A1C
06045B2C 0602D4B4 06006DFA 0601DC44                 (+repeated start, VB<=6000; the 0x0604xxxx
                                                     round-path fns the prior "paid path" also hit)
```
=> the cold-boot gameplay path is **code-complete** in the recomp (no missing functions); the
blocker is game-state/rendering, not a wall.

## 4. Honest per-item results (STV_BOOTMODE=1, coin+repeated start, 6000 VB, 15 seeds)

| question | result | evidence |
|---|---|---|
| boot controller ran? | **YES** | COV 0x06005590 ran=1; it writes its ctx ptr to 0x060987B8 even in default |
| func_022016a0 states ran? | **YES** | COV 0x060056AC ran=1; chain 1→2→3→4→terminal observed |
| cold-init (func_0201b92) fired? | **YES (mode 1 only)** | COV 0x06005B92 ran=1; tell-tale 0x0609868F = 2; coin-enable 0x0609880E=1 set at VB10 |
| attract→play transition fired? | **PARTIAL** | reaches title "INSERT COIN(S)/CREDIT 0" then the attract gameplay-demo (combat HUD) and loops — the real *credited* play transition does not |
| START routed to round code? | **YES (into round code), NO (no round)** | repeated START drives into the 0x0604xxxx round fns (same as the paid path); but with no credit it is the demo path, not a player round |
| credit accrued? | **NO** | coin-pending 0x0609880C=1 (VB200) but credit 0x06098674 stays 0 — the accrual fn 0x060045F8 is called from attract_handler 0x06008868, which does NOT run on the mode-1 path (COV ran=0) |
| actors spawned? | **NO (static)** | process_spawn_list 0x0602E5D0 now ran=1 and spawn-ctx 0x06099580 inits to 0x060922C4, but slot-pool 0x0609958C cycles small and ends 0 — no live fighters |
| stage rendered? | **NO** | playfield black (VDP2 RBG1 unimplemented, known); the HUD (portraits+health bars, NBG/VDP1) DOES render |

Frames (port/run_bm1/ , port/run_bm1_final/ , and the Mac scratchpad, clear names):
`bootmode1_title_insertcoin_vb600.png` (title), `bootmode1_dusk_skyline_vb2000.png` (attract bg),
`bootmode1_combatHUD_healthbars_vb2400.png` + `bm1final_vb2500.png` (the gameplay-demo combat HUD
with character portraits + multiple health bars — black playfield), `bm1final_vb5000.png` (loops
back to INSERT COIN). These show a live, cycling attract loop — NOT frozen, NOT a player round.

## 5. THE VERDICT

**(b) a bounded multi-run effort — NOT a deep rabbit hole.** With one important caveat: the brief's
headline blocker ("reproduce the boot-task install so func_022016a0 runs") is **already solved** —
func_022016a0 runs; the install is done by frame_sync_loop; the only missing boot-state was one byte
(0x060987CC), now reproduced. That part was effectively a **small step** and is done.

Reaching a *visible, credited, player-controlled round* from here is bounded and localised:
1. **Credit accrual on the active path (~1 run).** The accrual logic (fn 0x060045F8) exists and
   provably works on the warm path; on the cold-boot path its caller (attract_handler 0x06008868)
   is not the one running. Need: let the attract state run 0x06008868 (it is installed; it is a
   state-reach issue), or route the coin into mode-1's own coin-poll. Low risk — logic is intact.
2. **Credited START → round-load (~1-2 runs).** START already reaches the round code (0x0604xxxx);
   gate it on a real credit so it loads a player round instead of the demo. Depends on #1.
3. **Actor spawning in a real round (~1-2 runs).** The spawn machinery now runs and its context is
   initialised; need to confirm the round actually populates the slot-pool (vs the demo's brief
   cycles). The genuine unknown — could be quick (a state flag) or need the demo/round data path.
4. **VDP2 RBG1 stage floor (the big one — multi-run rendering work).** Unimplemented in the runtime;
   required for a visible stage. Independent of the logic. This is the largest single chunk and the
   main thing between "HUD + logic running" and "a stage you can see." Secondary: confirm the actor
   VDP1 sprites draw once actors spawn (the HUD sprites already draw, so the sprite path works).

Why NOT (c): there are **no missing-function walls** (clean to 6000 VBlanks), the boot/attract state
machine is fully mapped and proven to run, every remaining gap is named and localised, and the recomp
code behind them is intact. The effort is real but bounded; the biggest piece is a known rendering
feature (RBG1), not a reverse-engineering mystery.

## 6. Files touched / new seeds / artifacts
- `port/main_stv.cpp` — `STV_BOOTMODE=N` (env-gated boot-mode write to 0x060987CC, default off,
  heavily commented) + `STV_COV` (env-gated: report whether key boot/play fns ran via recomp ran[]).
  Default run behaviour unchanged (verified).
- `port/autolearn_boot.sh` — NEW: autolearn loop that sets STV_BOOTMODE and feeds coin/start, to
  seed the cold-boot/gameplay PIC walls. usage: `autolearn_boot.sh [MODE] [MAX] [VB] [STVINPUT]`.
- `port/seeds_extra.txt` — +15 bootmode-1 seeds (listed in §3). Additive; harmless on the default
  (boot-mode-0) path.
- Frames in `port/run_bm1/`, `port/run_bm1_final/`, and the Mac scratchpad (names in §4).
- NOT modified: project/, tools/decomp/, notes/symbols.txt. Pre-existing run/watch12.log + diag_*.png
  left intact.

## 7. Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
cmake --build recomp-build --target saturn_stv -j"$(nproc)"
# cold-boot: full init + title + attract loop (func_0201b92 runs, combat-HUD demo renders):
mkdir -p run_bm1
STV_BOOTMODE=1 STV_COV=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
   --vblanks 6000 --stvinput "200:COIN,340:START1,800:START1,1500:START1,2500:START1,4000:START1" \
   --shots 600,2400,5000 --out run_bm1          # grep "COV " for ran[]; peek 060987CC/06098674/0609958C
# default path unchanged (warm/attract, func_0201b92 skipped):
STV_COV=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 1000 --out run
# re-seed the cold-boot gameplay path if deeper input is used:
./autolearn_boot.sh 1 40 6000 "200:COIN,340:START1,800:START1,1500:START1,2500:START1"
```
