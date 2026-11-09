# Die Hard Arcade (ST-V) RECOMP port — freeze diagnosis

Date: 2026-10-19. Follows `notes/port/input_injection_status.md`. Port side only
(`port/` + `tools/saturn-recomp/`). The question: the demo boots to a gameplay-HUD
frame then freezes byte-identical from ~VB1000; what is it waiting on, who sets it,
and is the stubbed sound slave the cause?

## TL;DR (honest)

- **The freeze is NOT the slave/sound sync, and NOT a stubbed cue.** The prior
  hypothesis ("the stubbed slave never signals a sound-done cue, so the demo waits
  forever on bit7 of 0x060E0DD8") is **REFUTED by three independent experiments.**
- The spin at `0x022014BA` (HWRAM `0x060054BA`) waits on **bit7 of `0x060E0DD8`**,
  which is a **slave-idle / command-ring-drained handshake flag**. The flag **IS
  being set** — by the **SLAVE SH-2 itself** (`FUN_022011b0` @ `0x060051B0`), not by
  the 68000/SCSP and not by the master. The master **passes the wait every frame**
  (it clears the flag 1791× over 1800 VBlanks, last at VB1798). The master is
  **cycling `frame_sync_loop` for the whole run — it is not spin-hung.**
- The real wall is a **game-logic stall at round/stage start (~VB950→1000)**: the
  master keeps running its per-frame executive, but the game state stops advancing.
  The scene is the **HUD (two health bars + both portraits) over the VDP2 skyline,
  with no gameplay characters and no stage floor** — the playfield never populates.
- The **only** unimplemented runtime feature touched anywhere in the run is
  **VDP2 RBG0/RBG1 (rotation BG = the beat-em-up stage floor), logged once as "not
  done".** That is the prime remaining suspect for why the stage/actors never appear
  and (likely) why round-start never completes. This is a **game/VDP2 issue, not a
  sound issue.**
- **A minimal sound-done shim will NOT unblock the demo. Full slave-SH2/SCSP sound
  bring-up is NOT required to get past this wall either** — I brought the slave sound
  path up to zero stubs and it changed nothing about the freeze.

## 1. The exact polled condition at 0x022014BA

Disassembly of `frame_sync_loop` (`FUN_022013c4` @ cart `0x022013c4` = HWRAM
`0x060053C4`; disasm from `diehard_main.bin`, SH-2 big-endian). The loop has **three**
identical producer/consumer handshakes with the slave; the one the master always ends
on is the third, at HWRAM `0x060054BA`:

```
; r11 = 0x20000000 (cache-through bit), r13 = 0x80 (bit7 mask), r14 = 0
060054ba:  mov.l  r3 <- &0x060E0DD8
060054bc:  add    r11,r3            ; r3 = 0x260E0DD8  (cache-through mirror)
060054be:  mov.b  @r3,r2            ; read the byte
060054c0:  and    r13,r2            ; & 0x80
060054c2:  cmp/eq r13,r2            ; == 0x80 ?
060054c4:  bf     0x060054ba        ; spin until bit7 is SET
060054c6:  mov.l  r2 <- &0x060E0DD8
060054ca:  mov.b  r14,@r2           ; then CLEAR it: *0x060E0DD8 = 0
```

C equivalent: `while ((*(volatile u8*)0x260E0DD8 & 0x80) != 0x80) {} *(u8*)0x060E0DD8 = 0;`

This is the consumer side of a handshake: **"wait until the slave is idle (bit7 set),
then clear it and push the next display command."** The other two sites are
`0x0600542A` and `0x06005488`. All three poll the same byte the same way.

## 2. Who sets 0x060E0DD8 — MASTER vs SLAVE/sound

Watched with `--watch 060E0DD8:060E0DDB`. Every write that **sets** bit7 is:

```
store8 060E0DD8 = 80  in fn 060051B0 (cpu 1, pr 06005202, VB ..)   [millions of times]
```

- **The setter is the SLAVE SH-2** (`cpu 1`), inside `fn 060051B0` = `FUN_022011b0`,
  the slave's command-ring worker. Disassembly of that worker shows it writes
  `0x80` to `0x060E0DD8` (`mov #-128,r10; mov.b r10,@r8`) **whenever its command ring
  is drained** (read-ptr == write-ptr at `0x060E0DD0`/`0x060E0DD4`). It is a hot
  infinite loop (it also free-runs a counter at `0x060987C4`, which reached ~1.0M in
  900 VBlanks) — it never blocks, and re-asserts "idle" continuously.
- The **master** only ever writes `00` to `0x060E0DD8` (the clear, cpu 0). It is **not**
  the setter. The **68000/SCSP never touch this byte.**

So the handshake is master-clears / slave-sets, entirely between the two SH-2s. The
slave signals "ready" constantly; **the master is never starved of the signal.**

## 3. Hypothesis test — is it the stubbed sound slave? NO.

Three experiments, each decisive:

**(a) The slave is only barely stubbed, and un-stubbing it fully changes nothing.**
With `STV_SLAVE_LOG=1`, the slave's no-op'd (un-recompiled) calls were only **three**
unique targets: `060177A0` (×12631 from `06019C46`), `060171FC` (×2369 from
`0602BE1A`), `06019386` (×2 from `06005262`). The slave's handler table at
`0x06090274` is **fully populated with real pointers** (`0600521E … 0604EFB8`); the
handlers themselves run. I **seeded those three functions** (added to
`port/seeds_extra.txt`, relearned → 69 seeds / 980 funcs). Result: **zero slave
no-op calls remain — the slave sound path now runs with no stubs at all — and the
demo still freezes at the byte-identical frame (`7bdb…`) from VB1000.** The VB900
frame barely changed (`ced4f6…` vs `61f6…`, a few HUD/skyline pixels). → Sound/slave
bring-up does not unblock the demo.

**(b) The master already passes the wait, every frame.** `--watch 060E0DD8` filtered
to cpu 0 (`STV_WATCH_CPU=0`): the master writes `00` (the clear) **1791 times over
1800 VBlanks, first at VB8, last at VB1798.** It cycles `frame_sync_loop` across the
entire freeze; it is **not** stuck in the spin.

**(c) Deleting the wait entirely changes nothing.** `--spoof-sound-done` NOPs all
three `bf` spins (`0600542A/06005488/060054C4`) so the master **never** waits on
`0x060E0DD8`. Re-run: **byte-identical** output — VB900 `ced4f6…`, VB1000-1799 the
same frozen `7bdb…`. The 0x060E0DD8 wait is conclusively **irrelevant** to the freeze.

## 4. What the freeze actually is

- Frame-md5 timeline (sound-seeded build): frames change through **VB950**
  (`65896a…`, last "alive" frame), then lock to `7bdb…` from **VB1000** and stay
  byte-identical to VB1799 (and, per prior runs, to VB3600+). Freeze onset is
  **between VB950 and VB1000.**
- Visually (see frames below): at ~VB900 the two health bars show **partial
  depletion**; by VB1000 they are **refilled to full green** — i.e. a **new
  round/stage is (re)started**, the fresh HUD is drawn, and then everything locks.
  The **playfield is empty** the whole time: VDP2 skyline NBG + HUD VDP1 sprites
  only; **no character sprites, no stage floor.**
- The master returns to `frame_sync_loop` every frame (its PC at every sample is the
  spin `0x060054BA`), so the game-logic executive **runs to completion each frame and
  is not internally hard-spinning** — it is simply in a **state-machine fixed point**:
  it keeps doing the same thing because some round-start condition never becomes true.
- The **only** unimplemented-feature warning in the entire run is
  **`VDP2: RBG0/RBG1 (0119) is not done`** (fired once). No DMA errors, no master
  stubs, no fatals, no missing functions. Die Hard Arcade's stage floor (the
  scaled/rotated ground the characters stand on) is an RBG; with RBG unimplemented the
  stage never appears, and round-start plausibly gates on the stage/actors being
  placed.

## 5. Recommendation

1. **Do NOT build a sound shim for this.** It is confirmed not the cause; the slave
   already signals idle and the sound path is now stub-free.
2. **The next wall is game-logic / VDP2-RBG, not sound or input.** Investigate why the
   round/stage-start at ~VB950-1000 does not populate the playfield. Concretely:
   - Implement (or stub-with-status) **VDP2 RBG0/RBG1** in the runtime
     (`tools/saturn-recomp/runtime/src/video/vdp2.cpp`) — it is the one flagged gap and
     the stage floor depends on it. Even a non-rendering RBG that reports correct
     status may be enough if the game polls RBG/coefficient-table readiness.
   - Trace the **actor/stage spawn** on the master side (this overlaps the parallel
     functional-decomp agent's territory): find the round-start state variable and what
     it waits on. Candidate gates to check next: a **VDP1 draw-end/EDSR** status the
     object loop polls, a **SCU-DMA done** flag for stage/character graphics, or a
     demo-replay script pointer read from the cart. Use `--watch` on the relevant
     game-state RAM once the functional side identifies the round-start handler.
3. **Input is still downstream of this** (the game still never reaches a coin-wait on
   this path), exactly as `input_injection_status.md` concluded.

## 6. Experiments / instrumentation added this session (all port-side, reversible)

- `port/seeds_extra.txt`: **+3 seeds** `060177A0 060171FC 06019386` (the slave sound
  sub-functions). **Keep** — a genuine improvement: the slave sound path is now
  recompiled with **zero no-op stubs**.
- `port/main_stv.cpp`:
  - `STV_WATCH_CPU` env (diag): `--watch` reports only cpu 0 or cpu 1. Harmless, off
    by default.
  - `--spoof-sound-done` (EXPERIMENT, off by default, clearly commented): NOPs the
    three `frame_sync_loop` spins on `0x060E0DD8`. Kept behind the flag as documented;
    the finding is that it changes nothing.
- `tools/saturn-recomp/runtime/src/machine/machine.cpp`: `STV_SLAVE_LOG` env-gate on
  the slave no-op log line (prints every target instead of first 8). Default-off =
  original behavior.

Nothing runtime-default changed; no misleading dead code left.

### Deliverable frames
Copied to `port/run/` and the Mac scratchpad:
- `freeze_last_alive_vb950.png` — the last frame that still changes (full-health HUD,
  empty playfield).
- `freeze_vb900_soundseeded.png` — ~VB900, health bars partially depleted (the demo
  was still progressing here).
- `freeze_frozen_vb1200.png` — the permanent frozen frame (full-health HUD over the
  skyline, no stage/characters).

### Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
./relearn.sh                                   # 69 seeds (incl. the 3 slave sound fns)
# the freeze, with the slave sound path fully un-stubbed:
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 1800 --shots 900,950,1000,1200,1799 --out run_seed
# prove the 0x060E0DD8 wait is irrelevant (identical output):
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 1800 --spoof-sound-done --shots 900,1000,1200,1799 --out run_spoof
# who sets the flag (slave) / who clears it (master):
STV_WATCH_CPU=1 ... --watch 060E0DD8:060E0DDB   # slave writes 80
STV_WATCH_CPU=0 ... --watch 060E0DD8:060E0DDB   # master writes 00, 1791x to VB1798
# all slave no-op targets:
STV_SLAVE_LOG=1 ... 2>log ; grep -oE "no-op call to [0-9A-F]+" log | sort | uniq -c
```
