# Functional decomp report - band 0x232000-0x237000 (SH2)

Worker: band23mid. Serial, single worker. No git, no shared headers/tools/port touched.
All files in `project/src_functional/`, named by full 8-hex SH2 address.
Every file compiles clean: `gcc -c -fsyntax-only -Wall -Wextra` (zero warnings).
Every computed branch confirmed with `braf_resolve.py`; entries validated with
`find_entry.py` before work.

## Files added (9)

| file | role |
|------|------|
| 022320d4.c | particle_fall_task - ashr-driven falling particle, tail sprite_submit1 |
| 0223236c.c | homing_follow_task - target-nearest-actor follow, tail drawlist_prepend |
| 02232526.c | debris_arc_draw_task - gravity arc + matrix-stack model draw |
| 02233250.c | scene_stage_fade_spawn - per-scene stage loader + fade/particle spawner |
| 022332d4.c | particle_list_emit - 8-byte-node effect-pool emitter (tail of 02233250) |
| 02233388.c | spawn_record_build - 20-byte-record spawn-list builder |
| 0223346c.c | actor_task_setup_dispatch - actor init + per-frame handler-table dispatch |
| 02235f0c.c | actor_draw_11seg - 11-part body sprite draw (compiler unrolled x4) |
| 02236246.c | cam_projectile_task - camera-anchored 3-state projectile/spark |

## Key findings / corrections

- **2232090 is pool-contaminated.** The genuine entry is **0x022320d4** (first=2fe6);
  find_entry.py maps the triage row 2232090 forward. Decompiled at 0x022320d4.
- **23489a, 235d9a, 23577c are phantoms** (find_entry real=False, first=ffff/0001).
  Skipped - not real function entries. (235d9a appears as an example addr in the
  task brief but is NOT a real entry here.)
- **0x02233250 tail-calls a separate function at 0x022332d4** (`bra .L2332d4` after
  full epilogue). find_entry confirms 0x022332d4 is a real entry (it is not a
  distinct fn.units member, it's folded into 233250's extent). Decompiled both as
  separate files; 022332d4 already had a tentative symbol name.
- **ashr_var (0x0224a420) and sdiv32 (0x0224a06c) use an r0/r1 register ABI**, not
  r4/r5. For functional decomp this is modelled as ordinary 2-arg calls with the
  correct value semantics (value>>count, dividend/divisor). sdiv32 confirmed by
  reading its prologue (div0s + 32x div1, div-by-zero guarded on r0).
- **02233388 spawn dispatch**: the 5 jsr targets are mova+add computed (resolved by
  hand: 0x0220feec/fece/ff28/ff0a/ff90 for types 0-4) plus 1 bsrf (0x0220ff54 type 5).
  Record type > 5 is a dead path in the original (stale object pointer written);
  modelled as a no-op spawn with a comment.
- **02235f0c**: single 11-iteration (i=0..10) draw loop that the compiler unrolled by
  4 (<7 block then <0xB tail). Net iteration count is exactly 11; rolled cleanly.
  The `(attr<<9)>>4` depth with a negative-bias +15 is faithful but the bias never
  changes the result (attr<<9 is always a multiple of 16).

## Deferred (with reasons) - the band's video-bootstrap / debug-editor cluster

These remaining real entries in the band are all large multi-state video/scene
bootstrap or debug-editor handlers sharing the same hard features: a variadic UI
glyph thunk `(*0x060902A0)(...)` whose per-call-site arg count m2c cannot express,
VDP2 shadow-register blocks (`*0x06098F08`), handler-pointer installs via mova+add,
and out-of-cart DMA descriptor pointers. Doing them faithfully needs per-call-site
ABI verification beyond a clean single pass; left for a focused follow-up rather
than risk a wrong decomp.

- **0x02234984** (0x12a) - VDP2 stage/graphics loader: out-of-cart DMA descriptor
  pointers (0x22695d64/6c/74), 6-arg tilemap copy + 5-arg VDP2 packer with shared
  stack frame, tail func_022213a8. ABI of the multi-stack-arg calls needs care.
- **0x0223358c** (0x430) - video/menu bootstrap: has an **UNRESOLVED computed branch**
  (233740 `bsrf r6`) that braf_resolve cannot pin, plus the UI glyph thunk. Cannot
  confirm every branch -> excluded by the strict rule.
- **0x022363e4** (0x356, 14 branches), **0x0223673a** (0x39a), **0x02236af0** (0x376,
  debug cursor/text editor), **0x02236e66** (0x342) - same video-bootstrap/debug-UI
  family; glyph-thunk + VDP2 shadow + mova+add handler installs.
- **0x02232610** (0x5b4, 36 branches), **0x02232bc4** (0x55e, 30 branches) - very large
  dispatchers (div1/mac math). Big; left for a dedicated pass.

## Notes
- Did not touch git, notes/symbols.txt, shared headers, tools/, or port/.
- Band-specific symbols written to notes/symbols_band23mid.txt.
