# Functional decomp report - band 0x231xxx (SH2 0x02231000-0x02232000)

Worker: band231 (serial). Target cluster: 0x231xxx. All work on p620 under
`~/decomp/diehard-stv`. Each function hand-verified against `m2c_fn.py --asm`,
every computed branch confirmed with `braf_resolve.py`, phantom rows rejected
with `find_entry.py`, and each `.c` compiles with **zero** warnings under
`gcc -c -fsyntax-only -Wall -Wextra`.

## Census of the band

`census.py` lists 0x02231xxx members as entirely genuine code (callC/leafC).
`find_entry.py` corrects the fn.units segmentation; three census "functions"
are pool-contaminated phantoms and were skipped:

- **0x02231382** - inside the 0x231336 unit's trailing literal pool (find_entry
  `real=False`); real code resumes at 0x2314 4a (already done).
- **0x022314f6** - fn.units' reported first member of its unit; find_entry
  replaces it with the previous unit's extent-end (pool-contaminated).
- **0x022315fe** - `real=False`, first word 0xffff (pure pool).

Already-done before this pass (files present): 115c, 1336, 144a, 17c0, 1864,
194a, 19b2, 1a1c, 1b0e, 1d72.

## Functions added this pass (7)

| addr | bytes | name | notes |
|------|------:|------|-------|
| 0x0223101c | 0x140 | effect_prescatter_task | 2-state; spawns effect_scatter_task (0x230f74) |
| 0x0223152c | 0xd2 | projectile_launch_dual_task | ammo-reserve + 2 trail children; next=0x231630 |
| 0x02231a80 | 0x8e | entity_orient_from_angles | 8-arg stack-marshalled call reconstructed by hand |
| 0x02231b14 | 0x11c | launched_actor_task | matrix push/pop, 2 draw helpers, calls 0x231a80 |
| 0x02231c30 | 0x142 | projectile_emitter_task | ashl_var-gated cadence, child handler 0x231d72 |
| 0x02231db8 | 0x21c | rising_split_projectile_task | 3 exit paths incl. 2 braf tails |
| 0x02231fd4 | 0xb6 | effect_spawner_task | continuation of 0x231db8; 3 spawn variants |

These seven are a tightly-related family of per-frame projectile / effect task
handlers: `task->unk8` = the owned entity, `task->unk14->unk8` = the owner,
`task->unk10` = a small state byte, position at ent.unk8/C/10, velocity at
ent.unk14/18/1C, counter at ent.unk58/5A, anim id at ent.unk30. They share the
`g_06097D58[(oent.unk34<<8)+(u8)ent.unk5F*0x10]` anim/pos record indexing and
the `(&g_0604EBE4)[(s8)ent.unk34]` palette lookup seen in the already-done
siblings (0x23144a, 0x23194a, 0x2319b2).

## Key findings / corrections

- **func_0224a360 is `ashl_var`, not a generic dispatcher.** The older
  symbols.txt entry calls 0x0224a360 "dispatch_table_a / value generator"; the
  HIGH-confidence (exhaustively simulated) entry resolves it as
  `ashl_var(value r0, count r1) -> value << count` (count<0 unchanged, >=32 -> 0).
  In this band the shift count is always the runtime global `g_06096C90`
  (scale/level driver). Modeled with the standard tree convention
  `extern s32 func_0224a360(s32 value, s32 count);`. Used in 0x231c30 (twice:
  lifetime `12<<L`, cadence mask `4<<L`) and 0x231fd4 (child lifetime `26<<L`).
  m2c cannot lower it and silently treats it as an identity, which is wrong;
  callers must route through func_0224a360.

- **0x02231a80 is a shared helper, not a task.** It takes (entity, params) and
  both the 0x231b14 and 0x231c30 task handlers call it to set the aim angles.
  m2c dropped the entire 8-argument stack-marshalling for the look-at call
  (func_0221055c); it was hand-reconstructed from the push sequence - the two
  trailing stack args are &local_angle slots that receive elev/head outputs,
  which land in ent.unk24 and ent.unk26+0x4000.

- **0x02231db8 defeated m2c** ("Cannot find branch target .L231f4e"): the
  function's literal pool sits mid-body after the first braf tail (0x2153f6),
  and capstone mis-decodes it into bogus instructions + a phantom branch. The
  real control flow has three exits (kill, split-tail-0x2154fc,
  normal-tail-0x2153f6); decompiled from the raw asm, ignoring the phantom pool
  rows per the toolchain's documented behaviour.

- **Task continuation chain confirmed:** 0x231db8's normal path sets
  `task->unk0 = 0x02231FD4` and resets state to 0, handing the task to the next
  function in the band (0x231fd4) - which is why 0x231fd4 reads as a direct
  continuation (same entity layout, spawns the visual sub-objects).

## Deferred

None. All seven genuine undone functions in the band were decompiled cleanly.
The three phantom rows (0x231382 / 0x2314f6 / 0x2315fe) are data/pool, not code.
The spawn-template/handler addresses referenced out of band (0x230f74, 0x231630,
0x239194, 0x23938c, 0x2320d4, 0x238e94, 0x238dde) are left to their owning
bands; they are referenced here only as constant pointers.
