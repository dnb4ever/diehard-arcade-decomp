# Functional decomp report - gen6

## Cluster chosen
**[0x02239000, 0x0223f000)** - the densest remaining band of undone *genuine*
functions (census/triage cross-referenced against `project/src_functional/`).
This is the mid-ROM combat/effect **task-handler** cluster: projectile/debris
state machines, the per-player HUD counter pair, a pooled-effect spawn
allocator, and sprite-draw wrappers. Clear of the sibling avoid-list
(colour builders 0x234b58/0x235198/0x23577c and the 0x24852c..0x249e68 merged
unit). The 0x224a000 page (18 census rows) was rejected as a single merged
`unit=224a280` of tiny leaf fragments (phantom/data), not 18 real functions.

Single serial worker, no forks. Every `.c` compiles clean with
`gcc -c -fsyntax-only -Wall -Wextra` (**zero warnings**), was hand-traced vs
`m2c_fn.py --asm`, and had every computed branch/pointer resolved with
`braf_rewrite.py` / `ptr_resolve.py` (labels distrusted, resolved TARGET
addresses trusted). `find_entry.py` was run on every candidate first.

Addressing: files/func names use the SH2 address (`02` + 6-digit cart offset);
tools take the cart offset (SH2 - 0x02000000).

## Added this wave (10) - SH2 addr -> file -> name
| SH2 addr | file | name | role |
|---|---|---|---|
| 0x0223e17c | 0223e17c.c | hud_counter_build | build+render player HUD counter glyph list |
| 0x0223df30 | 0223df30.c | hud_counter_update | per-frame HUD counter animator (sibling of above) |
| 0x022397c2 | 022397c2.c | projectile_arc_handler | 2-state arc toss; tail draw_child_sprite |
| 0x0223cbb0 | 0223cbb0.c | effect_pool_spawn | pooled typed-effect spawn allocator (returns task) |
| 0x0223e3ba | 0223e3ba.c | aim_effect_spawn | spawn tracking/aim effect, installs func_0223db2a |
| 0x0223938c | 0223938c.c | projectile_toss_handler | arc projectile + sin-driven draw |
| 0x0223967a | 0223967a.c | draw_rot_sprite | draw scaled sprite rotated by angle |
| 0x02239082 | 02239082.c | spin_projectile_handler | spinning projectile + matrix-stack draw |
| 0x0223de0e | 0223de0e.c | both_players_special_trigger | fire special state on both active players |
| 0x02239868 | 02239868.c | spread_projectile_handler | 2-state spreading projectile, 3 child sprites |

## Resolved helpers referenced (externs, not decompiled)
- func_022153f6 draw_child_sprite(obj, scale, flags) [done]
- func_022154fc (work, a, b, c) [done]
- func_0220ff0a task allocate(pool, handler) [done]
- func_02213190 sin / func_022131a8 cos (fixed-point trig) [done]
- func_02213ca4 vec3 matmul / func_02213ce4 mtx_scale_cols [done]
- func_02215a2c render actor / func_022149aa, func_02214a50, func_022149e4 matrix ops
- func_022380c0 (cart 0x2380c0 -> RAM 0x0603c0c0) + func_022380fa digit/icon draw [fa done]
- func_02214a2e (avoid-list helper; referenced only)
- func_0223e104, func_0223a9d0, func_0223db2a (cluster siblings)

## Resisted / deferred (with reason)
- **0x0223eeb4** - pool-contaminated prologue; disassembler is misaligned over
  ~0x30 leading bytes (bogus `mov.l r8,@(8,r1)` / `mac.l` storing unsaved regs),
  so the true entry/first instructions are ambiguous. Body is clean (debris
  task handler ending in func_022153f6) but entry too uncertain for a faithful
  decode. Needs raw-byte boundary analysis.
- **0x0223c638** - same contamination; its leading pool constant is read as a
  bogus `.short 0x03dc` base pointer (m2c treats `0x3DC` as a struct), prologue
  misaligned. Defer pending byte-accurate extent.
- **0x02239462** - `m2c` hard failure ("Cannot find branch target func_2386fc");
  branch leaves the detected extent into a misseg. Needs extent repair.

## Remaining in cluster (not yet done, good next targets)
Clean-ish handlers still open: 0x02239194, 0x022399bc, 0x02239ce6, 0x02239fc6,
0x0223a3a2, 0x0223a712, 0x0223a856, 0x0223c7f6, 0x0223cbb0 neighbours
0x0223c95c/0x0223cbb0, 0x0223d3ae, 0x0223d558, 0x0223d726, 0x0223d98a,
0x0223db2a, 0x0223df30 neighbour 0x0223dcc4 (braf dispatch),
0x0223e520 (clean leaf, contaminated head - recoverable). Several large ones
(0x022382e8 0xaf6, 0x0223a9b8 0x8d2, 0x0223e690 0x78a, 0x0223f10e 0xaf8) remain
and are higher-effort.
