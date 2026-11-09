# Die Hard Arcade (ST-V) RECOMP — the attract-demo PLAYFIELD garble: it was RBG0 screen-over

Date: 2026-10-20. Port side only (tools/saturn-recomp/ + port/). project/ + tools/decomp/ read-only.
Work in isolated copy ~/decomp/dh-portA/. Builds on: coin_arm_and_reel.md (the full 2..10 attract reel),
rbg_roundstart_status.md / rbg1_status work (RBG0/RBG1 implemented).
Address map: runtime(HWRAM) = cart SH-2 + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.

## TL;DR — the garble was ONE renderer gap, now fixed: the VDP2 rotation-BG **screen-over process**.
The attract demo/fade scenes program RBG0 (and in some scenes RBG1) with the screen-over code
**RAOVR/RBOVR = 2 ("transparent outside the map")** in PLSZ bits 10-11 / 14-15. The runtime VDP2 only
ever did **repeat/wrap**, so wherever the rotated coordinates leave the (small, or not-yet-loaded)
rotation map, it **wrapped and sampled unrelated VRAM as pixels → the scanline/stripe noise band** that
covered the playfield. Implementing over-modes 2 (transparent) and 3 (clamp-512) in `rbg_pixel`
removes the noise. Scenes 7/8/9 (VB6300/7100/8050) now render **cleanly**; scenes that don't use those
modes are **byte-identical** (no regression).

HONEST, up front: this fixes the *rendering* garble (the backgrounds/HUD/stage layers are now clean and
watchable). It does **not** make the demo *fighters* appear — that is a separate, upstream (functional)
gap, proven below: the game emits the character sprites every frame but with **degenerate geometry**
(all vertices collapsed), so there is nothing for any renderer to draw. See §4.

## 1. How the garble was characterised (draw-time ground truth, not guesses)

The harness `--dump` runs at VBlank-IN, **after** the game has begun rebuilding the VDP1 command table
for the next frame, so the *live* table is misleading. I added a temporary draw-time snapshot (the exact
commands executed + the framebuffer produced at VBlank-OUT) and an `STV_LAYERMASK` layer-isolation
switch to the compositor, dumped the matching no-coin reel frames (same run that made
`run/reelframes/shot-{6300,7100,8050}.png`), and rendered each VDP2 layer in isolation.

Scene 8 (VB7100), layer by layer, in the **real** compositor (`STV_LAYERMASK`):
- **NBG0** → the full night-city **skyline**, renders perfectly.
- **NBG3** → the **HUD** (portraits, green/magenta health bars, the "ANTI-TANK RIFLE" item box + count),
  renders perfectly.
- **sprite layer (VDP1)** → black (see §4).
- **RBG0** → **the noise band.** This is the whole garble.

So the compositor itself is sound: every *populated* layer (skyline, HUD, title logo in scene 9, Sega /
Fox copyright text) renders correctly. The only garbled layer is the rotation BG.

## 2. Root cause — RAOVR/RBOVR = 2, which the runtime didn't implement

PLSZ (VDP2 0x3A): bits 8-9 RAPLSZ, **bits 10-11 RAOVR**, bits 12-13 RBPLSZ, **bits 14-15 RBOVR**.
The OVR 2-bit code is the rotation-screen "screen-over process":
- 0 = repeat (wrap) outside the map
- 1 = show the over-pattern (OVPNR) outside
- 2 = **transparent** outside the map
- 3 = the screen is a single **512x512** area; outside is transparent

Measured in the garbled frames:
- VB6300 (scene 7): PLSZ=0x8844 → RAOVR=2, RBOVR=2
- VB7100 (scene 8): PLSZ=0x8844 → RAOVR=2, RBOVR=2
- VB8050 (scene 9): PLSZ=0x8844 → RAOVR=2, RBOVR=2
- VB8500 (scene 10): PLSZ=0x0040 → RAOVR=0 (repeat) → never garbled, unaffected

The runtime's `rbg_pixel` always did `ix = floor(X) % map_w` (repeat). With the map small / the demo
floor tilemap not loaded, `floor(X)` runs far outside the map every line; wrapping sampled whatever
happened to be in VRAM (leftover graphics, pattern data) and drew it as sparse dots / vertical stripes —
exactly the band. On real hardware RAOVR=2 means those dots are **transparent**.

## 3. The fix (tools/saturn-recomp/runtime/src/video/vdp2.cpp ONLY)

- `struct Rbg`: added `int over;` (the OVR code).
- `setup_rbg`:  `s.over = reg(0x3A) >> 10 & 3;`   // RAOVR
- `setup_rbg1`: `s.over = reg(0x3A) >> 14 & 3;`   // RBOVR
- `rbg_pixel`, both the cell and the bitmap path, before the wrap:
    - `over == 2` → if `floor(X)/floor(Y)` is outside `[0,map_w) x [0,map_h)` (bitmap: the bitmap size),
      `return false` (transparent).
    - `over == 3` → if outside `[0,512) x [0,512)`, `return false`.
    - `over == 0` (repeat) and `over == 1` (over-pattern, not wired) → **unchanged** (wrap as before).
- Updated the file-header "what is/ isn't done" comment.

Guarding against regressions: the new branches run **only** when the game sets OVR 2 or 3. OVR 0 (the
mode every non-rotating/ordinary use and every disc game relies on) is the identical wrap path as
before, so the NBG/HUD/skyline/sprite paths, the RBG repeat path, and the Saturn disc path are
untouched. Verified byte-identical below.

## 4. HONEST remaining gap — the fighters (functional, NOT a renderer problem)

The demo *characters* still do not appear. This is **not** fixable in the renderer, because the game
never gives them geometry. Ground truth:
- Draw-time VDP1 command dump (the commands actually executed at VBlank-OUT): the fight/demo frames
  emit ~160-254 **distorted-sprite** commands (comm=2, 8bpp textures, correct srca/size — real
  character tiles) but **all four vertices are (0,0) / (-1,-1)** — every quad collapsed to a point.
- A write-watch on the VDP1 command vertex fields caught the game itself writing **0xFFFF (= -1)** into
  those sprite vertex slots. So the degenerate geometry is the **game's** output, not a dropped/dropped
  write in the port (the background poly in the same list *does* get real vertices, and lands fine).
- Across the full 16000-VBlank run, the **peak** number of textured sprites with a real (non-collapsed)
  quad in any single frame is **9**, versus ~164 degenerate every frame.

So the demo actor/transform pipeline never produces screen positions for the characters (and some
transition scenes also never load their stage tilemap — e.g. VB8050's near map is empty). That is the
same "demo sequencer spawns no positioned actors / never completes the stage load" conclusion as
rbg_roundstart_status.md §4, i.e. a functional-decomp gap. The renderer faithfully draws everything
that *is* populated.

## 5. Results (honest; nothing faked)

Re-ran the no-coin reel (same `--stvinput "9000:SERVICE"` that produced the original reelframes),
clean build (no diagnostics compiled in), md5 vs the original garbled reelframes:

| VB   | scene | before            | after the fix                                   |
|------|-------|-------------------|-------------------------------------------------|
| 2700 | 4     | ok                | **IDENTICAL** (no OVR 2/3) — no regression       |
| 4000 | 5     | ok                | **IDENTICAL** — no regression                    |
| 4500 | 6     | ok                | **IDENTICAL** — no regression                    |
| 6300 | 7     | green stripes+noise over a dusk mountain scene | **CHANGED → clean dusk landscape** (purple sunset sky, hills, city lights) |
| 7100 | 8     | noise band over the skyline | **CHANGED → clean**: full night skyline + HUD + item box |
| 8050 | 9     | HUD + scanline noise        | **CHANGED → clean**: HUD over a black playfield (no garble; playfield empty = the §4 functional gap) |
| 8500 | 10    | ok ("INSERT COIN(S)") | **IDENTICAL** (RAOVR=0) — no regression          |

RBG0-only, scene 8: **noise → fully black** after the fix (confirms the band was off-map garbage that
should be transparent).

Verdict: the attract reel's **backgrounds and HUD are now rendered cleanly through the whole loop**
(watchable attract footage). The fighters themselves need the upstream actor-positioning work (§4),
which is out of this renderer task's scope.

## 6. Files changed this session — EXACT list
MODIFIED (source, kept):
 - `tools/saturn-recomp/runtime/src/video/vdp2.cpp` — RBG screen-over process (RAOVR/RBOVR): over-modes
   2 (transparent) and 3 (clamp-512) in `rbg_pixel` (cell + bitmap), `over` read in `setup_rbg` /
   `setup_rbg1`, `int over` struct field, header comment. The ONLY behavioural change.
REGENERATED (build artifact, no manual edit): `port/recomp-build/saturn_stv`.
REVERTED to pristine after diagnosis (so they are byte-identical to the main tree again — NO net change):
 - `tools/saturn-recomp/runtime/src/video/vdp1.cpp`  (held a temporary draw-time snapshot + STV_SPRSTAT)
 - `tools/saturn-recomp/runtime/src/video/video.cpp`  (held a temporary STV_VTXWATCH)
NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp, seeds, mmio/machine, etc.

### Frames
- port/run/demo_render_fix/{before,after}_vb{6300,7100,8050}.png
- Mac scratchpad: demo_garbled_before_vb{6300,7100,8050}.png, demo_fixed_vb{6300,7100,8050}.png

## 7. Reproduce (p620, in the copy)
```
cd ~/decomp/dh-portA/port
cmake --build recomp-build --target saturn_stv -j"$(nproc)"
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 8600 --out run/x --stvinput "9000:SERVICE" --shots 6300,7100,8050,8500
# 6300/7100/8050 now render clean; 2700/4000/4500/8500 are byte-identical to the old reelframes.
```
