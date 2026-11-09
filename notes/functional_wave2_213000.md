# Functional decomp - Wave 2 (re-sweep), band 0x213000-0x220000

Scope: UNDONE real entries (find_entry.py corrected) in [0x213000, 0x220000) not
already present in project/src_functional/. Wave-1 added 22 here; this re-sweep found
~64 in-band undone remaining and adds 7 at full quality.

All 7 files compile with `gcc -c -Wall -Wextra -I.` at ZERO warnings and are
hand-verified against m2c_fn.py --asm. PIC resolved with ptr_resolve / braf_resolve.

## Functions added (7)

Movement / collision subsystem (the 3 big CLEAN targets + their callbacks - a
complete, self-consistent cluster):

- 0x022170ea entity_move_dispatch   - per-entity move/collide step. rec = g_060B6D9C[ent->unk34]
  (0x48 stride, same table as func_0221707c). Swaps the global mover callback g_060B889C to
  the handler matching ent->unk84 movement flags around each call into the sweep core, runs
  func_02217c10 resolve when g_060B88A4 bit31 set. 12 mova+add callback pointers resolved
  (func_02217888/022179b0/02217ae4). HIGH.
- 0x0221746c collide_resolve_sweep  - swept-AABB collision resolver (the core func_022170ea
  drives). Clamps speed, forms candidate position, probes the g_060B889C mover (a grid search)
  along X then Z walking the nearer free edge, resolves on the least-penetration axis and
  records the contact direction in g_060B88A4 (X=bit0/1, Z=bit2/3, bit31=hard stop). Clean
  m2c decode; entry clamp + screen-bounds + field offsets cross-checked vs --asm; sweep loops
  follow m2c verbatim. MED-HIGH (geometry semantics inferred, control flow faithful).
- 0x022182c6 shutter_update         - 7-state opening/closing panel driven by trigger-flag
  word g_06098580 and per-face param table 0x06054748. Each frame draws 2 (or 3, when
  w->unk5C) matrix-pushed/translated/rotated VDP1 layers via mtx_load_abc / mtx_apply_vec_at0
  / mtx_rotate_zyx / func_02215a2c. All 4 call pointers resolved; helper ABIs confirmed from
  their done .c files (mtx_load_abc takes no args - m2c's phantom args are dead). HIGH control
  flow / MED semantics (sprite-id + param tables indexed, element meaning inferred). Note:
  the layer submitter func_02215a2c is still undecompiled (declared extern).

- 0x02217888 grid_overlap_find_any      - spatial-hash neighbour search over the 0x20x0x10
  grid @g_060B659C (row 0x80, list link +0x44). 5x5 cell window; subject swept AABB (asym Y)
  vs node box [+0x28..+0x34] via aabb_overlap_3 (0x213354). Excludes g_060B88A0 (parent).
- 0x022179b0 grid_overlap_find_skiptypes - identical, also skips node types 1..7.
- 0x02217ae4 grid_overlap_find_type0     - identical, only type-0 nodes.
  (These three are exactly the callbacks g_060B889C is swapped between by entity_move_dispatch;
  all three bsrf resolve to aabb_overlap_3.) HIGH.

Animation:
- 0x0221ffb0 waypoint_mover_update  - 3-state object walking a sprite along a path of s16[4]
  waypoints {x,y,z,frames} (stride 8). Linear per-frame interpolation via sh2_hw_divide
  (0x213234); 16.16 positions with +0x200000 X/Z bias; copies work position into sprite+0xB8.
  Two bsrf -> func_022153f6 (draw_child_sprite). HIGH.

## Resisted (with reasons)

- 0x0221fd7a - m2c fails outright ("Cannot find branch target .L21fde2"); the braf targets
  (0x2149aa, 0x214a40) need braf_rewrite before m2c can produce a body. Defer.
- 0x0221fe00 - loop spawning objects from list @0x06090968 with 5 PIC calls (2149aa, 214a40
  + 3 more matrix/draw helpers m2c rendered as inline indirect calls). Tractable but needs all
  five braf targets pinned; deferred for a focused matrix-draw-helper pass.
- 0x0221c8a0, 0x0221e9c6 - proximity/spawn helpers calling 0x20feec (cross-band spawn) with
  pool-as-struct m2c artifacts and deref of mova+add code-addresses (0x21c8f2 / 0x227906) that
  need per-pool resolution; medium risk, deferred.
- Heavy entity cluster (0x0221898c, 0x02219c44, 0x02219f26, 0x0221a1a0, 0x0221a452,
  0x0221a80c, 0x0221add4, 0x0221af30, 0x0221b14a, 0x0221b450, 0x0221b5a0, 0x0221bf72,
  0x0221c2aa..0x0221f9c4, 0x0221f532 etc.) - many bsrf into the shared spawn/animation helpers
  (0x22a5d0/0x22a708/0x22a878/0x22dc2c animation_update) and cross-band 0x24xxxx, plus the
  wave-1-flagged 0x21596x/0x215a2c/0x215cb4 sprite-submission cluster. These depend on undone
  callee ABIs; left for after those helpers are fixed (as wave-1 noted).

## Band remaining (not attempted this wave)

~57 in-band undone members remain, dominated by the heavy entity cluster above. The movement/
collision core (170ea/1746c + the 3 grid callbacks) is now complete, which unblocks reasoning
about the entity handlers that call entity_move_dispatch.

Deliverables written: 7 .c files in project/src_functional/ (022170ea, 0221746c, 022182c6,
02217888, 022179b0, 02217ae4, 0221ffb0), this file, and notes/symbols_wave2_213000.txt.
No shared files modified.
