# Die Hard Arcade (ST-V) RECOMP port — RBG1 (second rotation BG / stage floor sibling)

Date: 2026-10-19. Port side only (`tools/saturn-recomp/runtime/src/video/vdp2.cpp`).
Follows and mirrors `notes/port/rbg_roundstart_status.md` (the RBG0 implementation).
Done in an isolated copy `~/decomp/dh-portB/` to avoid racing the two concurrent agents.

## TL;DR (honest)

- **RBG1 is implemented** in `vdp2.cpp`, per spec, as a sibling of RBG0: its own
  `setup_rbg1()` reading the RBG1 register set, composited as its own priority
  layer `L_RBG1`. It reuses the RBG0 renderer verbatim (`rbg_pixel`/`rbg_coef`
  operate on the shared `Rbg` struct), so the rotation maths and the cell/bitmap
  lookup are identical and already structurally validated.
- **Cannot be validated against live RBG1 data this run** — exactly as the task
  predicted. Die Hard does NOT program RBG1 in any currently-reachable state. At
  the frozen demo frame (VB1000) `BGON = 0x0119` → **R1ON (bit5) = 0**. RBG1 only
  comes on when an actual gameplay stage loads, which the port can't reach yet
  (the parallel round-load/actor-spawn work — see rbg_roundstart_status.md §4).
- **ZERO regression proven.** With RBG1 off (the only reachable state) the RBG1
  path early-outs cleanly, exactly like RBG0. A full A/B rebuild (RBG0-only
  baseline vs RBG0+RBG1) gives **byte-identical frames** across VB900/1000/1200,
  both faithful and flat-K (6/6 identical). The shared-helper refactor of the
  rotation-parameter read preserved RBG0 bit-for-bit.
- Builds clean headless (`-DSATURN_NO_SDL=ON`, g++ Release). No new warnings; the
  old `VDP2: RBG1 (rotation) is not done` note is now gone.

## 1. What RBG1 is on the Saturn (the crux the task flagged)

RBG1 is the *second* rotation background. Unlike RBG0 (which freely picks
rotation parameter set A, B, or A/B-switched via RPMD), RBG1:

- **Always uses rotation parameter set B** (table at RPTA+0x80; coefficient
  controls KTCTL/KTAOF at the parameter-B field shift of 4; map offset RBMPOF;
  plane size RBPLSZ; the 16-plane map MPABRB..MPOPRB at 0x58..0x66).
- **Occupies NBG0's slot** — it reuses NBG0's cell/pattern/colour registers:
  CHCTLA (N0CHCN/N0BMEN/N0BMSZ/N0CHSZ), PNCN0, BMPNA, CRAOFA(N0), PRINA(N0),
  CCRNA(N0), CCCTL(N0CCEN), colour offset (N0), window WCTLA-low, LNCLEN/SDCTL
  bit0. **RBG1 and NBG0 are mutually exclusive on the hardware** — when R1ON is
  set, NBG0 does not display.
- Has its own enable bit **R1ON = BGON bit5** and transparency bit
  **R1TPON = BGON bit13**.

Reference cross-check: Yabause `vidsoft.c` implements RBG1 inside `Vdp2DrawNBG0`
under `if (regs->BGON & 0x20)` — it reads CHCTLA, PNCN0, `PLSZ>>12` (RBPLSZ),
`MPOFR & 0x70` (RBMPOF), `PRINA & 0x7`, `(CRAOFA & 0x7)<<8`, sets
`rotatenum = 1` (parameter B) and uses `Vdp2ParameterBPlaneAddr`. Every
register/shift in `setup_rbg1()` was matched to that branch. The PLSZ layout
(RAPLSZ bits 9-8, RAOVR 11-10, RBPLSZ 13-12, RBOVR 15-14) was confirmed against
the Sega VDP2 manual bit-config and Yabause's RBG0 (`PLSZ>>8` for A, `>>12` for
B). NOTE: this contradicts the naive "RBPLSZ = bits 11-10"; those are RAOVR.

## 2. What was implemented (`runtime/src/video/vdp2.cpp`)

- `read_rot_param(Rbg&, param_off, coef_shift)` — a clean extraction of the
  rotation-parameter-table read that setup_rbg already had (matrix A-F, viewpoint
  P / centre C, Mx/My, kx/ky, Xst/Yst/Zst + deltas, Xp/Yp, coefficient-table
  controls). Parameterised by the A/B table offset (0 / 0x80) and the A/B
  coefficient-field shift (0 / 4). `setup_rbg` now calls it with the RPMD-selected
  set (behaviour **unchanged** — proven byte-identical); `setup_rbg1` calls it
  with set B (0x80, shift 4).
- `setup_rbg1(Rbg&)` — the RBG1 register reader described in §1. Early-outs when
  R1ON is clear (reads only BGON), exactly like `setup_rbg`.
- Compositing: new layer `L_RBG1`, considered right after RBG0 and before the NBGs
  — so on priority ties it loses to the sprite and RBG0 but wins over NBG1-3,
  which is NBG0's hardware tie position (NBG0 itself is suppressed:
  `if (rbg1.on) nbg[0].on = false`). Its window control (WCTLA-low), line-colour
  (LNCLEN bit0) and shadow (SDCTL bit0) all route through NBG0's bits, since it is
  NBG0's slot. Colour-calc / colour-offset / priority / ratio all honoured like
  RBG0.
- `note_unsupported()`: the RBG1 "not done" note removed. Header comment updated
  (RBG1 moved from "what is not" to "what is done").
- Diagnostic env `STV_RBG_FLATK=1` now also forces Kx=Ky=1 for RBG1 (it is applied
  inside the shared `read_rot_param`).

Honest limits (same family as RBG0's): screen-over modes beyond repeat, rotation-
BG gradation/line-colour, coefficient-table line-colour data, and line/vertical-
cell scroll remain unimplemented for both rotation BGs. RBG1 bitmap mode IS wired
(bm_addr/bm_pal/bm_spr/bm_scc from BMPNA + RBMPOF) and shares rbg_pixel's bitmap
path; it is per-spec but, like everything RBG1, unexercised at runtime so far.

This is a general Saturn feature, gated only on BGON bit5 — not env-gated and not
an ST-V hack. Disc games that use RBG1 now get a second rotation layer. Anything
not using RBG1 is unaffected (`setup_rbg1` early-outs).

## 3. Validation — what could and could not be checked

### Proven (this run)
- **No regression (the hard requirement).** A/B rebuild in the isolated tree:
  RBG0-only baseline (`/tmp/vdp2_rbg0_baseline.cpp`, md5 50d92f79…, the pre-change
  working tree) vs RBG0+RBG1 (my vdp2.cpp, md5 32d0b26c…). Frames at VB900/1000/
  1200, faithful and flat-K: **6/6 byte-identical** (cmp). RBG0/NBG0-3/sprite/VDP1
  output is bit-for-bit unchanged.
  - faithful VB1200 md5 `2ee08f91…` (base == mine);
    flat-K VB1200 md5 `c2dda184…` (base == mine).
- **Clean early-out confirmed by live state.** `--dump 1000` of the reachable
  demo: `BGON=0x0119` (N0ON+N3ON+R0ON, **R1ON=0**), `RPMD=0`, `PLSZ=0840`,
  `MPOFR=0000`, `CHCTLA=0010`, `KTCTL=0011`, `KTAOF=0001` — identical to the
  documented RBG0 capture. RBG1 is OFF, so `setup_rbg1` returns immediately and
  the L_RBG1 consider block is skipped. No RBG1 note fires; no new warnings.

### NOT validated (cannot be, until a round loads)
- **No live RBG1 pixels.** The game never sets R1ON in attract/demo, so the
  `setup_rbg1` → `rbg_pixel` path for RBG1 has no runtime data to check against.
  Confidence rests on: (a) register/shift parity with Yabause's RBG1 branch
  (cross-checked line-by-line), and (b) the renderer being the *same code* as the
  RBG0 path (which composites correctly where its data is valid).
- Same caveat as RBG0: the coefficient-table VRAM addressing is best-effort and
  needs a healthy RBG-using capture to confirm the KAst/dKAst units.

**Expected:** the stage floor's second rotation layer will render the moment the
round-load work lands and the game programs R1ON + parameter B — no further RBG1
code needed, only real data.

## 4. Files changed (for reconciliation)

- `tools/saturn-recomp/runtime/src/video/vdp2.cpp` — RBG1 implementation
  (`read_rot_param` extraction shared with RBG0, `setup_rbg1`, `L_RBG1`
  compositing with NBG0 suppression + LNCLEN/SDCTL/WCTL routing, `note_unsupported`
  no longer flags RBG1, header comment updated). **Only this file was edited.**

No other files touched. `port/main_stv.cpp` NOT touched (owned by another agent).
No new recomp seeds needed.

### Build / reproduce (p620, isolated tree)
```
cd ~/decomp/dh-portB/port
# configure points at the dh-portB runtime (the CMake default hardcodes the orig tree):
cmake -S recomp -B recomp-build-B -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DSATURN_NO_SDL=ON \
  -DSATURN_RUNTIME=$HOME/decomp/dh-portB/tools/saturn-recomp/runtime \
  -DSATURN_EXTRA=$HOME/decomp/dh-portB/port/stv.cmake
cmake --build recomp-build-B --target saturn_stv -j"$(nproc)"
./recomp-build-B/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 1200 --shots 900,1000,1200 --dump 1000 --out run/mine_faithful
STV_RBG_FLATK=1 ./recomp-build-B/saturn_stv ... --out run/mine_flatk
```

### Deliverable frames (`port/run/` and the Mac scratchpad)
- `mine_faithful/shot-1200.png`, `mine_flatk/shot-1200.png` — my RBG1 build.
- `base_faithful/shot-1200.png`, `base_flatk/shot-1200.png` — RBG0-only baseline.
- base == mine, byte-identical → the before/after that proves no RBG0/NBG
  regression. Nothing visibly new (RBG1 is off in every reachable frame); these
  document the no-regression result, not a visual advance.
