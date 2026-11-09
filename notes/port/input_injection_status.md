# Die Hard Arcade (ST-V) — RECOMP port: peripheral input / past-attract status

Date: 2026-10-19. Follows `notes/port/render_status.md` (first rendered attract frame).

## TL;DR (honest)

- **The attract self-advances into a gameplay view with NO input at all.** Running the
  existing (no-input) harness past the old 600-VBlank stop, the master runs the attract
  state machine forward on its own: `INSERT COIN(S)` skyline → **the in-game gameplay HUD
  (two health bars + both character portraits)** around VBlank ~830–900 → then it **freezes
  on one static HUD frame from ~VBlank 1200 and stays byte-identical through VBlank 3600+**.
  This is the **first time VDP1 sprites have ever rendered** in this port (render_status had
  0 VDP1 commands for all 600 frames; the HUD bars/portraits are VDP1).
- **Injecting coin/START does nothing, and cannot, in the current reachable state** — because
  the game **never reads any controller/coin input** on the path it is on. Verified three
  independent ways (below). The thing blocking real gameplay is **not missing input**, it is a
  **logic stall in the demo/attract state handler** (no missing-function fatal — a clean hang).
- **The runtime has no ST-V coin/service model at all** — it is a pure Saturn digital-pad
  model. I wired the harness's scripted-pad path (`--input`) anyway (it is the correct hook and
  will be needed once the stall is broken), but on ST-V a *coin* is not a pad button, and the
  game is not polling even the pad.

## 1. How the ST-V / this game reads input — what is real

Address map (runtime `machine/mmio.cpp`, normalized 27-bit): the **SMPC is at
`0x00100000..0x0010007F`** (the cache-through `0x20100000` view the game uses). There is **no
ST-V I/O gate array (IOGA) mapped** — a 4-byte read of `0x00400000` would `sat_fatal` "nothing
there".

**The game never touches the IOGA.** I scanned the full Ghidra catalog (`notes_catalog.tsv`,
~3538 functions): **no function anywhere references `0x00400000`/`0x02400000`** (the MAME
`stv_ioga` control region). Every input-ish access in the whole cart is SMPC.

The one SMPC "input" routine, `smpc_intback_read` @`0x0220acf6`, is **mislabeled in
`engine_notes.md`** — it is **not controller input, it is an RTC/status read**. Disassembled
(cart off `0x20acf6`): it issues INTBACK with `IREG0=0x01` (status enable), `IREG1=IREG2=0xF0`
(**peripheral bit `0x08` CLEAR → peripheral data NOT requested**), waits `SF`, reads `OREG3`
and returns `OREG3 >> 4`. In status mode `OREG3 = (weekday<<4 | month)`, so it returns the
**day-of-week** (used for the RNG seed / clock). It runs ~3× at boot only.

**No in-cart function reads `PDR1` (`0x20100075`, direct-mode controls).** The only direct-port
code is `smpc_pdr2_set/clr` @`0x02214970/80`, which only *writes* `PDR2` (`0x20100077`) — those
are the ST-V **coin-counter / lockout outputs**, not inputs.

Conclusion: the game's **per-frame controller/coin read is not present on the reachable path**.
On real ST-V the control panel (P1/P2 + coin/service/test/start) is reported by the SMPC in the
INTBACK *peripheral* bytes; the task handler that issues a *peripheral* INTBACK (or reads the
mirrored pad state) lives in the game's per-frame executive, which only runs once the attract
state machine reaches a coin-wait state — and it never does, because of the stall in §3.

### What the hardware log shows (confirmation)
`port/run/hw-log.txt` over the full run — the ONLY SMPC accesses, all of them boot INTBACK:
```
W 00100001/03/05 (IREG0/1/2) ·  W 0010001F x3 (COMREG) ·  R 00100027 x1 (OREG3) ·
R 00100063 x8 / W x3 (SF) ·  W 00100077 x4 (PDR2 output)
```
No `0x00100075` (PDR1) read, no `0x00400000` (IOGA) access, no per-frame INTBACK. 3 COMREG
writes in 1800 VBlanks = boot only, never continuous polling.

## 2. The scripted input path I added (real, but currently inert)

`port/main_stv.cpp`: added `--input "VBLANK:BUTTON+BUTTON,..."` (and set `g_cfg.input`), which
calls the runtime's own `smpc_input_script()` — identical to how `saturn_main` (machine.cpp)
installs a pad script. `smpc.cpp` then reports those presses through the SMPC peripheral (INTBACK
OREG) and the PDR1 direct read. Buttons: `RIGHT LEFT DOWN UP START A B C R X Y Z L` (and `N.` for
pad N). Built clean. This is the Saturn digital-pad model — **there is no coin/service bit**.

### Empirical test (the decisive one)
Ran 1800 VBlanks with a dense script hammering `START+A` every 20 VBlanks across VBlank
60..1750, with frame captures at 300/900/1200/1799:
```
--input "60:START+A,70:,80:START+A,90:, ... ,1740:START+A,1750:"
```
**Every captured frame is BYTE-IDENTICAL (md5) to the no-input baseline** at 300, 900, 1200 and
1799. Moreover the script's pad steps are **never consumed** (smpc.cpp emits no `pad:` notes),
because `pad_now()` is only reached when the game actually reads the pad — and it never does.
So: injection reaches the runtime correctly, but the game is not listening. Confirmed inert.

## 3. The real wall: a demo-state logic stall (NOT input, NOT a missing function)

Self-driven progression (no input), captured frames:
| VBlank | Frame | md5 |
|---|---|---|
| 300, 600 | `INSERT COIN(S)` + night-city skyline (VDP2 only) | c79a…, ac32… |
| 900 | **gameplay HUD**: 2 health bars + both character portraits over skyline (VDP1 sprites) | 61f6… |
| 1200, 1500, 1799, 2400, 3000, 3599 | HUD with **full green health**, static — **frozen, all identical** | 7bdb… |

The master ends every run in `frame_sync_loop` (cart `0x022013c4`; HWRAM pc `0x060054BA`).
Interrupts keep firing (VBlank-in 40, VBlank-out 41, Timer-0 43 — e.g. 43:3297 over the run) and
the VBlank counter advances to 3600, so **the frame loop is NOT dead-locked** — each frame
completes, the game logic just stops changing the scene.

Disassembly of `frame_sync_loop`: it pushes display commands (5, then 7) into a list via PIC
`bsrf` thunks, then spins (final spin @`0x022014BA`) on **bit 7 of the byte at `0x060E0DD8`**
(read through the `0x26…` mirror). `0x060E0DD8` sits **immediately adjacent to the sound
command-ring block** (`engine_notes.md`: read idx `0x060E0DE8`, write idx `0x060E0DEC`, base
`0x060E0DF0`). That spin *does* release each frame (frames complete), so it is the normal
per-frame sync — but it points at the sound/command subsystem, which is the **prime suspect**
for why the demo state handler stops advancing:

- The **slave SH-2 sound worker is stubbed to a no-op** (render_status: `cpu==1` unknown-call →
  `pc<-pr`). The attract **demo-play timeline is very commonly gated on sound/music cues**
  (e.g. "advance when the intro jingle finishes"). With the sound driver a no-op, that cue never
  arrives and the demo sits on its first HUD frame forever.
- Secondary: VDP2 **rotation backgrounds are unimplemented** (`VDP2: RBG0/RBG1 (0119) is not
  done` warning) — not the stall cause, but it means the demo-gameplay background is incomplete.

Either way, the game **never returns to a coin-wait state**, so it never polls input, so a coin
can never be accepted on this path.

## 4. Seeds learned this session (8 new, 58 → 66)

The old autolearn converged at 600 VBlanks only because its budget was 60/600. Re-running
`./autolearn.sh 30 1800` (budget 1800) knocked down the walls that only appear once the attract
advances past ~VBlank 830 into the gameplay-HUD scene, then converged again ("no learnable
non-entry", clean 1800-VBlank stop):
```
060141CE  0604A052  060118D0  0601198E  0604A3B8  06017354  06013B98  06032EE4
```
These are the real PIC indirect-call targets reached by the demo/gameplay-HUD path (health-bar /
portrait / VDP1-sprite build). All appended to `port/seeds_extra.txt`.

## 5. Files touched (all under `port/`, per the task's constraint)

- `port/main_stv.cpp` — added `--input "VBLANK:BUTTONS,..."` (set `g_cfg.input` +
  `smpc_input_script`). No version marker in the file → minimal edit, no version bump.
- `port/seeds_extra.txt` — +8 seeds (66 total).
- No runtime `.cpp`/`.h` changed this pass. No `tools/decomp/*` touched.

Capture dirs: `port/run1800b/` (1800-VBlank, no input), `port/run3600/` (3600, confirms the
freeze is permanent), `port/runinput/` (1800 with the START+A barrage — identical to baseline).

### Deliverable frames (copied to `port/run/` and the Mac scratchpad)
- `attract_insertcoin_vb300.png` — the attract `INSERT COIN(S)` skyline.
- `gameplay_hud_vb900.png` — **the gameplay HUD** (health bars + portraits; first VDP1 sprites).
- `gameplay_hud_frozen_vb1200.png` — the frozen full-health HUD frame (stable to VBlank 3600+).

## 6. Next concrete wall / recommendation

1. **Break the demo stall first — it is the gate, not input.** Prime hypothesis: the stubbed
   slave/sound worker. Test by instrumenting the demo-state handler's wait: `--watch
   060E0DD0:060E0DF4` to see who writes the sync/ring bytes and what the handler compares the
   demo timer / sound-done flag against. If it waits on a sound-ring drain or a music-ended flag
   that only the (stubbed) slave sets, that is the fix target — either bring up a minimal sound
   acknowledge or satisfy the specific flag.
2. **Only then does input matter.** Once the game reaches a coin-wait loop it WILL issue a
   *peripheral* INTBACK (or read mirrored pad state). At that point:
   - START and the pad buttons already work via the `--input` path just added.
   - **COIN needs a runtime change**: `smpc.cpp` currently reports a stock Saturn digital pad
     (ID `0x02`, bytes `RIGHT LEFT DOWN UP START A C B` / `R X Y Z L 1 1 1`) with **no
     coin/service bit**. ST-V reports the arcade control panel (coin1/2, service, test, start1/2,
     P1/P2) in the SMPC peripheral bytes with a different ID/layout. Making coin work means
     extending the INTBACK peripheral report (and ideally `--input` button names COIN/SERVICE/
     TEST) to the ST-V control-panel format — cross-check MAME `machine/saturn.cpp` / `stv.cpp`
     SMPC `pdr1/pdr2`/peripheral handlers for the exact byte layout before implementing.
   - I did **not** add speculative ST-V coin plumbing this pass: with the game not polling input
     at all in the reachable state, it would be untestable dead code. Flagging rather than
     guessing.

### Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
./relearn.sh                                   # regen recomp (66 seeds) + build
# self-driven attract → gameplay HUD → freeze:
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 1800 --shots 300,900,1200,1799 --out run1800b
# scripted pad (currently inert — game not polling):
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 1800 --input "60:START,70:,120:START+A,130:" --shots 900,1200 --out runinput
```
