# Readability pass 4

Completed 2026-11-09 against the supplied v1.09 functional tree, preserving the intervening round-5 fixes. Noise falls **21,948 → 20,374**, down **1,574 (7.17%)**. Added **525 semantic function spellings**: **497 definitions** (496 primary definitions plus one existing local helper) and **28 raw extern aliases**. Executable C now contains **zero address-style function identifiers and zero `unkXX` identifiers** outside the alias headers. Added **36 proven fields**, two layout types, **1,291 named access sites**, **33 constants** and three local-variable names. Changed **636 C files**, `dh_names.h`, `dh_work.h`, and the scoped notes.

All **1,099 objects**, all global symbol rows and the complete relocatable linked tree are **byte-identical** to the starting build. The before/after v1.09 scans both report **178 PASS / 890 PASS-void / 0 FAIL / 31 SKIP**, with **zero per-address status changes**. Independently lowered preprocessed token streams match for **1,099/1,099** translation units. [game_architecture.md](game_architecture.md) has the updated subsystem guide and a nine-row “where to look to change X” table.

## Function names and evidence

The initial [caller ranking](../work/readability4/caller_ranking.json) already had no anonymous target with more than one caller file. Pass 4 finishes the local/table-driven tail by subsystem: background/VDP loaders, results/options, scene sequences, player recovery/grab/throw, enemy reaction/motion selectors, projectiles/effects, camera helpers, sound cues and compiler-runtime leaves. Every previously anonymous definition found in the baseline is named. This includes the existing local `02216258` transform-reset helper inside `022161dc.c`; its raw name remains visible after preprocessing so v1.09 still observes its original call boundary.

Examples are `enemy_select_latched_attack`, `enemy_choose_action_and_side`, `player_handle_grab_throw_commands`, `player_tick_damage_recovery`, `scene_tick_branch_drain`, `camera_tick_waypoint_path`, `sound_step_command_cursor`, `model_task_draw_work` and `results_edit_options`. Names express inspected operations. They do not assign speculative enemy species, stage titles, physical button labels or sound recording titles. The old “sound/voice object” description of `02246254/0224627e` is narrowed to the packed-model-ID initialization and model draw that their bodies actually perform.

Confidence is **250 HIGH / 275 MED**, recorded with address and evidence in [symbols.txt](symbols.txt). [function_names.json](../work/readability4/function_names.json) is the complete manifest; [function_evidence.json](../work/readability4/function_evidence.json) preserves baseline source hashes, original comments, call identifiers and reference files. The original symbol map remains an exact byte prefix. All original C evidence comments remain intact.

The 28 raw extern aliases preserve symbols already distinct from descriptive definitions. `_source_alias` also distinguishes duplicate source drafts without merging linker identities or prototypes. Unproved event interior targets retain `_interior`; incomplete event implementations retain `_draft`. The absent `0222f8d0` continuation and shared soft-float entry receive caller-level names with explicit evidence limits. No missing body was invented. The [AST call-graph audit](../work/readability4/callgraph_audit.json) has zero parse errors and identical canonical address-reference multisets in every file; all 277 shared targets keep their existing identities.

## Layouts and access sites

[layout_evidence.json](../work/readability4/layout_evidence.json) lists each field's widths, sites and independent consumers. Duplicate v1.09 source bindings are normalized to callable entries before counting consumers. **Every new field has at least two independent consumers.** [layout_provenance.json](../work/readability4/layout_provenance.json) records the actual accepted bases; equal offsets alone never establish a type. The single-consumer `target_world_x` candidate remains rejected. `0221df80` is excluded from the sequence layout: its `+30` stores a child-task handle rather than a spawn-list cursor, despite sharing timer offsets with the sequence family.

| Layout | New fields | Evidence and use |
|---|---:|---|
| `DhActorWork`, same `0x18c` extent | 25 | Action-pending/source/paired-slot bytes; interaction latch and health; saved primary effect type/slot and primary handoff pair; motion speed; selected effect slot; type flags; animation script; queued attack kind/slot and start tick; nine descriptor/config/table pointer words. Spawn/damage/health transitions, effect channel swaps, attack selectors, descriptor loaders and reaction consumers agree. |
| `DhCameraWork`, same `0xcc` prefix | 2 | `update_handler` at `+64`, installed by framing and called through task trampolines; `horizontal_range` at `+68`, initialized and used to clamp tracking X. |
| `DhModelDefinition`, new `0x14` record | 5 | Vertex/quad counts, vertex/quad guest pointer words and draw-handler index. ROM/runtime loader `02223ac0` and immediate/pending/part renderers `02215cb4/02215bf4/02235dc0` agree. Unknown flag/scratch bytes remain reserved. |
| `DhSceneSequenceWork`, new `0x40` prefix | 4 | Main/phase ticks, spawn cursor and saved branch cursor. Timed/branching scene interpreters and `process_spawn_list` agree; this is not actor work. |

The field-site audit counts **943 actor / 155 camera / 27 model / 166 scene-sequence** substitutions: **1,291 total across 187 C files**. It also reuses previously proven actor/camera members at remaining raw offsets. [field_plan.json](../work/readability4/field_plan.json) retains every original expression and replacement, and [field_site_audit.json](../work/readability4/field_site_audit.json) verifies all planned sites were actually converted.

Player health at `+56` is established by spawn seeding and subtract/clamp consumers, despite older comments calling it a hit timer/readiness word. Saved primary effect fields `+6a/+6b` differ from live fields `+6c..+6f` and pending handoff `+70/+71`. The actor `+ec/+f0/+f4` region serves both saved-pose and motion-vector roles; it was not globally renamed as home position. Camera `+ae` remains handler-dependent slot/countdown storage. The sound ring already accounts for all `0x48` bytes; its missing read-side body prevents additional recovery, so this pass retains the established queue layout and its MED read-index interpretation.

Access wrappers preserve the original `M8/M16/M32/MU/MP/MEM` operation. Signedness, volatility, store order and native-pointer width are unchanged. Layout pointer words remain four bytes; no existing `MP` operation becomes a host struct-pointer load. [check_layouts.c](../work/readability4/check_layouts.c) asserts named offsets, widths and extents; it compiles and runs with C11, `-Wall -Werror`.

## Constants and source clarity

The **33 macros** comprise nine player handler states, three animation-control values, five effect-channel action states, thirteen trigger-named sound commands, one scene-ID gate, the twenty-byte model stride and a decimal spelling of the already proven action-mask group. They replace **203 sites in 78 files**. Numeric notation may differ for equal literals, while original integer types and suffixes are preserved. [constants.json](../work/readability4/constants.json) records definitions/evidence and [constant_sites.json](../work/readability4/constant_sites.json) records substitutions. State names are limited to the proven actor field and handler family; equal integers elsewhere were not globally replaced.

`DH_SCENE_SEGMENTED_MODEL_INTRO` identifies the scene-13 branch in `scene_tick_waypoint_model_intro`; it does not claim a level title. Camera timeline and event-pair sound constants describe trigger paths. Physical action-button labels remain unassigned. [local_names.json](../work/readability4/local_names.json) records the final three `unkXX` locals renamed from their proven slot, action-state and launch-segment uses.

The reported **548 files containing `unkXX`** were predominantly historical comments. **548 still retain those comments**, preserving old offset/ROM evidence; the code-only audit now finds none of those identifiers in executable C. Raw arithmetic and unnamed data globals remain where the evidence is insufficient. Function source files keep their address filenames for disassembly cross-reference.

## Noise v1.00

Both reports parse **1,099/1,099 files** and **1,173 definitions** with zero failures. Code lines stay **36,634**. Zero-noise definitions rise **123 → 163**; noisy lines fall **15,258 → 14,119** (**41.65% → 38.54%**). Header bodies and definition names are excluded by the unchanged metric, so layouts/constants are separately audited above. The fresh starting **21,948** differs from pass 3's **21,955** because the supplied round-5 functional tree is the baseline.

| Category | Before | After | Delta |
|---|---:|---:|---:|
| `raw_memory` | 1,687 | 1,687 | +0 |
| `offset_access` | 14,743 | 13,512 | -1,231 |
| `pointer_constant` | 884 | 884 | +0 |
| `hardware_address` | 224 | 224 | +0 |
| `address_function` | 285 | 0 | -285 |
| `numeric_call` | 92 | 92 | +0 |
| `unnamed_global` | 1,656 | 1,656 | +0 |
| `magic_offset` | 2,182 | 2,124 | -58 |
| `goto` | 82 | 82 | +0 |
| `label` | 39 | 39 | +0 |
| `register_variable` | 41 | 41 | +0 |
| `sign_extension` | 33 | 33 | +0 |
| **Total** | **21,948** | **20,374** | **−1,574** |

No category increases. Reports: [before](../work/readability4/noise_before.json), [after](../work/readability4/noise_after.json), [comparison](../work/readability4/noise_comparison.json). No metric implementation or shared baseline was edited.

## Byte identity and v1.09 agreement

The pass-3 build method is unchanged: compile all sorted source files using `gcc -O1 -c -I <source-directory> <file> -o <object>`, then `ld -r` the sorted object list and compare bytes/global symbol rows with the saved starting build. Before and final both compile **1,099 objects with zero failures**, link successfully, and retain **16 diagnostic-bearing files**. [final_binary_equality.json](../work/readability4/final_binary_equality.json) contains every baseline object hash and the equality result. Earlier retained [name](../work/readability4/names_binary_equality.json) and [layout](../work/readability4/layouts_binary_equality.json) checkpoints also match the same baseline.

The baseline/final linked `tree.o` SHA-256 is:

`5386d4adf7262b456dbe0958a8b8436eec8079e7a83119bca531cf6075e74b8f`

This is byte identity with the supplied functional relocatable build, not a claim of compiler-byte matching to the original SH-2 ROM. The [independent token check](../work/readability4/lowered_token_equality.json) lowers the recorded field substitutions, reverses the three local names, restores the original layout declarations and preprocesses through the aliases/constants. All **1,099 token streams match**, including operators, strings, declarations and integer suffix types.

Full before/after scans use unchanged v1.09 defaults, `--all --jobs 4`, sixteen requested vectors and inherited/additive profiles. Both have **178 PASS / 890 PASS-void / 0 FAIL / 31 SKIP**. [after_status_equality.json](../work/readability4/after_status_equality.json) verifies all 1,099 address/status rows and unchanged cart, tester, ABI, reference, hints/options, seed and input model metadata. Status SHA-256:

`189bdff79900a5bdd7a86474892a32b49cf6394078f53d52f567ec3d017cea42`

A rejected exploratory full scan preserved object bytes but changed `0221df80` from PASS-void to SKIP: active symbol-map rows for unproved interior targets made v1.09 split the preceding function early. Those two aliases remain named in C, with **comment-only evidence** in the symbol map, so no new callable boundary is asserted. A targeted recheck restored PASS-void; the final full scan below uses that corrected map. [review_findings.json](../work/readability4/review_findings.json) records this and the rejected scene-layout base; [interior_boundary_probe_after.json](../work/readability4/interior_boundary_probe_after.json) retains the unsuccessful scan. No tester or hint edit was used.

All recorded effects, inputs, return-check settings and branch coverage agree. Two pre-existing formatter SKIP reasons have only a generated-source line-coordinate change. Two rows (`0222f84e`, `0224a830`) have segmentation-only metadata changes: newly evidenced continuation/runtime entry symbols shorten previously unreached trailing regions. Executed branch counts and all other fields in those rows agree. Both old/new segmentation records and diagnostic strings are retained in the observation audit. [Original-source probes](../work/readability4/metadata_probe_equality.json) reproduce both segmentation changes with unchanged baseline C under the new symbol map, isolating them to symbol discovery. **9 result rows** differ only in generated-source line coordinates of otherwise identical indirect-prototype metadata; their values, columns and arities agree. Both coordinate maps are retained in [result_observation_equality.json](../work/readability4/result_observation_equality.json). Every emitted result source hash matches its frozen input; early SKIPs without hashes are listed in [result_source_hash_equality.json](../work/readability4/result_source_hash_equality.json). The full frozen input manifest is unchanged across the scan. Runs: [before](../work/readability4/before.json), [after](../work/readability4/after.json); per-source statuses: [difftest_by_file.tsv](../work/readability4/difftest_by_file.tsv). PASS remains bounded oracle evidence with the same 31 SKIPs and existing ABI/device/coverage limits.

## Bugs, preserved uncertainty and scope

**No new behavior bug was established or fixed.** Existing incomplete event FSMs (`0221cfa4`, `0221ec08`, `0221edec`) remain explicitly marked drafts and retain v1.09 SKIPs. Two inspected quirks are preserved without declaring them defects: the two-slot camera uses the first slot's vertical bias for the second focus, and `enemy_select_mode_reference` chooses the greater X distance in its mode-3 branch. Existing source comments document other unusual residual/default paths; names do not repair them.

[audit.json](../work/readability4/audit.json) verifies unchanged frozen C/header/symbol inputs, all original C comments, the original symbol-map prefix and all 1,099 source files. [floor_review.json](../work/readability4/floor_review.json) records 262 relevant floor rows as context; no floor was edited or ground through. Tester/decomp tools and `notes/floors.tsv` keep their starting hashes. Concurrent external drift is listed separately in the audit; it is not claimed as pass-4 work. All pass writes are confined to `project/src_functional/`, `notes/symbols.txt`, `notes/game_architecture.md`, this report and `work/readability4/`. Scan attempts remain inside their pass-local output directories. No port/saturn-recomp edits, shared attempt-ledger writes, commits, delegation, GPUs, privileged commands or process termination were used.

Reproduce the verification from the repository root:

```sh
python3 work/readability4/build_check.py recheck
python3 work/readability4/verify_batch.py recheck
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --json work/readability4/recheck.json --output-dir work/readability4/recheck_run
python3 work/readability4/compare_status.py recheck
tools/decomp/venv/bin/python tools/decomp/noise_report.py --src project/src_functional --symbols notes/symbols.txt --json work/readability4/noise_recheck.json --markdown work/readability4/noise_recheck.md
gcc -std=c11 -Wall -Werror -I project/src_functional work/readability4/check_layouts.c -o work/readability4/check_layouts
work/readability4/check_layouts
python3 work/readability4/verify_lowered.py
tools/decomp/venv/bin/python work/readability4/caller_audit.py
python3 work/readability4/site_audit.py
python3 work/readability4/audit.py
python3 work/readability4/finish_report.py
```

1. Noise: 21,948 → 20,374, down 1,574 (7.17%).
2. Named 497 definitions and added 28 linkage-preserving raw extern aliases.
3. Executable C has zero address-style function or `unkXX` identifiers outside alias headers.
4. Added 25 actor fields, two camera fields and two new layouts with nine fields.
5. Converted 1,291 access sites across 187 files; all new fields have multiple independent consumers.
6. Added 33 typed-literal-preserving constants and renamed three unknown locals.
7. Updated the modder guide with subsystem details and a nine-row change-location table.
8. All 1,099 objects, global symbol rows and the linked tree are byte-identical.
9. All tokens and v1.09 statuses agree: 178 PASS / 890 PASS-void / 0 FAIL / 31 SKIP.
10. No behavior bugs fixed, out-of-scope writes, delegated work or commit.
