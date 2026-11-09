# Readability pass 1

Applied **60 semantic naming targets**: 20 repeated globals, 26 Saturn hardware registers and 14 task/object fields. Changed 621 C translation units and added the macro-only shared [dh_names.h](../project/src_functional/dh_names.h), included by all three existing shared headers. The substitutions name **1,385 field-access sites**. No functions were renamed.

The raw-address ranking in `notes/noise_report.md` selected the global targets. Existing aliases were checked against users/writers; preferred names below supersede conflicting historical suggestions such as `g_drawPriority` for the matrix-stack cursor, or color/mode names for the UI column/row. Uncertain meanings and unrelated layouts remain raw.

Each new global name is a shared macro alias for its original extern symbol. `_legacy` preserves the second hex-case spelling where the source tree used distinct C identifiers for the same guest location. Every extern type, signedness, volatility and linker spelling remains intact. Absolute-address constants retain the original integer literal type (`_U` at originally unsigned sites). Field constants retain the original accessor and offset, without introducing a host-dependent struct layout.

## Globals: chosen names and evidence

Address constants use `DH_<SEMANTIC_NAME>_ADDR` (and `_U` where needed); both numeric-address accesses and raw extern uses were updated throughout the source tree. All semantic extern spellings are recorded in [symbols.txt](symbols.txt).

| Guest address | Semantic extern name(s) | Evidence |
|---|---|---|
| `0x06098878` | `g_matrix_stack_cursor` | 02214a2e.c: push advances cursor by 48 bytes and duplicates 12 longs. |
| `0x060997a4` | `g_ui_cursor_column`, `g_ui_cursor_column_legacy` | 022095cc.c: glyph marker columns are 0x16 (P1) and 0x21 (P2). |
| `0x060997a8` | `g_ui_cursor_row`, `g_ui_cursor_row_legacy` | 022095cc.c: glyph rows use i, i+2, i+4 and button-grid rows 8/10. |
| `0x06099584` | `g_actor_slot_registry` | 02227bc4.c: reserves slot id at +0xC and task pointer at +0x2C. |
| `0x060e0ddc` | `g_sound_effects_enabled`, `g_sound_effects_enabled_legacy` | 02215fac.c and 02211b04.c: gates sound_queue_push requests; boot sets it to 1. |
| `0x0609882c` | `g_view_task`, `g_view_task_legacy` | 022225f6.c: work at task+8 supplies the position and angles for the view transform; 0220b1c0.c requests camera reset. |
| `0x0604ebe4` | `g_actor_slot_masks`, `g_actor_slot_masks_legacy` | 0220c17c.c: per-slot masks are ANDed with active-slot bits during camera framing. |
| `0x06098580` | `g_scene_event_flags` | 02202138.c raises 0x10000/0x20000 event bits; 022083a0.c consumes and clears them. |
| `0x060995b0` | `g_actor_slot_tasks`, `g_actor_slot_tasks_legacy` | 0220cdf6.c: indexed task pointers, task+8 supplies actor world position and state. |
| `0x06098b1c` | `g_transformed_vector`, `g_transformed_vector_legacy` | 02210690.c: three outputs consumed after transforming X/Z basis vectors. |
| `0x060987bc` | `g_frame_tick`, `g_frame_tick_legacy` | 02223c74.c increments the frame counter; actor timestamp comparisons and blink gates read it. |
| `0x06098f08` | `g_vdp2_register_shadow`, `g_vdp2_register_shadow_legacy` | 02202138.c programs shadow display registers; 02201f20.c copies TVMD to hardware. |
| `0x0609866c` | `g_scene_descriptor`, `g_scene_descriptor_legacy` | 0220b1c0.c: descriptor fields +0x28/+0x29 select per-scene position and angle records. |
| `0x06098830` | `g_view_request_priority` | 0220b1c0.c seeds 10 then accepts request 3; 02244d0a.c accepts request 1 only when current priority >=1. |
| `0x06098670` | `g_play_session_mode` | 0222a8d8.c: zero treats both players ready; nonzero checks BIOS player states; camera init selects in-play poses. |
| `0x06099510` | `g_display_transfer_cursor` | 022079d0.c: reserve_coef returns current word pointer then publishes pointer+1 before filling it. |
| `0x06098668` | `g_scene_index` | 02201958.c clears it; 02205b20.c is the scene director (existing g_scene_index alias). |
| `0x06099590` | `g_actor_slot_ids` | 02227bc4.c: registry+0xC holds occupied id, alias address 0x06099590; 0220cdf6.c checks occupancy. |
| `0x060987ec` | `g_bios_status_buffer`, `g_bios_status_buffer_legacy` | 02201958.c passes byte buffer to BIOS callback at 0x06000634 and decodes language/difficulty from it. |
| `0x06098734` | `g_player_input_records` | 02223ee0.c: 0x18-byte records per player supply current buttons and edge/combo data. |

## Hardware registers

Names and offsets were checked against Sega manuals: [VDP1 address map](https://www.infochunk.com/saturn/segahtml_en/hard/vdp1/hon/p02_12.htm), [VDP2 TVMD](https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p02_40.htm), [VDP2 CCCTL](https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p12_14.htm), [SCU register list](https://www.infochunk.com/saturn/segahtml_en/hard/scu_/hon/p03_01.htm) / [DMA registers](https://www.infochunk.com/saturn/segahtml_en/hard/scu_/hon/p03_02.htm), [SMPC SH-2 interface](https://www.infochunk.com/saturn/segahtml_en/hard/smpc/hon/p01_20.htm), and [SCSP startup/common control](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p01_20.htm). The table paraphrases register identities; code use confirms the mapping.

| Guest address | Named constant | One-line evidence |
|---|---|---|
| `0x25d00000` | `VDP1_TVMR_ADDR` | TV mode selection; mapped accesses in 02223cf0.c / 02223d98.c. |
| `0x25d00002` | `VDP1_FBCR_ADDR` | framebuffer change control; mapped accesses in 02223cf0.c. |
| `0x25d00004` | `VDP1_PTMR_ADDR` | plot trigger; mapped accesses in 02214cdc.c / 02214d26.c. |
| `0x25d00006` | `VDP1_EWDR_ADDR` | erase/write data; mapped accesses in 02214cdc.c / 02214d26.c. |
| `0x25d00008` | `VDP1_EWLR_ADDR` | erase/write upper-left coordinate; mapped accesses in 02214cdc.c / 02214d26.c. |
| `0x25d0000a` | `VDP1_EWRR_ADDR` | erase/write lower-right coordinate; mapped accesses in 02214cdc.c / 02214d26.c. |
| `0x25d00010` | `VDP1_EDSR_ADDR` | transfer end status (manual offset 0x100010); mapped accesses in 02214f90.c. |
| `0x25f80000` | `VDP2_TVMD_ADDR` | TV screen mode; mapped accesses in 02201c9c.c / 02201f20.c / 02202138.c. |
| `0x25f800ec` | `VDP2_CCCTL_ADDR` | color calculation control; mapped accesses in 02223dca.c. |
| `0x25fe0000` | `SCU_D0R_ADDR` | level-0 DMA read address; mapped accesses in 0221478a.c. |
| `0x25fe0004` | `SCU_D0W_ADDR` | level-0 DMA write address; mapped accesses in 0221478a.c / 0221482e.c. |
| `0x25fe0008` | `SCU_D0C_ADDR` | level-0 DMA count; mapped accesses in 0221478a.c. |
| `0x25fe000c` | `SCU_D0AD_ADDR` | level-0 DMA address addition; mapped accesses in 0221478a.c / 0221482e.c. |
| `0x25fe0010` | `SCU_D0EN_ADDR` | level-0 DMA enable; mapped accesses in 0221478a.c / 0221482e.c. |
| `0x25fe0014` | `SCU_D0MD_ADDR` | level-0 DMA mode; mapped accesses in 0221478a.c / 0221482e.c. |
| `0x25fe007c` | `SCU_DSTA_ADDR` | DMA status; mapped accesses in 0221478a.c / 0221482e.c. |
| `0x25fe0090` | `SCU_T0C_ADDR` | timer-0 compare; mapped accesses in 02201020.c / 02223dca.c / 0223e5cc.c. |
| `0x25fe0098` | `SCU_T1MD_ADDR` | timer mode; mapped accesses in 02201020.c. |
| `0x20100063` | `SMPC_SF_ADDR` | status flag; mapped accesses in 0220acf6.c / 02214918.c / 02214944.c. |
| `0x2010001f` | `SMPC_COMREG_ADDR` | command register; mapped accesses in 0220acf6.c / 02214918.c / 02214944.c. |
| `0x20100001` | `SMPC_IREG0_ADDR` | input register 0; mapped accesses in 0220acf6.c. |
| `0x20100003` | `SMPC_IREG1_ADDR` | input register 1; mapped accesses in 0220acf6.c. |
| `0x20100005` | `SMPC_IREG2_ADDR` | input register 2; mapped accesses in 0220acf6.c. |
| `0x20100027` | `SMPC_OREG3_ADDR` | output register 3 (OREG0 is +0x21); mapped accesses in 0220acf6.c. |
| `0x20100077` | `SMPC_PDR2_ADDR` | port-2 data; mapped accesses in 02214970.c / 02214980.c. |
| `0x25b00400` | `SCSP_COMMON_CONTROL_ADDR` | common control word: MEM4MB, DAC18B, version, master volume; mapped accesses in 02215da8.c. |

Existing register externs also use descriptive `reg_<chip>_<register>` aliases; their original C linkage is retained. SMPC byte address `0x20100027` is **OREG3**, counting from OREG0 at `0x20100021`. VDP1 EDSR remains at the source/manual address `0x25d00010`. Corrected misleading inline PTMR/EWLR/EWRR comments in `02214d26.c`; register writes are unchanged.

## Object fields

| Offset | Name | Evidence / scope |
|---|---|---|
| `0x00` | `TASK_HANDLER` | task node callback; 0220fde4.c calls handler(node); null marks dead node. |
| `0x04` | `TASK_NEXT` | task node link; 0220fc3c.c splices node/anchor through +4. |
| `0x08` | `TASK_WORK` | task payload/work pointer; constructors clear +8 and handlers load entity from it. |
| `0x10` | `TASK_STATE` | task byte state; 02202a28.c uses it as keyframe/state index. |
| `0x14` | `TASK_PARENT` | task parent pointer; 0220fc82.c stores parent at +0x14. |
| `0x0e` | `TASK_DEPTH` | task tree depth byte; 0220fc82.c stores parent depth+1. |
| `0x08` | `OBJ_POS_X` | world X in selected actor/camera payloads; 0220d262.c computes radius*sin+base. |
| `0x0c` | `OBJ_POS_Y` | world Y in selected actor/camera payloads; 0220d262.c adds vertical base offset. |
| `0x10` | `OBJ_POS_Z` | world Z in selected actor/camera payloads; 0220d262.c computes radius*cos+base. |
| `0x24` | `OBJ_ANGLE_X` | first Euler angle in selected world objects; 0220b1c0.c loads angle triple. |
| `0x26` | `OBJ_ANGLE_Y` | second Euler/facing angle; 02227704.c chooses 0x4000/0xC000. |
| `0x28` | `OBJ_ANGLE_Z` | third Euler angle in selected world objects; 0220b1c0.c loads angle triple. |
| `0x34` | `ACTOR_SLOT_ID` | signed byte slot id; 02227bc4.c stores newly allocated id; 02227704.c indexes pose records. |
| `0x5c` | `ACTOR_ANIM_REQUEST` | requested animation word; 0222dc2c.c indexes recB and 0223a6dc.c loads pose animation id. |

Task names apply to confirmed `task` parameters and the two inspected task allocators. Position/angle names apply only to explicitly selected world-object payloads, not every `M32(..., 8)` or payload (wave-effect work reuses those offsets as phase data). Actor slot/animation names apply to the inspected animation/actor-handler family. Exact file/base/accessor substitutions are in [names.json](../work/readability1/names.json); constructors and actor registration establish the layouts. Field sizes and signedness remain expressed by the original accessors.

## Noise measurements (v1.00)

This pass: **29,840 → 26,408**, down **3,432 (11.50%)**. Against the supplied baseline: **29,690 → 26,408**, down **3,282 (11.05%)**. Both measurements parse all 1,099 files and 1,172 definitions without failures.

The actual starting tree already differed from `work/noise_report_baseline.json` by **+150 events** and 164 code lines. Its coefficient/line helpers were already named `reserve_coef` / `reserve_line` instead of baseline `push_coef` / `push_line`. Consequently the supplied-baseline comparison reports two added/two removed helper identities; **this pass added or removed zero function identities**, and code-line count remains 36,545. The saved original source was measured independently to isolate this pass.

| Category | Supplied baseline | Actual before | After | Pass delta | Supplied-baseline delta |
|---|---:|---:|---:|---:|---:|
| `raw_memory` | 2,561 | 2,580 | 1,747 | -833 | -814 |
| `offset_access` | 17,657 | 17,764 | 16,380 | -1,384 | -1,277 |
| `pointer_constant` | 1,068 | 1,075 | 913 | -162 | -155 |
| `hardware_address` | 264 | 265 | 224 | -41 | -40 |
| `address_function` | 2,799 | 2,817 | 2,817 | +0 | +18 |
| `numeric_call` | 94 | 94 | 94 | +0 | +0 |
| `unnamed_global` | 2,851 | 2,848 | 1,790 | -1,058 | -1,061 |
| `magic_offset` | 2,204 | 2,203 | 2,249 | +46 | +45 |
| `goto` | 80 | 81 | 81 | +0 | +1 |
| `label` | 37 | 38 | 38 | +0 | +1 |
| `register_variable` | 46 | 46 | 46 | +0 | +0 |
| `sign_extension` | 29 | 29 | 29 | +0 | +0 |
| **Total** | **29,690** | **29,840** | **26,408** | **−3,432** | **−3,282** |

Zero-noise definitions: **55 → 69** (4.69% → 5.89%). Noisy lines: **20,127 → 18,295** (55.07% → 50.06%). The **+46 `magic_offset`** sites are existing displacements newly classified after the surrounding absolute-address literals became identifiers: for example `MEM32(MEM32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c)` and view-task payload `+8` accesses. No displacements or operations were added (all preprocessed C tokens match). Raw-memory/pointer noise at those enclosing expressions falls. Other category increases relative to the supplied baseline predate this pass.

Reproduction (outputs stay within the requested scope):

```sh
tools/decomp/venv/bin/python tools/decomp/noise_report.py --src work/readability1/original/project/src_functional --symbols work/readability1/original/notes/symbols.txt --markdown work/readability1/noise_before.md --json work/readability1/noise_before.json
tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline compare --markdown work/readability1/noise_after.md --json work/readability1/noise_after.json
```

Artifacts: [before noise](../work/readability1/noise_before.json), [after/baseline comparison](../work/readability1/noise_after.json), [isolated pass deltas](../work/readability1/noise_comparison.json). The supplied baseline and `notes/noise_report.md` were left intact.

## Differential equality and compilation

Before any source edits, ran the full requested scan; after the edits, ran it again using identical default 16-vector configuration:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --json work/readability1/before.json --output-dir work/readability1/before_run > work/readability1/before.log 2>&1
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --json work/readability1/after.json --output-dir work/readability1/after_run > work/readability1/after.log 2>&1
```

| Status | Before | After | Changed addresses |
|---|---:|---:|---:|
| PASS | 110 | 110 | 0 |
| PASS-void | 675 | 675 | 0 |
| FAIL | 16 | 16 | 0 |
| SKIP | 298 | 298 | 0 |

**All 1,099 address→status entries are identical**, with no missing/added addresses. Cart, harness, reference hashes, vector count, seed and model metadata match. Both scan commands exit 1 because the starting tree already has 16 FAILs; every original FAIL and SKIP remains in the same status. No status-changing edits were retained. PASS remains bounded differential-test evidence under the existing oracle, not a general correctness proof.

Canonical sorted address/status SHA-256 (both runs):

`0886c667d70ae7e9c06003bba8d7f14b5741e6ab23ba78d276f979d71996faed`

Full per-address equality and both JSON hashes: [status_equality.json](../work/readability1/status_equality.json). Recheck with `python3 work/readability1/finish_report.py`.

Whole-tree gate: `gcc -O1 -c -I project/src_functional <file> -o /dev/null` for every C translation unit, as documented in `notes/decomp_tail_opus_20261030.md` and the recent whole-tree check notes. **1,099 compiled; zero failures; 17 translation units emitted existing diagnostics.** In addition, GCC-preprocessed C tokens match the saved originals for **all 1,099 TUs**, after normalizing whitespace and equivalent integer notation while retaining literal U/L suffixes. This checks preserved extern identities, widths, qualifiers, offsets, expressions and call order, including code the oracle skips. No function bodies, control flow or ABI were changed.

Compile diagnostics, original/current token hashes and source hashes: [whole_tree_compile.json](../work/readability1/whole_tree_compile.json). Recheck with `python3 work/readability1/verify_tree.py`. `git diff --check` passes. The floor registry was consulted; this pass made naming-only substitutions and did not attempt functional repairs of floor entries.

Changes are limited to `project/src_functional`, `notes/symbols.txt`, this note and `work/readability1/`. No tools, port files, baseline snapshot or unrelated untracked files were changed. No commit was made.
