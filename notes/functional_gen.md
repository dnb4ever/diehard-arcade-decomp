# Functional decomp wave "gen" (Opus, single serial worker)

Headline: **15 new verified functional `.c`**, every one compiling clean
`gcc -c -fsyntax-only -Wall -Wextra` with **ZERO warnings**, hand-traced
statement-for-statement against bounded SH-2 disassembly (`m2c_fn.py --asm`), with
every computed branch (`braf_resolve`) and PIC data pointer (`ptr_resolve`) resolved,
and **`find_entry.py` run on every target** (two pool-contaminated / misaligned
members were caught and deferred, see below).

This wave works the **scripted-event actor-step family** at cart 0x21cxxx-0x21fxxx —
the small per-state task handlers that drive cut-scene / event actors — plus the
spawn / clone / dispatch helpers they and the 0x22c-0x24x actor code lean on. All of
them were unblocked by the now-done shared helpers `scene_value_select` (0x22a878),
`process_spawn_list` (0x22a5d0) and `slot_flag_refresh` (0x22a708).

## Functions added (cart -> SH2 file -> name, role)
| cart | file | name | role |
|---|---|---|---|
| 0x21ccf8 | 0221ccf8.c | `evt_actor_spawn_wait100` | 3-state: spawn; wait 100f (entity->unk18); set g_06098580|=1; end |
| 0x21ce6e | 0221ce6e.c | `evt_actor_spawn_sfx66` | spawn+flag 0xC; sfx A01206FF; 0x42-frame timer; sfx A01207FF |
| 0x21cee8 | 0221cee8.c | `evt_actor_spawn_sfx40` | spawn; sfx A01208FF; 0x28-frame timer; sfx A01209FF; end |
| 0x21d4fc | 0221d4fc.c | `evt_actor_spawn_respawn` | spawn; timer = spawnlist->unk4; re-run process_spawn_list on expiry |
| 0x21eb62 | 0221eb62.c | `evt_actor_spawn_notify` | spawn; func_022332d4(g_060751B0); flag 4; 0x34 timer; flag 2 |
| 0x21f1d6 | 0221f1d6.c | `evt_actor_spawn_wait11` | spawn; wait 11f (entity->unk1C); flag 1 |
| 0x21ca1e | 0221ca1e.c | `evt_actor_spawn_4state` | 4-state: spawn; slot_flag_refresh; re-spawn; countdown-end |
| 0x219916 | 02219916.c | `evt_actor_render` | per-frame draw step; pose (unkF<<8)|unk12; 4 draw helpers inside g_06098878 push/pop |
| 0x21f004 | 0221f004.c | `evt_actor_spawn_child` | one-shot: spawn; flag 0xC; spawn child via func_0220fece(task,&func_0221ad06) |
| 0x220190 | 02220190.c | `spawn_entry_list` | spawn a child per entry of g_06090970[scene->unk28] while entry->unk2>0 |
| 0x247d6e | 02247d6e.c | `actor_block_setup` | slot entity block/guard reaction; rng_next()&7 clamped by ~unkE8&7 |
| 0x2301c4 | 022301c4.c | `spawn_clone_from` | spawn from g_06091354[idx]; copy ~18 fields from a source entity + position |
| 0x233122 | 02233122.c | `evt_timed_dispatch` | task step: init, countdown, INDIRECT (*g_060902A0)(task,arg,g_06088824), end |
| 0x22c322 | 0222c322.c | `actor_fall_setup` | init fall physics; dispatch via fp table g_060911BC[g_0609979A](actor,0xE) |
| 0x23c6bc | 0223c6bc.c | `actor_draw_proximity` | pick pose by player proximity (g_060995B4/B8); vec3_array_matmul draw |

### The evt_actor family shape (0x21cxxx-0x21fxxx)
`arg0` = task; `task->unk8` = entity; `task->unk10` = state byte (switch);
`task->unk0` = step fp (cleared to 0 = terminate). The canonical state-0 prologue is
`entity->unk30 = scene_value_select(); process_spawn_list(entity);` (entity->unk30
holds the chosen spawn-descriptor-list pointer, which process_spawn_list walks). The
later states are per-frame countdowns on `entity->unk18`/`unk1C` that raise bits in
the global event-flags word `g_06098580`, optionally gate sound effects on
`g_060E0DDC`, and advance or terminate the state.

### Notable confirmations
- `func_02210020` is **`rng_next`** (notes/symbols.txt) — its `&7` result in
  `actor_block_setup` is a small random, exactly as modelled.
- `func_02213ca4` is **`vec3_array_matmul`** (the 16.16 transform) — the draw call in
  `actor_draw_proximity` is a 3D transform, consistent.
- Two **indirect dispatches** are decoded faithfully rather than stubbed:
  `actor_fall_setup` calls through the fp table `g_060911BC[g_0609979A]`, and
  `evt_timed_dispatch` calls through the single fp slot `*g_060902A0`.

## Resisted / deferred (honest, with reasons)
- **0x21ce1c, 0x21b080, 0x21f22e, 0x21eaac — MERGED-EXTENT trap.** Each census member
  begins with a tiny tail-call dispatcher (`record = 0x060745xx + scene->unk29*12;`
  then an unconditional `braf` to the shared handler **0x218038**), immediately
  followed by a *separate* full handler body that the forced-disassembly extent walker
  swallowed past the unconditional branch. `find_entry` reports the start as "real" but
  the member actually covers two functions. These need member re-segmentation (split at
  the instruction after the braf delay slot) before they can be written as clean,
  correctly-named files. **Priority-1 next-wave item** — 0x218038 itself is a large,
  already-analysable standalone handler (switch on task->unk10, args r4=task/r5=record).
- **0x21e8c4 / 0x21e8c8 / 0x21e9c6 — pool-contaminated cluster.** `find_entry 0x21e8c4`
  -> real entry **0x21e9c6** (pool-contaminated), yet a clean prologue also sits at
  0x21e8c8. The census member, the true entry and the sibling `func_21e9c6` are tangled;
  writing a file risks a wrong name / double-count. Defer until segmentation is fixed.
- **0x21fed4 — complex transform call.** Fills a 12-byte stack struct from
  func_02214194, offsets the three results by 0x200000, then calls func_0221055c with
  6+ args (four pushed on stack). Faithful but higher-effort; clean target for a
  follow-up.
- **0x2453de — jump-table dispatch.** `bsrf r0` via `mov.b @(r0,r8),r0` (a byte-offset
  jump table). Must be decoded to a real switch, not stubbed; sizeable (244B). Deferred
  for a dedicated pass.
- **0x238e94 — 0x24Axxx runtime band.** Calls 0x24A360 / 0x24A420 (the
  dispatch/divide/copy runtime helpers the standing constraints say to leave alone as
  decomp targets); also an m2c extent failure. Skipped per constraints.
- **0x2300e8 / 0x21fd7a / 0x21f22e — m2c extent failures** ("cannot find branch target"
  / "two delay slots") — these co-occur with the merged-extent issue above and want
  manual segmentation first.

## ABI assumptions recorded in the source (not hidden)
- `actor_block_setup` (0x247d6e): `rng_next` is called nilary (r4 holds leftover
  idx*4 but is not freshly set for the call); and the `tst r0,r0` after the
  `func_02245fc4` call tests **that call's return value** — m2c mis-modelled r0 as the
  preserved `~unkE8`. The compiler would only emit that test if the return is used, so
  the faithful reading is `if (func_02245fc4(...) != 0)`.
- `spawn_clone_from` (0x2301c4): `obj->unk6C = 8` is the delay-slot store of the
  `func_02238dde(obj)` call (store completes before the callee runs).

## Next-band recommendation
1. **Re-segment the braf->0x218038 dispatcher family** (0x21ce1c, 0x21b080, 0x21f22e,
   0x21eaac, and any other member whose body starts with the `record=...; braf 0x218038`
   prologue). Splitting these yields ~8-12 clean functions: the tiny dispatchers plus
   the standalone handler bodies, all in the same well-understood evt-actor idiom.
   Decompile 0x218038 itself first (the shared target) to lock its (task, record) ABI.
2. **Finish the rest of the evt_actor family** in 0x21cxxx-0x21fxxx that are NOT
   braf-prefixed — the same spawn/countdown shape, now fully template-matched here.
3. **Map the 0x20fece / 0x20ff28 / 0x20ff54 spawn-holder helpers once** (all three are
   `(parent/task, template) -> holder; holder->unk8 = entity`). Decompiling these pins
   the holder struct and retroactively firms up `spawn_*` / `evt_actor_spawn_child`.
4. The 0x242xxx-0x247xxx actor band still has clean single-arg members (e.g. around
   0x243xxx) once the misaligned entries (like 0x243588, which `find_entry` shows has a
   6-byte pool-contaminated prologue) are resolved.
