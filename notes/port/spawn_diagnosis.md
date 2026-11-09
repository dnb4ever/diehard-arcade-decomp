# Die Hard Arcade (ST-V) RECOMP port — round-start freeze: ROOT CAUSE found

Date: 2026-10-19. Follows notes/port/freeze_diagnosis.md and rbg_roundstart_status.md.
Port/diagnosis side only. A flag-gated experiment **advanced the game past the freeze**
and proved the cause.

## TL;DR (the root cause, proven)

**The freeze is not a game-logic/data bug in the recompiled code, not the boot-bypass
skipping demo-arm, not sound, not RBG, not process_spawn_list, and not the "frozen
globals." It is the harness stubbing the entire ST-V BIOS service window
(0x06000600-0x06000700) to silent no-ops.** The game delegates its per-frame
control-panel / coin / input read to an ST-V BIOS service at **0x06000660** (called
~once per frame, 1089x in 1100 VBlanks). With that service no-oped, the read buffer at
**0x060987D8 is never filled** and the **coin/credit flag 0x06098674 and the player-1/2
start flags 0x06098688 / 0x06098689 are never set**, so the game can never leave the
attract loop into real gameplay. It sits in the attract/round-start fixed point — the
byte-identical frozen HUD.

**Proof (experiment):** forcing the P1-start flag once (0x06098688=1, flag-gated
`STV_STVBIOS=join`) drives the game straight out of the freeze into the real
game-start / continue sequence — the screen advances `INSERT COIN(S) / CONTINUE 9 -> 8`
(attract) then `HUD + portraits + CONTINUE 5 -> 2` (game start, counting down), frames
actively changing instead of frozen. It needed only **2** ordinary PIC-seed functions
(0x0604E1C8, 0x0604E120 = cart 0x0224A1C8/0x0224A120) to run cleanly — i.e. the code
behind the gate is intact; it was simply never reached.

This also explains the input-injection agent's finding ("the game never polls input / no
per-frame INTBACK"): the game DOES attempt a per-frame input read every frame, but via the
stubbed BIOS service, so no INTBACK is issued and the --input Saturn-pad script is never
consumed.

### Address-mapping correction (notes had this wrong)
Module base = 0x06004000, image base = cart 0x02200000, so **runtime addr = cart +
0x03E04000** (this is exactly what relearn.sh uses). The prior notes' "HWRAM = cart -
0x021FC000" is wrong. So process_spawn_list (cart 0x0222A5D0) is runtime **0x0602E5D0**,
NOT 0x0600E5D0 (off by 0x20000). Watch logs name functions in runtime space.

---

## Q1. Does process_spawn_list (cart 0x0222A5D0 / runtime 0x0602E5D0) execute? NO.

- **It never runs in the attract.** Watched the entity-struct region 0x06092000-0x06093800
  for a full 1100-VBlank run and attributed every store to its writer function: **0 writes
  from fn 0602E5D0**, 0 allocations attributable to it. Entities are created/updated
  entirely by the **task system** — the top writer is fn 06013DE4 = `task_list_run`
  (cart 0x0220FDE4), 24116 writes, plus the 0x06013xxx spawn_obj family.
- So process_spawn_list is **not the attract/round-start spawn mechanism.** (For the record,
  its disasm: `ctx->+0x30` is the spawn-script pointer, it reads `script+0x14` and exits at
  0x0222A6F0 if that is 0 — but this is moot, the function is never entered.)
- Entities *do* spawn and despawn normally during the attract via the task/slot system: the
  slot-pool active count at **0x0609958C** cycles 0->4->0 (VB30-441), 0->3->0 (VB443-693),
  then 0->1->0 (VB695-845), then stays 0. So two demo segments run with live actors; the
  stall is *after* them.

## Q2. What arms attract-DEMO, and does the boot bypass reach it? YES, it reaches it.

- **main_init_entry sets no game-mode enum.** Disassembled (cart 0x02201020): it clears
  globals (FUN_02201958), installs the 3 ISRs, programs SCU Timer-0, writes 0x0505 into
  `*(0x06098F08)+0xF0..0xF6` (0x06098F08 is a struct pointer = 0x06098DE8, set at boot),
  sets **0x06099578 = 1** (the EDSR-wait gate), then tail-branches frame_sync_loop.
- **The attract/demo DOES arm under our boot path** — proven by the actor cycling above
  (the pool spawns/despawns across two demo segments). The attract sequencer is a per-frame
  **task handler at cart 0x02204868** (runs via task_list_run every frame; it is the writer
  of the scene index, pr matches). So the boot bypass does **not** skip demo-arm.
- **=> The "bypass skipped demo-arm" hypothesis is DISPROVEN.** The demo plays; it stalls
  later, at the transition into populated gameplay.

## Q3. The "frozen" globals 0x060997A4 (=0x19) and 0x06099578 (=1) — both red herrings.

- **0x060997A4 is NOT frozen.** Watched: it is written **6696 times** in 1100 VB — a
  per-frame **working loop index** that sweeps a repeating value sequence every frame
  (writers fn 06027080 = cart 0x02223080, and 0603A3E4). The prior agent's "=0x19" was a
  single snapshot of a value that is constantly changing. 0x19 is just whatever it held at
  peek time.
- **0x06099578 = 1** is written **once**, by main_init_entry at boot (the EDSR-wait gate),
  and essentially never again. Not a scene gate.
- The real "state" is the scene-controller task (0x02204868) taking the same path each frame
  (a fixed point), not a stuck enum.

## Q4. BOOT/STUB EXPERIMENT — the cause, isolated and proven.

### The stubbed ST-V BIOS service window
`main_stv.cpp::install_bios_workarea` points the whole **0x06000600-0x06000700** window at
`kDefaultHandler` (0x2000) -> `bios_call` returns with r0 unchanged, **no side effects**.
That window is the ST-V BIOS service table the skipped PIC boot stub would populate from the
ST-V BIOS ROM (which we do not have). A silent no-op is **not a fault**, so the prior
"zero stubs/faults on the master" scans never flagged it.

Flag-gated logging (`STV_STVBIOS=log`, routes the window to a logging shim) shows which ST-V
services the running game calls in 1100 VB:

| service | calls | role (from call sites / disasm) |
|---|---|---|
| 0x06000660 | **1089** (per frame) | **per-frame control-panel / coin / input read**; buffer arg r4=0x060987D8; output byte drives dispatch table 0x06090304; 17 call sites across the scene controllers |
| 0x06000610 | 1093 (per frame) | per-frame, called from the VBlank ISR (role TBD: display/SCU) |
| 0x06000668 | 1093 (per frame) | per-frame, called from the VBlank ISR (role TBD) |
| 0x06000628 / 063C / 0640 / 0688 | 1 each | one-shot boot/setup services |

### Behavioral proof the input subsystem is dead
Watched 0x06098670-0x060987E0 for a full run:
- **0x060987D8 (the 0x660 read buffer): 0 writes.** Never filled.
- **0x06098674 (credit): only 0, at init.** Never a credit.
- **0x06098688 / 0x06098689 (P1/P2 start): only 0, at init.** Never set.

### The decisive experiment (positive — advanced the game)
Two flag-gated probes in the runtime bios.cpp (default-off, see "Files"):
- `STV_STVBIOS=fill` — fill the 0x660 buffer with arbitrary bytes (0x01/0x20/0xFF) every
  frame. Buffer now written 937x, but the game does **not** advance (pool stays 0,
  byte-identical frozen frame). => a non-empty buffer is not enough; **correct** panel data
  is needed, i.e. a real coin/start event.
- `STV_STVBIOS=join` — one-shot set the **P1-start flag 0x06098688 = 1** (what a real
  "press start" would set via the 0x660 service). **Result: the game immediately leaves the
  freeze and runs the game-start path** — at VB303 the scene controller's join block fires
  (sets credit 0x06098674=3, consumes the flag), and the screen advances through
  `INSERT COIN(S)/CONTINUE 9 -> 8` then `HUD+portraits/CONTINUE 5 -> 2`, frames changing.
  The path first hit two undiscovered PIC targets (0x0604E1C8, 0x0604E120); seeding those
  (ordinary autolearn) made it run clean. Frames saved (see deliverables).

**Conclusion:** the gate is the coin/start flag, which is dead solely because the ST-V BIOS
coin/input service (0x06000660) is a no-op. Open the gate and the real game flow runs.

## Q5. STRATEGIC — shortest path to on-screen gameplay.

- **The freeze is specific to the self-playing attract: the game is stuck in the attract
  loop because it can never register a coin/start.** It is not a demo-replay-script bug per se.
- **Both "fix the demo" and "wire coin-start" converge on the same work: implement the ST-V
  BIOS control-panel/coin service (0x06000660).** The --input Saturn-pad path is useless here
  — the game does not read the SMPC directly; it reads the BIOS service buffer. So coin/start
  can only be delivered by implementing that service (or by directly setting the coin/start
  flags, as the `join` probe does).
- **Recommended path: coin-start, via a real 0x06000660 service body.** It is the shorter
  route — it bypasses needing the self-playing demo to be faithful and lands directly in the
  real game-start flow (already demonstrated by the join probe). Concretely, next build:
  1. Implement 0x06000660 as an ST-V control-panel read: issue the runtime's existing
     INTBACK/pad model (smpc.cpp) and write the result into the r4 buffer **in the ST-V
     panel-report byte format**. The format is the one unknown — resolve it by tracing how the
     scene controller consumes 0x060987D8 (byte+3 -> dispatch table 0x06090304; find which
     index = coin / start) and/or cross-referencing MAME stv.cpp / saturn.cpp SMPC handlers.
     Also implement the per-frame 0x06000610 / 0x06000668 (ISR services) if they gate
     round completion.
  2. Extend --input with ST-V COIN/SERVICE/TEST/START buttons feeding that service (the
     input-injection agent already flagged the pad->panel format gap).
  3. Run the autolearn loop WITH a coin/start delivered (not the default harness) to seed the
     game-start code path (only ~2 new seeds surfaced in the first segment).
- The `join` probe already proves this will work; it is an engineering task, not a mystery.

---

## Files touched (all port-side, all flag-gated, default behavior unchanged & verified)
- `port/main_stv.cpp` — `install_bios_workarea`: when env `STV_STVBIOS` is set, re-point the
  0x06000600 window at the runtime sentinel base (0x4000+off) instead of the no-op; default
  (unset) keeps the original 0x2000 no-op. (Env-gated, 1 block.)
- `tools/saturn-recomp/runtime/src/machine/bios.cpp` — flag-gated ST-V service shim for the
  0x4000-0x4100 sentinel range: `STV_STVBIOS=log` logs each service call; `=fill` fills the
  0x660/0x66C buffer (value from STV_FILLVAL); `=join` one-shot sets 0x06098688 (VB from
  STV_JOINVB, default 300). Reached ONLY when main_stv routes the window there (i.e. only when
  the env is set). Default path byte-identical to baseline (verified: default frozen frame md5
  0ff787a8 == current post-RBG baseline).
- `port/seeds_extra.txt` — +2 genuine seeds 0604E1C8, 0604E120 (the game-start/join-path PIC
  targets). Real improvements; harmless on the default (closed-gate) path.
- **NOT modified:** anything under project/ or notes/symbols.txt (parallel agent's territory).

## Deliverable frames (in port/run/ and the Mac scratchpad)
- `diag_FROZEN_attract_hud_vb900.png` — the OLD freeze: static attract HUD, empty playfield.
- `diag_join_continue9_vb350.png` — join probe: INSERT COIN(S) / CONTINUE 9 (gate opening).
- `diag_join_gamestart_continue5_vb900.png` — game started: HUD + portraits + CONTINUE 5.
- `diag_join_continue2_vb1250.png` — still progressing: CONTINUE 2 (frames changing, not frozen).

## Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
cmake --build recomp-build --target saturn_stv -j"$(nproc)"
# the proof: force the P1-start flag once, watch the game leave the freeze:
STV_STVBIOS=join STV_JOINVB=300 ./recomp-build/saturn_stv --cart diehard_main.bin \
    --cartrom ../cart.bin --vblanks 1300 --shots 350,500,900,1250 --out run_join_final
# the input subsystem is dead (buffer/flags never written):
./recomp-build/saturn_stv ... --watch 06098670:060986A0     # 060987D8/74/88/89 stay 0
# which ST-V BIOS services the game calls (per frame: 0x610/0x660/0x668):
STV_STVBIOS=log ./recomp-build/saturn_stv ...               # grep "STVBIOS svc"
```
