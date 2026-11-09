# Readability pass 2

Named **61 functions**, **22 additional globals** and **23 input/state constants**. Recovered task, actor, world-pose, player-input and direction-tap layouts in `project/src_functional/dh_work.h`; converted **1,951 field accesses**. Changed **617 C files**, updated `project/src_functional/dh_names.h` and `notes/symbols.txt`, and added this report. Existing pass 1 and unrelated edits were preserved. No commit was made.

Noise falls **26,408 → 24,493**, down **1,915 (7.25%)**. All **1,099** before/after difftest statuses match. All **1,099 GCC object files and the whole-tree relocatable link output are byte-for-byte identical** to the saved starting tree.

**Function naming and address recovery.** Selection began with `tools/decomp/coverage_callgraph.py` and a comment-free ranking of distinct caller translation units, saved in `work/readability2/caller_ranking.json`. The high-use targets include random generation (71 caller TUs), culled model submission (42), child task spawning (34), facing resolution (33), and sprite drawing (33/28). The executive, scene director, player input/combat machine, enemy continuations, collision, camera and display/audio setup were inspected as connected systems.

Each renamed definition has a short purpose/address header. Source spellings are macro aliases to their original `func_XXXXXXXX` linker names; address-based filenames remain unchanged. Existing descriptive definitions and distinct historical extern spellings were preserved. In particular, `actor_anim_frame_update`, `actor_state_dispatch`, `sound_queue_push` and `sound_queue_reset` already had descriptive definitions; they were retained. No unrelated externs were silently unified. `notes/symbols.txt` records the new names, confidence and evidence; older guesses remain historical. The new player-combat, view-transform, texture-upload and look-at names supersede broader or misleading old guesses.

| Address | Source spelling | Caller TUs | Confidence |
|---|---|---:|---|
| `0x02201020` | `game_boot_and_run` | 0 | high |
| `0x02201958` | `game_reset_globals` | 1 | high |
| `0x02205b20` | `scene_advance_task` | 1 | high |
| `0x02202138` | `scene_vdp2_bringup_task` | 0 | high |
| `0x02201c9c` | `scene_palette_fade_task` | 1 | high |
| `0x02201f20` | `scene_display_setup_task` | 0 | high |
| `0x0220fc18` | `task_pool_reset` | 1 | high |
| `0x0220fc3c` | `task_insert_after_current` | 5 | high |
| `0x0220fc82` | `task_insert_after_parent` | 4 | high |
| `0x0220fdc8` | `task_cancel_below_depth` | 17 | high |
| `0x0220fde4` | `task_run_frame` | 1 | high |
| `0x0220feec` | `task_spawn_child_work88` | 34 | high |
| `0x0220fece` | `task_spawn_current_work6c` | 29 | high |
| `0x0220ff28` | `task_spawn_list_work7c` | 28 | high |
| `0x0220ff54` | `task_spawn_current_work40` | 19 | high |
| `0x0220ff72` | `task_spawn_child_work40` | 11 | high |
| `0x0220ff0a` | `task_spawn_actor_work` | 12 | high |
| `0x02214bb8` | `task_alloc_zeroed_array` | 11 | high |
| `0x0220b354` | `task_run_work_handler` | 0 | high |
| `0x0220f990` | `input_reset_records` | 8 | high |
| `0x0220f9d4` | `input_poll_records` | 1 | high |
| `0x0220fb98` | `input_update_direction_tap` | 1 | high |
| `0x02223ee0` | `player_update_combat_input` | 1 | high |
| `0x02227704` | `actor_resolve_facing` | 33 | high |
| `0x0222adaa` | `actor_dispatch_pose_and_type` | 21 | high |
| `0x0222aadc` | `actor_queue_pose_sound` | 1 | high |
| `0x0223a6dc` | `actor_load_pose_animation` | 10 | high |
| `0x0222863e` | `actor_update_task` | 2 | high |
| `0x022469f2` | `enemy_reset_idle_or_walk` | 26 | med |
| `0x022489d0` | `actor_enter_hit_reaction` | 17 | high |
| `0x0221707c` | `actor_register_movement_slot` | 18 | med |
| `0x02228106` | `actor_allocate_effect_slot` | 19 | med |
| `0x0222a878` | `scene_select_player_parameter` | 27 | med |
| `0x02227bc4` | `actor_spawn_fixed_slot` | 5 | high |
| `0x0220cdf6` | `camera_update_slot_orbit` | 0 | high |
| `0x0220cfac` | `camera_update_player_follow` | 0 | high |
| `0x0220d262` | `actor_update_orbit_task` | 0 | high |
| `0x0221055c` | `camera_compute_lookat_angles` | 15 | high |
| `0x022225f6` | `view_build_display_transform` | 1 | high |
| `0x0222ecc4` | `collision_reset_frame` | 1 | high |
| `0x0222ece4` | `collision_sweep_grid` | 1 | high |
| `0x0222eea2` | `collision_queue_actor` | 25 | high |
| `0x0222eee4` | `collision_insert_grid` | 23 | high |
| `0x0222ef38` | `collision_test_pose_hitboxes` | 1 | high |
| `0x0222f1d0` | `collision_test_swept_hitbox` | 1 | high |
| `0x02215a2c` | `model_submit_culled` | 42 | high |
| `0x022153f6` | `sprite_draw_child` | 33 | high |
| `0x022154fc` | `sprite_draw_billboard` | 28 | high |
| `0x022150d4` | `vdp1_upload_texture_groups` | 9 | high |
| `0x02214cdc` | `vdp1_begin_frame` | 1 | high |
| `0x02214d26` | `vdp1_trigger_draw` | 8 | high |
| `0x02214f90` | `vdp1_upload_command_lists` | 1 | high |
| `0x02215da8` | `audio_load_driver` | 1 | high |
| `0x02215e20` | `audio_send_command_buffer` | 6 | high |
| `0x02215fac` | `audio_queue_scene_sound` | 6 | high |
| `0x02210020` | `game_random_next` | 71 | high |
| `0x022131c8` | `fixed_mul16` | 23 | high |
| `0x02213164` | `fixed_sqrt16` | 4 | high |
| `0x02213290` | `memory_clear_vec4_blocks` | 19 | high |
| `0x0224a360` | `shc_shift_left` | 23 | high |
| `0x0224a420` | `shc_shift_right_signed` | 19 | high |

Full per-name evidence: `notes/symbols.txt` and `work/readability2/function_names.json`. The raw callgraph tool does not expand header aliases: its referenced-name count changes 999 → 984, while file coverage stays 99.4%, with the same seven undefined entries. Restoring the documented aliases gives exactly the original address references in every C file (`work/readability2/callgraph_alias_equality.json`). This is naming visibility, not a coverage gain.

**Recovered layouts.** All offsets are guest byte offsets. Unknown bytes remain explicit `reserved_*` storage. No packing directives, bitfields, pointer-size assumptions or function signatures were introduced. `work/readability2/check_layouts.c` uses C11 assertions to verify every recovered offset used by the conversion, unsigned-accessor offsets, and record sizes.

| Layout | Size / known range | Named offsets and evidence |
|---|---|---|
| `DhTaskRecord` | `0x20` | `00 handler`, `04 next`, `08 work`, `0e depth`, `10 state`, `14 parent`; node constructors `0220fc3c`, `0220fc82`, `0220fcc4` and runner `0220fde4`. |
| `DhWorldPose` | Pose fields through `0x29` | `08/0c/10 position_x/y/z`; `24/26/28 angle_x/y/z`; orbit `0220d262`, camera initialiser `0220b1c0`, view builder `022225f6`. |
| `DhActorWork` | `0x18c` allocation | Same pose; `14/18/1c movement_x/y/z`; `34 slot_id`; `40 interpolation_ticks`; `41 animation_control`; `43 state_handler`; `44 facing_right`; `4e hit_shake_ticks`; `54 animation_events`; `58 animation_frame`; `5a animation_current`; `5c animation_requested`; `84 action_category`; `88 action_state`; `e0 state_flags`; `e4 state_index`. |
| `DhPlayerInput` | `0x18`, five records | `00 buttons`, `02 injected_buttons`, `04 pressed`, `06 released`, `0c direction_hold_frames`, `0e direction_change_frames`, `10/12 button1/2_hold_frames`, `16 latched_direction`; updater `0220f9d4`, resetter `0220f990`, player consumer `02223ee0`. |
| `DhDirectionTap` | `6`, two records | `02 phase`, `04 hold_frames`, `05 fired`; `0220fb98` and its player caller. `00..01` remain unknown. |

Actor allocation/slot evidence comes from `0220ff0a` and `02227bc4`. `022170ea` integrates and clears the movement accumulators. `0222dc2c` establishes animation fields, `02227704` establishes facing, `0222863e` establishes state dispatch/hit-shake fields, and `02223ee0` establishes category/state. `02244e2e` uses state-index dispatch and bit `0x40` of state flags to suppress dispatch.

Typed numeric accesses use `ACTOR_WORK(p)->field`, `WORLD_POSE(p)->field`, `PLAYER_INPUT(p)->field` and `DIRECTION_TAP(p)->field`. Unsigned actor word reads use `ACTOR_U16`, preserving the original unsigned lvalue. Guest task image pointer words are `uint32_t`; task accesses use `TASK_PTR`, `TASK_S32`, `TASK_S8` or `TASK_U8` to preserve the existing per-site native-pointer/word model and signedness. The image struct is not imposed on existing host pointer slots. `TASK_WORK` remains recoverable as the original offset alias.

Task `+8` is used as work by handlers and as an owned-allocation chain by heap cleanup. The layouts document both uses without changing either. Camera and short-lived effect/hitbox work do not receive the full actor tail. The actor spawn-list controller `0222a120` was explicitly excluded from actor-layout conversion; its work has a different shape.

| Conversion kind | Access sites |
|---|---:|
| `task` | 1,121 |
| `world_pose` | 60 |
| `input` | 46 |
| `tap` | 7 |
| `actor` | 641 |
| `actor_unsigned` | 76 |

Exact file/base/member/accessor substitutions: `work/readability2/struct_sites.json`. The candidate pilot preserved PASS-void for input polling, direction-tap update, player combat input and facing; the animation driver kept its original compiler-runtime SKIP.

**Globals and constants.** Additional names cover the task list/count, PRNG state, shared collision node pool/count/list/grid, effect-slot occupancy, player ready/join/tap state, actor/pose/type/enemy dispatch tables, actor movement records, audio latch/readiness and sound command ring. Original C spellings, per-TU types, qualifiers and unsigned literal suffixes remain intact. Exact address/evidence list: `work/readability2/global_names.json` and `notes/symbols.txt`.

Input masks name the proven X/Z axes, horizontal/depth/direction groups, numbered action bits, and system P1/P2 start bits. Physical punch/kick/jump labels were not inferred. State constants name the four direction-tap phases and the display sequencer’s configure/wait/notify states. Scene constants name index 81 as the director stop entry and 76 as its first nonplay index; no stage title was guessed. They remain macro constants, preserving the original integer literal types and preprocessed operations.

**Noise report v1.00.** Both runs parse 1,099/1,099 files and 1,172 definitions with zero failures. Definition names are excluded by the metric, so function-name gains count at actual uses. Many converted task accesses already used pass 1 offset constants and were already clean. The function identity comparison records 61 renames as additions/removals; no function was actually added or removed.

| Category | Before | After | Delta |
|---|---:|---:|---:|
| `raw_memory` | 1,747 | 1,676 | -71 |
| `offset_access` | 16,380 | 15,699 | -681 |
| `pointer_constant` | 913 | 889 | -24 |
| `hardware_address` | 224 | 224 | +0 |
| `address_function` | 2,817 | 1,813 | -1,004 |
| `numeric_call` | 94 | 92 | -2 |
| `unnamed_global` | 1,790 | 1,657 | -133 |
| `magic_offset` | 2,249 | 2,249 | +0 |
| `goto` | 81 | 81 | +0 |
| `label` | 38 | 38 | +0 |
| `register_variable` | 46 | 46 | +0 |
| `sign_extension` | 29 | 29 | +0 |
| **Total** | **26,408** | **24,493** | **-1,915** |

Zero-noise definitions: **69 → 104** (5.89% → 8.87%). Noisy lines: **18,295 → 17,077** (50.06% → 46.73%). Code-line denominator remains **36,545**. No category increased.

Artifacts: `work/readability2/noise_before.json`, `work/readability2/noise_after.json`, their `.md` reports, and `work/readability2/noise_comparison.json`.

**Batch verification.** Default 16-vector configuration, identical cart/harness/reference hashes, seed and model. Every touched file was measured before edits and after its batch. All status comparisons are exact, including PASS versus PASS-void. Every result field other than the edited source SHA-256 is identical, including failure diffs and SKIP reasons. No batch required rollback.

| Batch | C files edited | Files tested | PASS | PASS-void | FAIL | SKIP | Status changes |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1: Function spellings | 476 | 1099 | 110 | 675 | 16 | 298 | 0 |
| 2: Recovered fields | 339 | 339 | 13 | 240 | 6 | 80 | 0 |
| 3: Globals/constants | 112 | 112 | 17 | 60 | 1 | 34 | 0 |

| Status | Whole tree before | Whole tree after | Touched files before | Touched files after |
|---|---:|---:|---:|---:|
| PASS | 110 | 110 | 47 | 47 |
| PASS-void | 675 | 675 | 417 | 417 |
| FAIL | 16 | 16 | 7 | 7 |
| SKIP | 298 | 298 | 146 | 146 |

All 1,099 per-address rows, including a touched-file flag and batch statuses: `work/readability2/difftest_by_file.tsv`. Raw full runs: `work/readability2/before.json`, `work/readability2/after.json`; raw batches: `work/readability2/batch1.json`, `work/readability2/batch2.json`, `work/readability2/batch3.json`. Equality/configuration summary: `work/readability2/status_equality.json`. Scan exit status 1 reflects the same pre-existing failures.

Compile gate: `gcc -O1 -c -I project/src_functional <file> -o <object>` for each TU. **1,099 compile; zero failures; the same 17 TUs emit diagnostics.** `ld -r` over all objects succeeds before and after. All individual object bytes, the complete linked `tree.o`, and global symbol rows match exactly (`work/readability2/binary_equality.json`). This verifies the repository’s relocatable build; it does not introduce a standalone executable link target.

Naming batch: all 1,099 GCC-preprocessed token streams match the saved starting tree (`work/readability2/batch1_tokens.json`). Final tree: mechanically lower only the recorded field conversions back to their original accessors; all 1,099 token streams match, after equivalent integer notation normalization (`work/readability2/lowered_tokens.json`). Offsets and widths are independently checked by layout assertions. Exact object identity provides an additional check independent of lowering and of difftest SKIPs.

Reproduction commands (run from repository root):

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --json work/readability2/after.json --output-dir work/readability2/after_run
tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline compare --baseline-file work/readability2/noise_before.json --markdown work/readability2/noise_after.md --json work/readability2/noise_after.json
python3 work/readability2/build_check.py after
python3 work/readability2/verify_struct_equivalence.py
gcc -std=c11 -Wall -Werror -I project/src_functional work/readability2/check_layouts.c -o work/readability2/check_layouts
work/readability2/check_layouts
python3 work/readability2/finish_report.py
```

**Open questions and boundaries.**

- The actor/camera/effect tails overlap in address but not meaning; unknown bytes and unproven bases remain raw. More constructors and readers are needed before expanding the layouts.
- Input words at `+8`, `+a` and `+14`, and direction-tap word `+0`, remain unknown. Input timer words are also copied by actor/UI consumers; those uses do not prove screen-coordinate fields.
- Task `+8` has distinct work/allocation-chain uses, and existing host pointer accessors have host-width behavior. This pass preserves that model rather than imposing guest pointer widths.
- Numbered action bits are proven; physical punch/kick/jump labels and stage identities still need independent evidence.
- The sound queue service body at `02215f36` is absent from the C tree. The companion read-index name has medium confidence based on reset/pair structure and the existing symbol map; command transport/init/queue push are directly inspected.
- Medium-confidence function names mark structural roles without assigning an enemy species, animation meaning or scene title. Original FAIL/SKIP causes and the floor registry were left unchanged.

All task edits are confined to `project/src_functional/`, `notes/symbols.txt`, `notes/readability_pass2.md` and `work/`. No port files, tools, floor entries or commits were changed.
