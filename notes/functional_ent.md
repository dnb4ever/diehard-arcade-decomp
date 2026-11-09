# Functional decomp - entity-behaviour state-machine family (braf -> 0x22eee4)

Session goal: unlock the big actor state-machine family the 0x210-band (gen-3) agent
deferred in `notes/functional_gen3_210.md` (Resisted / deferred). They all share the
tail `braf -> 0x22eee4` (except 0x2114b0) and make many out-of-band helper calls.

All new TUs live in `project/src_functional/`, include `dh.h` (M8/M16/M32/MP accessors;
chosen over `dh_band21.h` because these actors chase struct fields and the shared tail's
neighbours already use dh.h), and compile clean with
`gcc -c -fsyntax-only -Wall -Wextra` (zero warnings). Every function was hand-verified
against `braf_rewrite.py --asm`; all `bsrf`/`braf` targets resolved with braf_rewrite/
ptr_resolve (indirect jsr/mova+add via ptr_resolve, trusting resolved addresses).

## 1. The shared tail dispatcher - func_0222eee4 (cart 0x22eee4) ABI

`void func_0222eee4(void *entity)` - **spatial_grid_insert**. The common tail every
state machine braf's into after drawing. Registers the entity into a per-frame 32x16
spatial-hash grid so later passes can query cell occupancy.

- origin struct @ 0x0609742c: +0 = world-X origin (16.16), +8 = world-Y origin (16.16)
- cell: `col = ((entity->unk08 - originX) >> 16) & 0x1F` (0..31),
        `row = ((entity->unk10 - originY) >> 16) & 0x0F` (0..15)
- node pool @ 0x06097450: up to 32 8-byte nodes { next, entity }
- bucket head array @ 0x06097558: 16 rows x 32 cols of node pointers (row stride 0x80)
- count @ 0x06097550: 0..32; when >= 32 the entity is silently dropped
- effect: push node[count] to the front of grid[row][col]; count++. No return value.

This is NOT a handler/task dispatcher (the name dispatcher in the brief is a misnomer);
it is a frame collision/render bucket registration. Entity args read: unk08 (world X),
unk10 (world Y).

## 2. The family (10 added)

| file | name | states | tail | notes |
|------|------|--------|------|-------|
| 02210690.c | orientation_angle_builder | - | - | leaf; matrix-stack transform, indirect jsr=mtx_apply_vec(0x213538); writes 3 look-at angles |
| 0221185c.c | actor_update_spin_popup | 0,1 | grid | spin + scale-up/hold/down by life unk58; table g_0605333c |
| 02212b20.c | actor_update_sway | 0,1 | none | sinusoidal sway AI; no own draw (animation_update does it) |
| 02212c80.c | actor_update_breakable | 0,1,2 | grid | arm->break(spawn debris cbb0(6,3,0))->despawn |
| 02212e00.c | actor_update_risingfx | 0-4 | grid | rising spin fx; cbb0(5,3,0); blink despawn |
| 0221214a.c | actor_update_orbiter | 0,1,2 | grid | 8-phase facing nudge; trigger spawn cbb0(11,3,0) |
| 02212350.c | actor_update_scripted_spawner | 0,1,2 | grid | script-list interpreter g_060906e8[task->unkF]; feec/fece/ff28/ff0a + cbb0 spawns |
| 02211108.c | actor_update_thrown | 0-3 | grid* | re-hittable tumbler; jsr r10=sin_lookup; *no tail while dying (blink) |
| 022114b0.c | actor_update_debris_emitter | 0-4 | none | timeline popups + burst; spawns func_0221185c / func_022119a4 / func_0223194a |
| 02211b04.c | actor_update_grenade | 0-6 | grid* | largest; arcing projectile; jsr r10=move, r8=func_0223a924; *no tail while exploding |

Common shape: `task->unk8 = entity`, `task->unk10 = state`; init allocs a collision slot
(func_02228106, <0 => deactivate via `task->unk0 = 0`), registers it (func_0221707c),
runs per-state logic, then draws (mtx_load_abc 0x2149aa / apply-vec / rotate / submit
0x215a2c) with the matrix stack *(0x06098878) +/- 0x30, and tail-calls func_0222eee4.
Dying/exploding states blink (draw on odd timer) and skip the grid insert, and release
the shared occupancy byte g_060996d4[unk34-8] when unk34 > 8.

### Resolved indirect/computed calls
- 0x210690: `jsr @r11` -> 0x02213538 mtx_apply_vec (ptr_resolve mova+add)
- 0x211108: `jsr @r10` -> 0x02213190 sin_lookup; ff28 handler -> func_0223194a
- 0x212350: mova+add child handler -> func_0221279c
- 0x2114b0: feec/ff28 handlers -> func_0221185c / func_0223194a / func_022119a4
- 0x211b04: `jsr @r10` -> 0x022170ea move; `jsr @r8` -> 0x0223a924; ff28 -> func_0223194a

### Helper ABIs pinned (see notes/symbols_ent.txt)
func_0223e4a0 nullary (NOT (task) - r4 ignored); func_0223a924 is 4-arg (idx,mode,src,save);
func_0222db44 is 1-arg (r5 dead); func_02210858(obj, val-as-ptr); func_0221080a(vec,angles);
func_022109d4 returns s16 (end-of-sequence flag).

## 3. Nearby functions
- 0x210622 (look-at angles sibling) was already done (02210622.c) - referenced by 0x210690.
- No other genuinely-undone functions were found in the immediate 0x2211xxx-0x2212xxx
  cluster beyond the family itself; the surrounding entries are the already-done matrix/
  anim helpers (0x213xxx, 0x214xxx) the family calls.

## Resisted / deferred
None from the assigned family - all 10 + the tail were decompiled to the faithful,
zero-warning, hand-checked bar. One modelling note carried in-file: 0x212350's spawn-switch
`default` reuses the previous child pointer exactly as the SH-2 reuses r10 (obj initialised
to 0 so the C is defined); valid scripts only use e->unk4 in 0..3.
