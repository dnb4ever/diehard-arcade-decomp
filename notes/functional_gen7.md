# Functional decomp - gen7

## Cluster chosen
The **projectile/spark firing + swept hit-detection subsystem** in the 0x22d-0x230
band. Census (`census.py`, after masking addresses to the lower-24-bit convention
the tool prints) showed the densest remaining undone-genuine clusters at
0x22f000 (12), 0x231000 (10), 0x20f000 (10). I verified the 0x24a000 "pocket"
cluster (18 rows) is almost entirely **phantom** - unit 0x24a280 is a jump-table
whose tiny "leafC" rows are table data, not functions - so I avoided it.

Within 0x22d-0x230 I picked the contiguous, fully-undone run 0x22db44..0x22ff40.
`find_entry.py` was run on every candidate and corrected heavy pool
contamination: census rows 0x22f628, 0x22f8b6 are both aliases of the single real
function 0x22f640; 0x22fb4a->0x22fb90, 0x22fa08->0x22fa20, 0x22ff28->0x22ff40,
0x22eece->0x22eee4. These real functions form one coherent module: the actor-fired
spark/arc/slide projectiles, their emit/trail logic, and the weapon hit test.

Stayed off the avoid-list (0x235198, 0x2492f8, 0x248bb8, port/).

## Added (11 functions, all compile -Wall -Wextra clean, hand-verified vs asm)
- **0x0222db44 actor_load_pose_buffers** - pose-state init from a variant record;
  uses ashl_var (0x24a360) and the r0/r1/r2 block-copy helper (0x24a280).
- **0x0222f1d0 hit_check_swept** - swept point-to-segment weapon/target overlap
  test (vec3_dot_fixed / fixmul / hw_divide), hit registration + knockback angles.
  m2c had failed on this (branch-into-branch); fully hand-decoded.
- **0x0222f4e8 actor_anim_step_task** - per-frame path/anim actor task with partner
  trail spawn and 4-way motion dispatch.
- **0x0222f640 projectile_track_init_task** -> installs 0x0222f6a8.
- **0x0222f6a8 projectile_phase_track_task** - phase-driven sibling of f4e8.
- **0x0222fa20 slide_projectile_fire_task** -> installs 0x0222fb90 (table velocity).
- **0x0222fb90 spark_projectile_slide_task** - horizontal slide, grid-collision kill.
- **0x0222fc02 spark_projectile_fire_task** -> installs 0x0222fd54 (fixed velocity).
- **0x0222fd54 projectile_spark_arc_task** - arc motion, height kill.
- **0x0222fe14 actor_emit_sparks** - spawns up to 3 randomised trail objects.
- **0x0222ff40 projectile_seed_table_task** - sibling of func_02230006, table velocity.

The fire-task -> motion-task links were cross-checked (0x22fc02->0x22fd54,
0x22fa20->0x22fb90, 0x22ff40->0x022300e8); 0x22ff40's call convention was
confirmed identical to the already-done sibling func_02230006
(`func_022301c4(task, ent, rec, (ent->unk74+1)&1)`).

## Resisted / deferred (with reason)
- **0x0222eee4 spatial_grid_insert** - I independently decoded it (as
  collision_grid_insert) and only then found it **already present** in
  src_functional under its real-entry filename. My decode matched the existing
  one field-for-field (good cross-validation); not re-deployed.
- **0x0222ef38** (sz 0x298) - m2c fails ("Cannot find branch target .L22f1b2"),
  branch-into-branch control flow; all 5 helpers are resolved (0x21330c, divide
  0x213234, 0x2107dc, 0x21055c, sound 0x215f6c) but the merged control flow needs
  a careful full manual trace that is worth its own focused pass. Deferred.
- **unit 0x22d0e8** (0x22d0e8 / 0x22d40e / 0x22da1e / 0x22db44[done]) and
  **0x22dbf6 / 0x22e396** - large (0x326-0x7a0) 16x-unrolled matrix/draw routines.
  All their bsrf targets resolve cleanly to the known matrix-stack draw cluster
  (mtx_load_abc / mtx_apply_vec / mtx_rotate_zyx / vec3_array_matmul), so they are
  tractable, but each is a long, repetitive hand-verify. Deferred as a batch for a
  draw-focused pass. (0x22db44 itself was done - it is the init helper, not an
  unrolled drawer.)

## Remaining in band after gen7
0x22d0e8, 0x22d40e, 0x22da1e, 0x22dbf6, 0x22e396 (draw/unrolled),
0x22ef38 (branch-into-branch). The 0x231xxx and 0x20fxxx clusters remain
untouched and are the next densest targets.

## Method notes
- Addresses in census/scan_family output are the lower 24 bits of the SH2 cart
  address (0x2200400 printed as 0x200400); the honest undone-genuine count is
  derived by masking both triage rows and src_functional filenames to 0xFFFFFF and
  matching on addr OR unit_start.
- `find_entry.py` FIRST on every row caught 4 pool-contaminated entries and 1
  duplicate pair in this band alone.
- Non-standard-ABI helpers (ashl_var r0/r1, block-copy r0/r1/r2) declared with
  logical argument lists and an ABI comment, matching the existing house style for
  func_0224a360.
