# Die Hard Arcade (ST-V) RECOMP port — RBG0 + round-start stall

Date: 2026-10-19. Follows `notes/port/freeze_diagnosis.md`. Port side only
(`port/` + `tools/saturn-recomp/`). Two prongs: (A) implement VDP2 RBG0 (the
stage floor), (B) trace what the round-start state machine stalls on and decide
whether that stall is RBG-dependent or an independent gate.

## TL;DR (honest)

- **Prong B verdict: the round-start stall is an INDEPENDENT game-logic/data
  gate, NOT RBG-dependent, and NO runtime status-stub can clear it.** There is no
  infinite hardware-status spin anywhere at round-start — every candidate gate
  (VDP1 draw-end/EDSR, SCU-DMA-done) is either satisfied in-run or bounded by a
  timeout. The master runs its full per-frame executive every frame; it believes
  it is *in gameplay* but **spawns zero actors and never loads the near-field
  stage data**. The byte-identical frozen frames are a *consequence* of nothing
  render-relevant changing, not a wait on a flag. This is a failure in the
  attract-**demo** sequencer (actor-spawn / demo-script), which is the parallel
  functional-decomp agent's territory — not a port/runtime gate.
- **Prong A: RBG0 is implemented** in `vdp2.cpp` (rotation parameter A/B, the full
  matrix, viewpoint/centre, screen-start + per-line/per-dot deltas, and the
  coefficient table for Kx/Ky), composited as its own priority layer. The
  `VDP2: RBG0/RBG1 is not done` warning is **gone**. The NBG/sprite/VDP1 paths are
  **provably untouched** (flat-K output is byte-identical to the pre-RBG frozen
  frame, md5 `7bdb…`).
- **RBG0 does NOT clear the freeze — exactly as Prong B predicted.** And on this
  specific frozen demo frame RBG0 cannot draw a correct floor, because **the
  game's own rotation parameters are incomplete** (the stage-load stalled before
  finishing them): `My` ≈ 1964 (off-map) and the **coefficient table is empty
  (all-zero) throughout VRAM**. So the floor data the renderer needs was never
  produced. This is the same root cause as the actor/stage failure in Prong B.

## 1. What the game actually programs into VDP2 for RBG0 (dump at VB1000)

From `--dump 1000` (parsed from `run_probe/dump-1000.bin`, since removed). RBG0 is
**ON** and uses **rotation parameter A** (RPMD = 0):

```
BGON   = 0119   N0ON + N3ON + R0ON (bit4) + N0TPON  -> NBG0 (skyline) AND RBG0 (floor) both on
CHCTLB = 0010   RBG0: 1x1 cells, cell mode (not bitmap), 16 colours
PNCR   = C003   RBG0 pattern name: 1-word, aux-supplement mode, supp=0x003
PLSZ   = 0840   RAPLSZ=0 (1x1-page planes)
MPOFR  = 0000
RPMD   = 0000   parameter A only
RPRCTL = 0000   apply per-line dXst/dYst
KTCTL  = 0011   RAKTE=1 (coefficient table enabled, param A), mode 0, 2-word data
KTAOF  = 0001   coefficient table addr offset (param A) = 1
RPTAU/L= 0002 / F800  -> rotation param table at VDP2 VRAM byte 0x5F000
PRIR   = 0004   RBG0 priority = 4 (above NBG0=3, below sprite=5)
CRAOFB = 0002   RBG0 colour-RAM offset = 0x200
```

Rotation parameter A (at 0x5F000):
```
Xst=0 Yst=0 Zst=0   dXst=0 dYst=1.0 dX=1.0 dY=0
matrix A=0.99997 B=0 C=0  D=0 E=0.2533 F=0.9674     (a Y-axis scale, no shear/rotate)
Px=176 Py=120 Pz=256   Cx=176 Cy=120 Cz=256         (viewpoint == centre)
Mx=96.0   My=1964.7  (<- STALE/garbage, pushes Y off the visible map)
kx=1.0 ky=1.0
KAst=0x7C80 (int)  -> coefficient table region is EMPTY (all-zero everywhere in VRAM)
```

RBG0 4×4 plane map (A..P): row A-D = plane 0x00; **row E-H = plane 0x38 @0x70000
(floor tiles present)**; rows I-P = plane 0x2E @0x5C000 **(EMPTY — never filled)**.

**Conclusion from the register state:** the matrix and the E-H tile row are set up,
but `My`, the coefficient table, and the lower plane rows are stale/empty — i.e.
**stage setup stalled partway**. A correct floor is impossible from this state
regardless of renderer quality.

## 2. Prong A — what was implemented (`runtime/src/video/vdp2.cpp`)

A real RBG0 path, modelled on the existing NBG machinery and the file's existing
sprite rotation-parameter reader:

- `struct Rbg` + `setup_rbg()` — reads BGON/CHCTLB/PNCR/PLSZ/MPOFR, the 16-plane
  map (MPABRA..MPOPRA), CRAOFB/PRIR/CCRR/CCCTL, and the rotation parameter table
  (matrix A–F, Px/Py/Pz, Cx/Cy/Cz, Mx/My, kx/ky, Xst/Yst/Zst + deltas) with the
  documented fixed-point formats, plus the coefficient-table controls (KTCTL/KTAOF).
  Precomputes the manual's constant screen offset Xp/Yp.
- `rbg_coef()` — reads a 1-word or 2-word coefficient entry (top-bit = line
  transparent), mode-aware (Kx/Ky/both).
- `rbg_pixel()` — the manual's screen-coordinate transform per dot
  `X = Kx·(Xsp + dXd·H) + Xp`, `Y = Ky·(Ysp + dYd·H) + Yp`, then a 4×4-plane /
  page / cell / pattern-name lookup that reuses the (now templated) `dot()` helper.
- Integrated into `vdp2_compose()` as layer **L_RBG0**, considered right after the
  sprite (so it wins priority ties over the NBGs, loses to the sprite), with its
  own window (WCTLC low byte), line-colour and shadow (SDCTL bit4) handling.
- `note_unsupported()` now flags only **RBG1** (bit5), not RBG0.
- **Diagnostic env `STV_RBG_FLATK=1`** forces Kx=Ky=1 (ignores the coefficient
  table) to visualise the raw affine tilemap. Default (unset) is faithful.

Honest limits (documented in-code): RBG1 is still unimplemented; RBG0 bitmap mode,
screen-over modes beyond repeat, RBG0 gradation/line-colour, and coefficient
line-colour data are partial. **The coefficient-table VRAM addressing is
best-effort and UNVALIDATED** — this capture has no valid coefficient data to check
against (the table is empty), so it needs a healthy RBG game to confirm.

This is a general Saturn feature (gated only on BGON bit4), not an ST-V hack, so it
is not env-gated; disc games that use RBG0 now get a rotation layer instead of a
missing floor. Non-RBG games are unaffected (`setup_rbg` early-outs).

## 3. Results — RBG0 active, freeze unchanged (as predicted)

Rebuilt headless (`cmake --build recomp-build --target saturn_stv`), ran 1200 VB.

| frame | pre-RBG baseline | RBG0 flat-K | RBG0 faithful |
|---|---|---|---|
| VB1200 md5 | `7bdb…` (frozen) | `7bdb…` (identical) | `0ff7…` (differs) |

- **flat-K == baseline, byte-identical** → the NBG/sprite/VDP1 compositor is
  untouched; RBG0 contributes nothing here because `My` is off-map (samples the
  empty plane rows). Proves no regression.  (`run/rbg0_flatk_clean_vb1200.png`)
- **faithful differs** → the empty coefficient table yields K=0, collapsing RBG0
  to near-single-texel sampling; it shows as a faint dotted band over the skyline
  and shifts frame-to-frame because the game rewrites the rotation parameter table
  (0x5F000) every frame (a live-but-idle in-game camera). HUD (sprite) and skyline
  (NBG0) still render correctly over/under it. (`run/rbg0_faithful_vb1200.png`)
- **The state machine is still frozen** (last visually-advancing frame ~VB950;
  locked from ~VB1000). master pc still cycles `frame_sync_loop`. RBG0 did not and
  could not unblock it.

## 4. Prong B — the independent gate, with evidence

No round-start hardware-status spin exists. Candidates checked and ruled out:

- **VDP1 draw-end/EDSR** — wait routine at HWRAM `0x06018F90`, EDSR CEF poll at
  `0x06018FAC` (reads VDP1 EDSR `0x25D00010`), **gated by `*0x06099578`** and
  satisfied in-run (EDSR = 0x0003 at VB940 and VB1700). Exits immediately.
- **SCU-DMA-done** — routines `0x06018790` / `0x06018840` poll SCU DSTA
  (`0x25FE007C`) but are **bounded by a timeout counter** (cmp/ge + inc) — they
  cannot hang.
- No other VDP1/VDP2/SCU status is polled in a loop. A full 1100-VB master run has
  **zero no-op / missing-fn / fatal** — the stage-load path executes cleanly.

What is actually wrong (data/actors, not a flag):
- **No gameplay actors ever spawn** — the VDP1 command list stays HUD-only
  (~120–130 cmds, varying, so VDP1 build is alive; but no character sprites are
  ever added).
- **Near-field stage data never loads** — RBG0 plane 0x2E (@0x5C000, map cells
  I–P) is empty at both VB940 and VB1700; the coefficient table is all-zero
  everywhere, forever.
- **Scene-state globals are frozen** — `0x060997A4`=0x19, `0x06099578`=1,
  `0x060997A8/AC`=1 are identical VB960 vs VB1010. The only VDP2-VRAM region that
  changes is the rotation param table at 0x5F000 (the idling camera).

So the engine entered gameplay from the attract demo but the **demo sequencer
spawned no characters and never completed the stage load**. That is a functional
decomp problem, not a port/runtime gate. **No status-stub would help.**

### Next concrete blocker (hand-off to the functional-decomp side)
- Scene/demo controller region HWRAM `~0x06007500–0x06008900` (writers of
  scene-state `0x060997A4`=0x19).
- `process_spawn_list` @ cart `0x0222A5D0` (HWRAM `0x0600E5D0`) — verify whether it
  is ever invoked with a non-empty spawn script during the attract demo; if it
  runs on an empty/misread script, that is the root of "no actors".
- The demo-replay input/script source flagged in `freeze_diagnosis.md`.

Useful addresses recorded: `0x06018F90` (EDSR wait), `0x06018FAC` (EDSR poll),
`0x06099578` (gates the wait, =1 in-game), `0x06018790`/`0x06018840` (bounded SCU
DMA waits), `0x060997A4` (scene-state =0x19), VDP2 VRAM `0x5F000` (RPTA, rewritten
every frame). Note: the table at `0x0604EBD8` is a BITMASK table, not code
pointers, so `*0x060987CC` is NOT the scene dispatcher — disregard that earlier
lead. HWRAM addr = cart SH-2 addr − 0x021FC000.

## 5. Files touched (port side only)

- `tools/saturn-recomp/runtime/src/video/vdp2.cpp` — RBG0 implementation
  (`Rbg`/`setup_rbg`/`rbg_coef`/`rbg_pixel`, templated `dot()`, L_RBG0 compositing,
  `note_unsupported` now flags only RBG1, `STV_RBG_FLATK` diag env). Updated the
  file header comment.

No new recomp seeds were learned (none needed — the master stage-load path already
runs with zero stubs/faults). `port/seeds_extra.txt` unchanged.

### Deliverable frames (in `port/run/` and the Mac scratchpad)
- `rbg0_flatk_clean_vb1200.png` — RBG0 flat-K; byte-identical to the pre-RBG frozen
  frame (proves NBG/sprite untouched).
- `rbg0_faithful_vb1200.png` — RBG0 faithful; the rotation layer composites (faint
  dotted artifact from the game's empty coefficient table). Nothing advanced past
  the freeze — these document the RBG0 layer, not a progression.

### Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
cmake --build recomp-build --target saturn_stv -j"$(nproc)"
mkdir -p run_rbg
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 1200 --shots 900,1000,1200 --out run_rbg          # faithful
STV_RBG_FLATK=1 ./recomp-build/saturn_stv ... --out run_rbg_flatk  # affine-only
```
