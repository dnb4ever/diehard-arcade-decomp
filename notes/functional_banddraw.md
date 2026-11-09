# Functional decomp report — band "banddraw" (deferred large matrix/draw routines)

Scope assigned: the deferred 16x-unrolled matrix/draw units flagged by a prior
agent at SH2 0x0222d0e8, 0x0222d40e, 0x0222da1e, 0x0222dbf6, 0x0222e396.
All work on p620, `~/decomp/diehard-stv`, tools under `tools/decomp/`.

## Key correction: 3 of the 5 flagged addresses are pool-contaminated phantoms

`find_entry.py` (run with CART offsets, not SH2 addresses) collapses the five
flagged addresses to just **two** undone real functions plus two already-done:

| flagged (SH2) | cart | find_entry real entry | status |
|---|---|---|---|
| 0x0222d0e8 | 22d0e8 | 0x0222d0e8 (real)        | **decompiled here** |
| 0x0222d40e | 22d40e | 0x0222db44 (pool phantom)| already done (0222db44.c, other worker) |
| 0x0222da1e | 22da1e | 0x0222db44 (pool phantom)| already done |
| 0x0222dbf6 | 22dbf6 | 0x0222dc2c (pool phantom)| **decompiled here** |
| 0x0222e396 | 22e396 | 0x0222ea0e (pool phantom)| already done (0222ea0e.c) |

d40e/da1e/dbf6/e396 all land inside the *trailing literal pools* of the two big
functions (d0e8 has a ~0x600-byte pool; dc2c a ~0x660-byte region). They are not
functions. So the real band work was d0e8 and dc2c.

## Files added (project/src_functional/)

- **0222d0e8.c** — `draw_actor_hier_model(void *entity)`.
  16x-unrolled articulated-model draw. Fully straight-line (no loop); walks a
  segment hierarchy with an irregular matrix push/pop pattern. Sets up three
  per-actor source arrays and, for ~15 segments, rotates/translates the active
  matrix and submits each segment for drawing. All 33 `bsrf` plus the 2
  `mova+add; jsr` PIC calls resolved into the known draw cluster
  (mtx_load_abc / mtx_apply_vec / mtx_rotate_zyx / mtx_stack_push / submit_model).
  Hand-verified against `m2c_fn.py --asm`; `braf_resolve.py` confirms every
  computed branch; the two `jsr` targets computed by hand
  (0x0222d144+0xFFFE78EA=0x02214a2e, 0x0222d184+0xFFFE89B0=0x02215b34).

- **0222dc2c.c** — `s32 actor_anim_frame_update(s32 arg0, void *entity)`.
  Per-frame animation driver (reached from dbf6's corrected entry). Advances the
  animation frame, (re)primes the three 108-byte working pose buffers via block
  copies, computes the interpolation step `unk40 = 1<<(SHBASE+field)`,
  interpolates the 3-component root position toward the next pose (shifted by
  `ashr_var`), folds it into the actor world position through a 5-way motion
  switch, calls the post-update hook, and fires the one-shot spawn/child/combo
  events on the active frame. All 34 `bsrf` resolved; the lone PC-relative `bsr`
  targets 0x0222e3b0 (declared extern). m2c gives a strong control-flow skeleton
  which I corrected in two places: `unk40 = 1` (a delay-slot `mov #1,r0` that is
  actually the value arg to `ashl_var`) and `return var_r0_5` (a dead switch
  register that is overwritten by `func_022170ea`'s return — the real return).

Both compile with `gcc -c -fsyntax-only -Wall -Wextra` with **zero warnings**.

## Deferred (analyzed, not written): 0x0222e3b0

`pose_matrix_blend_decompose` — a 1630-byte helper `bsr`'d by
`actor_anim_frame_update` (and by interp_shorts_48's `SHBASE!=0` path). It is a
16-iteration loop, 4-way unrolled: per joint it builds two rotation matrices
(prev-pose and next-pose Euler sets), blends them `M = A*w + B*(1-w)` over 6
matrix elements, then re-decomposes the blended matrix back into angles via
`mtx_rotate_y(-a)/mtx_rotate_z(-a)`, `vec3_matmul_out`, `angle_from_vec2`,
`lookat_angles_xy`, writing angle shorts into the current pose buffer.

All 41 computed branches are resolved (see symbols_banddraw.txt). **Reason for
deferral:** the function uses dynamic `@-r15` argument pushes (e.g. 0x22e508/50e/
514/51a) that shift the stack frame *mid-function*, cleaned up in bulk later
(`add #64,r15` at 0x22e9a2, `add #104,r15` at 0x22e9f8). Fixed-slot modelling is
therefore unsafe — m2c itself emits undefined locals (sp24/sp28/sp38 referenced
before assignment) for exactly this reason. A faithful version needs a careful
frame-offset-tracking pass and, ideally, a runtime equivalence check (not
available in this functional-decomp flow) before shipping. Shipping a ~300-line
guess here would risk a silent equivalence bug, so it is left documented instead.
dc2c references it only through a correct `extern`, so nothing is left dangling.

## Toolchain note for the next worker

`fn.py` / `find_entry.py` / `braf_resolve.py` / `m2c_fn.py` all take **CART
offsets** (SH2 − 0x02000000) as their numeric argument, not SH2 addresses.
Passing the SH2 address makes find_entry silently treat it as a cart offset and
add 0x02000000 again (you get a bogus 0x042xxxxx "entry").
