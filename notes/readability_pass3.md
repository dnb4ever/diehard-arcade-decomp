# Readability pass 3

Completed 2026-11-09 against the uncommitted v1.07 tree supplied for this task. Added **414 evidence-based source spellings**: **393 anonymous definitions renamed**, with one-line purpose/address headers, and **21 raw extern aliases** beside already descriptive definitions. Extended the actor work with **27 fields**, added **seven layout types**, converted **988 offset access sites plus five sound-queue expressions**, and added **36 constants** including two suffix-preserving variants. Changed **744 C files**, `dh_names.h`, `dh_work.h` and appended to `notes/symbols.txt`. [game_architecture.md](game_architecture.md) is the accompanying guided map for modders.

Noise falls **24,495 → 21,955**, down **2,540 (10.37%)**. All **1,099 objects**, global symbol rows and the complete relocatable link output are byte-identical to the saved starting tree. All **1,099 per-address difftest statuses** are unchanged, and independently lowered preprocessed tokens match in every translation unit.

## Naming by call-graph importance

Selection began with a comment-free ranking of remaining address-style references, including callback values. The first cohort covers every target in that ranking with at least two caller files; boot/executive, scene, camera, render, sound and enemy continuations followed. The final AST audit excludes prototypes and counts identifiers inside function bodies. It finds **224 previously anonymous definitions with two or more caller files; all now have semantic names**. Across all **277 shared targets**, none has an anonymous definition remaining, and no shared target lacks a C definition. Every file's canonical function-address references remain exactly equal to the baseline.

Examples of high-use names are `actor_commit_effect_handoffs`, `actor_dispatch_action_and_finish`, `effect_pool_spawn`, `matrix_translate_current`, `collision_sweep_pose`, `actor_select_directional_animation`, `enemy_attack_is_armed` and `enemy_classify_attack_timing`. The executive stage functions and enemy wave/category/behavior handlers now have searchable names even when called only through task or ROM tables. The architecture guide links the important routes and distinguishes their state fields.

All new names retain original C linker identities through macros. Existing descriptive definitions remain descriptive. The 21 `_entry` aliases, and collision-avoiding `_raw_entry` spellings where needed, preserve raw externs that were already distinct C symbols; this pass does not unify their prototypes or link names. The original symbols file is an exact byte prefix of the final file. Original C evidence comments are all preserved.

Confidence: **269 HIGH / 145 MED**. Medium names describe inspected structural roles such as a recovery, handoff or response; they do not assign guessed enemy species, stage titles or sound sample identities. Every name has its address, confidence and evidence in [symbols.txt](symbols.txt), with the complete machine-readable manifest in [function_names.json](../work/readability3/function_names.json).

Artifacts: [initial ranking](../work/readability3/caller_ranking.json), [AST/reference audit](../work/readability3/callgraph_audit.json), [floor review](../work/readability3/floor_review.json). The 237 relevant floor rows were reviewed for context; no functional repair or floor-status edit was attempted.

## Recovered layouts and access conversions

The [offset/width inventory](../work/readability3/offset_width_histogram.json) scans raw accessor expressions across all 1,099 translation units and retains file, base, byte offset, width, signedness/accessor and line. Computed/unresolved bases remain visible rather than being silently assigned a layout. [layout_evidence.json](../work/readability3/layout_evidence.json) groups the explicitly established bases by layout, showing site counts and every agreeing file. **Every newly converted field has matching width and use evidence in at least two independent source files.** Rejected candidates are retained alongside accepted ones.

| Layout | Extent | Recovered fields and independent users |
|---|---:|---|
| `DhActorWork`, extended | `0x18c` | 27 further fields: render flags; hit pending; direction/model selectors; motion pitch/heading; primary/secondary effect type/slot; interaction flags; action ticks; behavior state/phase; action phase; event count/tick; player/effect/ordinary target slots; XZ target offsets; four descriptor animation words. Spawn, palette, motion, handoff, enemy-state, timing and descriptor users agree. |
| `DhCameraWork` | prefix `0xcc` | World pose, stage origin, focus, orbit offsets, request/shake timers and target eye/angles. Initialization `0220b1c0/0220bc20`; framing `0220b3cc/0220b8d4/0220be2c`; easing `0220e0aa`; orbit/follow `0220cdf6/0220cfac`. |
| `DhSceneDescriptor` | `0x50` stride | Resource lists at `00/04/08`, sound resource `0c`, camera definition `10`, stage index `20`, view radius `24`, pose word `28` and overlapping angle byte `29`. Resource loaders and scene/camera consumers agree. |
| `DhEnemyActionDescriptor` | `0x44` source span | Four animation words copied into actor `148/14a/14c/14e`, consumed by idle/approach/state-2/state-8 setters. Other source bytes remain reserved; the actor tail's later pointer order is not assumed to match this source record. |
| `DhVdp1CommandEntry` | `0x24` | Control, draw mode, colour, character address/size, four packed vertex words and software sort key. Immediate and deferred sprite builders plus frame headers agree. |
| `DhSpriteDrawRecord` | `0x30` | Four packed vertices, depth, descriptor pointer word and mode bits. Both forward/reverse command builders agree; depth `-1` terminates the list. |
| `DhSpriteCharacter` | `0x10` | Character address/size, dimensions and draw mode, agreed by immediate and deferred sprite consumers. |
| `DhSoundCommandQueue` | `0x48` | Two index words plus sixteen whole command words, from reset/push. The pre-existing read-index meaning retains MED confidence because its consumer body is absent. |

Unknown regions have explicit `reserved_*` bytes. The scene word/byte overlap uses a documented union; accessors address the original bytes without imposing a host-endian interpretation. Camera `+ae` remains reserved because its slot and definition-countdown roles overlap. The task/wave/branch tail of the scene descriptor remains raw where only one independent consumer base was established. Type-specific actor descriptors and short effect/controller work are not cast wholesale to a full actor.

Width-specific accessors such as `ACTOR_M32(p, behavior_state)`, `CAMERA_M32(p, target_x)`, `SCENE_MEM32(p, model_list)` and `VDP1_COMMAND_MEM16(p, control)` expand to the **same original accessors** with named offsets. This preserves signedness, volatility, pointer width, alias behavior and store order. In particular, `MP` retains its native-pointer model; guest layout pointer words remain 32-bit. No signature, control flow, allocation, packing directive or device access order changed.

Exact conversions: [field_plan.json](../work/readability3/field_plan.json), [sound_struct_sites.json](../work/readability3/sound_struct_sites.json). [check_layouts.c](../work/readability3/check_layouts.c) asserts recovered member offsets, widths and all layout sizes, including the scene overlap and sound queue. The checks compile with C11, `-Wall -Werror`, and run successfully. The source descriptor's four animation fields share the corresponding actor-field evidence.

## Constants and IDs

The 36 macros comprise **34 semantic constants and two unsigned-literal variants**. They cover three scene index roles, thirteen enemy behavior states, three fixed animation entry/wait poses plus the absent-entry sentinel, two effect types with proven capacity limits, the actor type mask, nine sound command/base roles and two sound-ring bounds. Four input/debug consumers now reuse the existing proven action/direction masks.

Names intentionally state what the inspected code proves: scene 52 bypasses saved-action restoration; effect types 13/14 use tighter pool gates; fixed sound names identify their triggering path. Physical button labels, stage titles, enemy species and unidentified sample names remain unassigned. Constants are macros so the original integer literal types, explicit unsigned suffixes and casts are retained. Evidence and exact substitutions: [constants.json](../work/readability3/constants.json), [constant_sites.json](../work/readability3/constant_sites.json).

## Noise report v1.00

Both measurements parse **1,099/1,099 files** and **1,173 definitions**, with zero parse failures. Function names themselves are excluded from the metric; their actual call/callback uses account for the improvement. Header bodies are also excluded, so layout definitions and constants are audited separately through the manifests and assertions above.

| Category | Before | After | Delta |
|---|---:|---:|---:|
| `raw_memory` | 1,687 | 1,687 | +0 |
| `offset_access` | 15,676 | 14,753 | -923 |
| `pointer_constant` | 888 | 884 | -4 |
| `hardware_address` | 224 | 224 | +0 |
| `address_function` | 1,827 | 281 | -1,546 |
| `numeric_call` | 92 | 92 | +0 |
| `unnamed_global` | 1,661 | 1,661 | +0 |
| `magic_offset` | 2,249 | 2,182 | -67 |
| `goto` | 82 | 82 | +0 |
| `label` | 39 | 39 | +0 |
| `register_variable` | 41 | 41 | +0 |
| `sign_extension` | 29 | 29 | +0 |
| **Total** | **24,495** | **21,955** | **−2,540** |

Zero-noise definitions increase **109 → 125** (9.29% → 10.66%). Noisy lines decrease **17,088 → 15,263** (46.56% → 41.59%). The code-line denominator remains **36,703**. No category increases. The fresh starting value differs from pass 2's 24,493 because the intervening verified functional rounds are already part of this task's baseline.

Reports: [before](../work/readability3/noise_before.json), [after](../work/readability3/noise_after.json), [comparison](../work/readability3/noise_comparison.json). No tool or shared noise baseline was edited.

## Batch and whole-tree verification

Each batch was built and compared with the saved starting tree before the next batch. A full default v1.07 scan was run before edits and after each batch, always with `--all --jobs 4`, the same requested sixteen vectors and unchanged inherited/additive profiles. All four scans have **157 PASS / 774 PASS-void / 0 FAIL / 168 SKIP**, with **zero per-address status changes** and unchanged cart, tester, ABI, reference, hint, seed and input-model metadata. No failing source batch was retained and no batch required rollback.

| Batch | C files changed in batch | Objects different | Linked tree different | Difftest status changes |
|---|---:|---:|---:|---:|
| 1: call-graph/path names | 702 | 0 / 1,099 | No | 0 / 1,099 |
| 2: layouts/accessors | 170 | 0 / 1,099 | No | 0 / 1,099 |
| 3: constants/executive continuations | 139 | 0 / 1,099 | No | 0 / 1,099 |

The build command matches pass 2: `gcc -O1 -c -I <source-directory> <file> -o <object>`, followed by `ld -r` over the sorted objects. Each build compiles **1,099 files with zero failures**; the same **16 files** emit diagnostics. All global symbol rows match. This is the repository's relocatable whole-tree build, not a new standalone executable target or a claim of byte matching the original SH-2 ROM.

The complete linked `tree.o` SHA-256, identical before and after all three batches:

`44c7807ccf3dc00d4a22912e86356401ec92e025f358cc6a90def005ea6e7532`

Independent token gate: mechanically lower only the recorded field/sound substitutions back to their original accessors, restore the original layout declarations for that comparison, then preprocess using the new name/constant aliases. **All 1,099 token streams match**, retaining operator boundaries, literal suffix types, strings and declarations. This is independent of the object-byte gate and the difftest SKIPs. See [lowered_token_equality.json](../work/readability3/lowered_token_equality.json).

All result observations and coverage fields also agree. Nine result rows have only a **+128 generated-source line shift** in indirect-prototype metadata keys after the layout declarations grew; their columns, arities and return descriptions agree exactly. No calls, effects, vector inputs, branch observations or SKIP reasons changed. The [observation audit](../work/readability3/result_observation_equality.json) retains both coordinate maps rather than hiding that metadata difference.

Per-address results: [difftest_by_file.tsv](../work/readability3/difftest_by_file.tsv). Full runs: [before](../work/readability3/before.json), [batch 1](../work/readability3/batch1.json), [batch 2](../work/readability3/batch2.json), [after](../work/readability3/after.json). Configuration/status equality: [final gate](../work/readability3/after_status_equality.json). The canonical address/status SHA-256 is `f48f01978e6da195eb7ae5ce746bd79dc6b93c169e7b7644a2e395b68899212f` in all four runs. All 4,396 scan attempt rows were appended to the existing journals without replacing prior work.

Object-level manifests: [batch 1](../work/readability3/batch1_binary_equality.json), [batch 2](../work/readability3/batch2_binary_equality.json), [batch 3](../work/readability3/batch3_binary_equality.json).

## Preservation, remaining uncertainty and reproduction

The original uncommitted sources and symbols were saved under `work/readability3/original/` before editing. All original evidence comments remain, all original symbol-map bytes remain, and no source file or function body was added or removed. Separate source/header/symbol hashes guard the final scan's inputs. Original difftest FAIL/SKIP causes, existing verification fixes and the floor registry were not repaired or rewritten.

Some lower-degree and table-only functions still retain address spellings; remaining ambiguous layouts are not named solely from equal offsets. The architecture guide documents the missing display-executive entry and sound queue consumer, the scene-81/director-stop distinction, multiple actor state layers, separate movement/combat grids, and hardware packets versus software draw records. PASS remains bounded oracle evidence, with the same ABI/device/pointer/coverage limitations documented in [decomp_round3_20261108.md](decomp_round3_20261108.md).

Files outside this task's scope changed during the run, including port/convergence work and status notes. **This pass did not edit or revert them.** [audit.json](../work/readability3/audit.json) retains the observed external hash drift instead of claiming that the entire shared workspace stayed frozen. The tester/tool sources and `notes/floors.tsv` retain their starting hashes. This pass's writes are confined to `project/src_functional/`, `notes/symbols.txt`, these two deliverables and `work/readability3/` plus append-only scan journals in `work/attempts/`. No commit, delegation, GPU, privileged command or process termination was used.

Reproduce from the repository root:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --json work/readability3/recheck.json --output-dir work/readability3/recheck_run
tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline compare --baseline-file work/readability3/noise_before.json --markdown work/readability3/noise_recheck.md --json work/readability3/noise_recheck.json
python3 work/readability3/build_check.py recheck
python3 work/readability3/verify_batch.py recheck
python3 work/readability3/compare_status.py recheck
gcc -std=c11 -Wall -Werror -I project/src_functional work/readability3/check_layouts.c -o work/readability3/check_layouts
work/readability3/check_layouts
python3 work/readability3/verify_lowered.py
tools/decomp/venv/bin/python work/readability3/caller_audit.py
python3 work/readability3/audit.py
python3 work/readability3/finish_report.py
```

## Ten-line summary

1. Noise: 24,495 → 21,955, down 2,540 (10.37%).
2. Added 414 source spellings: 393 definitions and 21 preserved raw extern aliases.
3. All 224 previously anonymous definitions with multiple caller files are named.
4. Extended actor work by 27 fields and added seven recovered layout types.
5. Converted 988 offset sites plus five sound-queue expressions with original access semantics.
6. Added 36 constants and reused proven input masks in four consumers.
7. Added the linked modder guide, notes/game_architecture.md.
8. All 1,099 objects, global symbols and the relocatable linked tree are byte-identical.
9. All token streams and difftest statuses agree: 157 PASS / 774 PASS-void / 0 FAIL / 168 SKIP.
10. Prior functional work is preserved; no commit or task-attributable out-of-scope edits.
