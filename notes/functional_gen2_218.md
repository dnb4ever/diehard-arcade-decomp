# Functional decomp wave "gen2_218" (Opus, single serial worker)

Headline: **11 new verified functional `.c`**, every one compiling clean
`gcc -c -fsyntax-only -Wall -Wextra` with **ZERO warnings**, hand-traced
statement-for-statement against bounded SH-2 disassembly (`m2c_fn.py --asm`), with
every computed branch (`braf_resolve.py`) and PIC data pointer (`ptr_resolve.py`)
resolved, and `find_entry.py` run on every target.

This wave executes the priority-1 item the "gen" wave flagged: **unlock the
MERGED-EXTENT scripted-event sub-family** that `braf`s into the shared handler
**0x218038**. Step 1 locks that handler's ABI; step 2 re-segments the merged members
into dispatcher + handler pairs; step 3 clears the deferred 0x21e8c4 cluster.

## 1. The shared branch target 0x218038 (ABI locked)

`func_02218038` (named **scene_record_bootstrap_sm**) is the `braf` tail-call target
of a family of tiny per-scene dispatchers. **Locked ABI: `r4 = task`, `r5 = record`**,
where `record` is a **12-byte per-scene-variant descriptor** selected by the caller as
`base + g_scene->unk29 * 12`. Record fields used: `unk0` s32 (spawn param, -> arg b of
`func_0220feb0`), `unk4` s32 (optional notify arg -> `func_02233388`), `unk8` s16 and
`unkA` s16 (seed `spawned->unk8->unk50`; `unkA` also drives a position-panned sound).

It is the record-driven **sibling of `scene_bootstrap_sm` (0x22027e4)**: same handler
install (`0x20f6f6` into `g_0609882c`), same 4-state shape (switch on `task->unk10`,
`task->unk8 = entity`), same VDP2 teardown in state 3 (TVMD = shadow->unk0 & 0x7fff,
clear scene-ready globals, `sound_queue_push(0xa00003ff)`, prune via
`func_0220fdc8((s8)task->unkE + 0xff)`). State 1 **falls through** into state 2 (verified
`/* fallthrough */`, no `break` in the asm). All 12 `bsrf` and all 5 `mova+add`
pointers resolved; the `jsr @r10` target is `spawn_obj_0x6c` (0x20fece).

The ABI was cross-checked against the callers (the dispatchers): each loads
`r5 = record`, leaves `r4 = task` untouched, and `braf`s -> 0x218038. Confirmed.

## 2. Re-segmentation of the merged-extent members

Each census "member" is a tiny tail-call dispatcher that the forced-disassembly extent
walker had glued to a **separate following handler** past the unconditional `braf`.
Split point = the instruction after the `braf` delay slot. Every member yielded two
clean functions:

| census member | dispatcher (-> 0x218038) | record base | following handler | handler role |
|---|---|---|---|---|
| 0x21ce1c | `func_0221ce1c` | 0x06074b54 | `func_0221ce3e` | one-shot: `g_06098580\|=1`; pick+spawn list; terminate |
| 0x21b080 | `func_0221b080` | 0x06074550 | `func_0221b0a2` | 2-state motion/anim step; rng jitter; `func_02154fc` |
| 0x21f22e | `func_0221f240` | 0x060752b0 | `func_0221f262` | spawn; timer=spawnlist->unk4; respawn on expiry |
| 0x21eaac | `func_0221eac0` | 0x060751c0 | `func_0221eae2` | spawn; notify; flag 0xC; 56f timer; sfx A01207/08FF |

Re-segmentation notes:
- **0x21ce1c, 0x21b080** — `find_entry` reports the census start as the real entry; the
  dispatcher occupies the start, the handler begins right after the braf delay slot.
- **0x21f22e** — pool-contaminated. Its first 0x12 bytes are a *trailing literal pool of
  the previous function*; the real dispatcher starts at **0x21f240**. `find_entry` skipped
  the whole member to 0x21f2bc (the next sibling), so both the dispatcher (0x21f240) and
  the handler (0x21f262) were segmented by hand.
- **0x21eaac** — pool-contaminated. `find_entry` -> real code entry **0x21eac0** (the
  dispatcher); handler follows at 0x21eae2 and ends exactly where the already-done
  `evt_actor_spawn_notify` (0x21eb62) begins. NOTE this dispatcher uses **`extu.b`**
  (unsigned) for the record index, where the other three use `exts.b` — faithfully
  reproduced.

The handlers are all in the well-understood evt-actor idiom (state 0 prologue
`entity->unk30 = scene_value_select(); process_spawn_list(entity);`), matching the gen
wave's `evt_actor_spawn_*` family; `func_0221f262` and `func_0221eae2` are near-twins of
the already-decompiled 0x21d4fc and 0x21eb62 respectively.

## 3. Deferred 0x21e8c4 cluster (RESOLVED)

The "pool-contaminated cluster" turned out to be two clean standalone functions once
the leading garbage is recognized as a trailing pool of the previous function (the 4
bytes 0x21e8c4..0x21e8c8). `find_entry` gives two real entries:
- **`func_0221e8c8` evt_actor_fall_proximity_step** — a falling actor with a per-frame
  draw pass (same `g_06098878` +/-0x30 push/pop idiom as `evt_actor_render`, plus a
  `mat3_scale_columns(0x20000^3)` call). Despawns when either active player enters its
  proximity box, else after a 90-frame lifetime.
- **`func_0221e9c6` evt_proximity_spawn** — the helper it calls: a 2D box proximity test
  (`|dx| < 0x8000 && |dy| < 0xa000`) that, when in range, spawns a body (handler
  0x227906) via `func_0220feec`, copies position from `g_060995B0[unk34]->unk8`, bumps a
  clamped per-slot counter, and returns -1 (0 when out of range).

## Notable confirmations / faithful decodes (no stubs)
- **Record ABI** of 0x218038 locked from body + all four callers.
- `func_02154fc` (0x2154fc) takes **four** args (r4..r7 all live: entity, unk64,
  0x50000, 1) — verified from its prologue (stores r5/r6/r7, switches on `r7 & 3`).
- Position-panned sound in 0x218038 state 2: `sound_emit_gated((128 - |record->unkA|) >> 1)`
  — the neg/cmp-pz/neg/add/shar sequence decoded exactly (helper `iabs`).
- `func_0221e9c6` indexes the source table by `(s8)unk34` (stride 4) but the counter /
  `unk60` by `(s8)unk34 - 1` (stride 2) — two `exts.b` of the same byte, reproduced.

## ABI assumptions recorded in the source (flagged inline, not hidden)
- 0x218038: three inherited-r4 calls (`func_0215fac` gets the just-spawned obj;
  `func_0216d58` gets task) and `func_02233388` named from its call site (GUESS,
  2-arg). `func_0221ad58` kept its existing `(s32 obj)` signature (task cast to s32).
- `func_0221e8c8`: `func_022149aa` (mtx_load_abc) is reached with a **stale r4** — the
  asm sets up no argument; the matrix loader reads the cached matrix `g_06098abc` and
  ignores r4. Modelled as `(task)` to match the sibling `evt_actor_render`; arg value
  immaterial. This is the only non-HIGH point in the wave and is documented in-file.

## Resisted / deferred (honest)
- Nothing from the mission target list was resisted — all four merged members, their
  eight dispatcher/handler halves, the shared 0x218038, and both 0x21e8c4-cluster
  functions are delivered (11 files).
- `func_02233388` (0x2233388) and `func_02227906` (0x227906) are left as **declared
  callees** (GUESS signatures from their call sites); decompiling them is out of scope
  for this wave (they are not part of the merged-extent family) and both have m2c extent
  quirks worth a dedicated pass.

## Files added (project/src_functional/)
02218038.c  0221ce1c.c  0221b080.c  0221f240.c  0221eac0.c
0221ce3e.c  0221b0a2.c  0221f262.c  0221eae2.c  0221e9c6.c  0221e8c8.c
