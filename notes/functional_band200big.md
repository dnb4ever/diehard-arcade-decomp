# Functional decomp report - band 0x200-0x210 "big targets" (Opus, single serial worker)

Target list was the large still-undone functions flagged in notes/functional_band200.md plus
the deferred HUD/score builders. No forks; every .c compiles clean under
`gcc -O1 -c -Wall -Wextra -I project/src_functional` (zero diagnostics; unused-param /
uninitialized-read would fail). Names/globals -> notes/symbols_band200big.txt.

## Added (.c in project/src_functional/, named func_<8hexSH2>)

| file | SH-2 | fidelity | what |
|------|------|----------|------|
| func_0220d9ec.c | 0x0220d9ec | HIGH | view_bounds_from_actors (leaf): XZ bounding region over active actors + recent-position ring -> min/max corners |
| func_0220c17c.c | 0x0220c17c | HIGH | camera_track_single: single-focus per-frame camera framing + zoom-out; tail mtx_build_fwd_inv |
| func_02208b98.c | 0x02208b98 | HIGH | scene_layer_loader_spawn_c: DMA/fill/plane loader task, tail-spawns func_02208c44 |
| func_02207bcc.c | 0x02207bcc | HIGH | screen_fx_builder_task: 6-state VDP display-list + sin palette effect |
| func_0220a390.c | 0x0220a390 | HIGH | hud_stats_screen_build: percentage-bar stats HUD |
| func_0220a4f0.c | 0x0220a4f0 | HIGH | hud_tally_screen_build: 2-column tally HUD |
| func_0220a648.c | 0x0220a648 | HIGH | hud_grid_screen_build: 10x3 value-grid HUD |
| func_0220bc20.c | 0x0220bc20 | HIGH | camera_init_pose: scene-start camera pose init + ring clear + mtx build |

## Key findings / corrections (these change the triage)

- **0x208b98 is NOT ~595 insns - it is a ~22-instruction loader.** m2c fails ("Cannot find
  branch target func_208b0e") and braf_resolve reports a garbage `bsrf r0 -> 0x5711c114` at
  cart 0x208c04. Cause: the real function ends at a `braf` tail (cart 0x208be6) and is
  followed by its own literal pool (cart 0x208bdc..0x208c43). m2c decoded that pool as code,
  producing the phantom `bt func_208b0e`, the garbage bsrf, and `bra func_209656`. The "595"
  count swallowed the pool + the next function (func_02208c44, which begins at cart 0x208c44,
  bytes 2fe6 2fd6 = a prologue). This is the exact DATA trap from the brief.

- **0x20c17c is only the first of two contiguous functions.** It ends with a `braf` tail call
  to mtx_build_fwd_inv at cart 0x20c3dc; a SEPARATE real entry begins at cart 0x20c3e4
  (camera_track_pair, the dual-focus / 2-player sibling, find_entry confirms "already real").
  The "771" insn count in functional_band200.md covers BOTH. Only 0x20c17c was in scope;
  0x20c3e4 is decoded in my head but left undone (next-priority - it is the twin of
  func_0220c17c with two foci unkAE+unkB0).

- **0x207bcc's emit thunk func_0221632e is FIXED 4-arg, not variadic.** m2c renders calls
  like `temp_r10(2, cur, 0x25E3FC00, 0xF0, 4, 0x203, ...)` - the trailing args are actually
  the gradient fill-loop stores (`*g_06099510++ = v`) mis-folded into the call. Every real
  site sets only r4-r7. braf_resolve's `207d66 -> garbage` and `207ea4 -> TABLE/COMPUTED`
  are both literal-pool shorts (0x0203) between code blocks, not instructions.

- **The HUD/score builders are cleanly doable via the established g_060902A0 model.** The
  variadic ui_printf thunk *g_060902A0 is modeled (per existing func_0220ad58.c) as
  `void(*)(s32 fmt, ...)` with ALL args on the stack, fmt first = lowest pushed. Each site's
  arg count was read from the push sequence + `add #N,r15` (never from m2c's counts):
  0x20a390/0x20a4f0/0x20a648 all use `g_060902A0(fmt 0x06051434, value)` (2 stack args/site).
  The glyph thunk *g_0609029C is the 1-arg ui_glyph. The resident fp *g_06000644 RETURNS a
  u16 field value here (op=0), so it is declared `s32(*)(s32,void*,s32)` not void.

- **0x20a648 / 0x20bc20 real entries differ from the triage addresses** (find_entry):
  0x20a612 -> 0x20a648, 0x20bbfa -> 0x20bc20 (both were pool-contaminated pre-entries).
  0x20bc20 is a camera pose-init (calls mtx_build_fwd_inv), NOT a ui_printf HUD builder -
  it was mis-grouped with the variadic HUD set.

- **Re-rolled compiler unrolls** (matching prior band200 convention): the 4x ring loop in
  0x20d9ec, the 3x gradient loops in 0x207bcc, and the 2x glyph loops in 0x20a4f0/0x20a648.
  Each count is an exact multiple of the unroll factor, so the single-step loop is equivalent.

## Folded-in / already-done (no new file)

- **0x20e1e2 -> 0x20d9ec** (pool-contaminated pre-entry; same function, done here).
- **0x202126 -> 0x202138** already decompiled (project/src_functional/02202138.c).
- **0x204362 -> 0x2044cc** already decompiled (project/src_functional/022044cc.c).

## Deferred (with reason)

- **0x20c3e4 (camera_track_pair)** - genuine separate entry revealed while doing 0x20c17c;
  the dual-focus camera twin. Out of the assigned target set; left for a follow-up (it is
  fully analysable - same helpers, two foci, extra pitch/head sign handling).
- **func_02208c44, func_0220bdb4** - handlers referenced by the loaders/camera-init above;
  not in scope this pass.

## Note on naming convention
Brief said write func_<8hexSH2>.c; I followed that. The existing 838 files in
project/src_functional/ use bare <hexSH2>.c (no func_ prefix) - flagging the inconsistency,
but the explicit instruction wins. Compile/symbol lookup are filename-independent.
